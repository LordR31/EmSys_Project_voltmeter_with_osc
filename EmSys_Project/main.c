#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define F_CPU 16000000UL // cpu freq


// ADC
#define BAUD 9600
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)
#define MAX_VOLTS 5 // Volts (max value for adc, for AVCC)
#define ADC_MAX_VALUE 1023 // 10 bit adc -> 0 - 1023 range
#define NUM_SAMPLES 10


// LED PINS
#define GREEN_LED PC5
#define ORANGE_LED PC3
#define RED_LED PC4


// LCD PINS & PORTS
#define LCD_RS_BIT PC0
#define LCD_EN_BIT PC1
#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR  DDRA
#define LCD_CTRL_PORT PORTC
#define LCD_CTRL_DDR  DDRC


// FLAGS & VARIABLES
volatile bool is_adc_on = false;
volatile bool button_pressed = false;
volatile uint32_t last_button_time = 0;
volatile uint32_t millis_counter = 0;

bool is_green_led_on = false;
bool is_orange_led_on = false;
bool is_red_led_on = false;

//////////////////////
////    TIMER0    ////
//////////////////////


void Timer0_Init(void){
	TCCR0A = 0x00;                          // reset counter config
	
	TCCR0B &= ~(1 << CS02);                 // clear CS02
	TCCR0B |= (1 << CS01) | (1 << CS00);    // set CS00 & CS01 for 64 prescaler (but this is a bit too slow still, 1.024ms instead of 1 ms flat for ovf => preload the counter)
	
	TCNT0 = 6;                              // preload counter value to get exactly 1ms for ovf
	TIMSK0 |= (1 << TOIE0);                 // enable Timer0 overflow interrupt
}

ISR(TIMER0_OVF_vect){
	TCNT0 = 6;           // reload for next 1 ms
	millis_counter++;    // increment ms counter
}


//////////////////////
////    TIMER1    ////
//////////////////////


void Timer1_Init(void){
	TCCR1A = 0x00;                          // reset counter config
	
	TCCR1B &= ~((1 << CS12) | (1 << CS11)); // clear CS12 and CS11
	TCCR1B |= (1 << CS10);                  // set CS10 for no prescaler
	
	TCNT1 = 0;                              // clear counter value
	TIFR1 |= (1 << OCF1A);                  // clear compare flag
}


void timer_delay_ms(uint16_t ms){
	while (ms--){
		uint16_t ticks = 16000; // 1 ms = 16000 ticks at 16 MHz

		OCR1AH = (ticks >> 8);
		OCR1AL = (ticks & 0xFF);

		TCNT1 = 0;
		TIFR1 |= (1 << OCF1A);

		while (!(TIFR1 & (1 << OCF1A)));
	}
}

void timer_delay_us(uint16_t us){
	uint16_t ticks = us * 16; // 1 us = 16 ticks

	OCR1AH = (ticks >> 8);
	OCR1AL = (ticks & 0xFF);

	TCNT1 = 0;
	TIFR1 |= (1 << OCF1A);

	while (!(TIFR1 & (1 << OCF1A)));
}


///////////////////////
////    BUTTONs    ////
///////////////////////


void BUTTON_Init(void){
	DDRE &= ~(1 << PE4);                     // set button as input
	PORTE |= (1 << PE4);                     // pull-up resistor
	EICRB &= ~((1 << ISC41) | (1 << ISC40)); // clear both regs
	EICRB |= (1 << ISC41);                   // set ISC40 (01)

	EIMSK |= (1 << INT4);                    // interrupt mask reg
} 

ISR(INT4_vect) {
	button_pressed = true;
}


////////////////////
////    LEDs    ////
////////////////////

void LED_Init(void){
	DDRC |= (1 << PC5) | (1 << PC4) | (1 << PC3); // set pins as output
	
	// startup LEDs check
	PORTC |= (1 << PC5) | (1 << PC4) | (1 << PC3);
	timer_delay_ms(1000);
	PORTC ^= (1 << PC5) | (1 << PC4) | (1 << PC3);
}


////////////////////
////    ADC0    ////
////////////////////


void ADC_Init(void){
	ADMUX = 1 << REFS0;                                                // select avcc & adc0
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable adc & set prescaler to 128
}


///////////////////
////    LCD    ////
///////////////////


void LCD_GPIO_Init(void){
	LCD_DATA_DDR = 0xFF;                                   // PORTA pins as output
	LCD_CTRL_DDR |= (1 << LCD_RS_BIT) | (1 << LCD_EN_BIT); // RS and EN as output
}

void LCD_write_byte(uint8_t byte, uint8_t rs){
	// RS
	if (rs) LCD_CTRL_PORT |= (1 << LCD_RS_BIT);
	else    LCD_CTRL_PORT &= ~(1 << LCD_RS_BIT);

	// Data
	LCD_DATA_PORT = byte;

	// Pulse EN
	LCD_CTRL_PORT |= (1 << LCD_EN_BIT);
	timer_delay_ms(1);
	LCD_CTRL_PORT &= ~(1 << LCD_EN_BIT);
	timer_delay_ms(1);
}

void LCD_send_command(uint8_t cmd){
	LCD_write_byte(cmd, 0);
}

void LCD_send_data(uint8_t data){
	LCD_write_byte(data, 1);
}

void LCD_set_cursor(uint8_t row, uint8_t col){
	uint8_t row_offsets[] = {0x00, 0x40};
	if (row > 1) row = 1;
	LCD_send_command(0x80 | (row_offsets[row] + col));
}

void LCD_Init(void){
	LCD_GPIO_Init();
	timer_delay_ms(40);

	LCD_send_command(0x38); // 8-bit, 2-line, 5x8 font
	LCD_send_command(0x0C); // display ON, cursor OFF
	LCD_send_command(0x06); // entry mode
	LCD_send_command(0x01); // clear display
	timer_delay_ms(100);
}

void LCD_print(const char *str){
	while (*str){
		LCD_send_data(*str++);
	}
}


/////////////////////////////
////    MAIN FUNCTION    ////
/////////////////////////////


int main(void){
	sei();
	Timer0_Init();
	Timer1_Init();
	LCD_Init();
	//LCD_set_cursor(0, 1);     
	//LCD_print("VOLTAGE SCREEN"); 
	
	LED_Init();
	BUTTON_Init();
	ADC_Init();
	
	uint16_t adc_value;
	float voltage_value;
	while (1){
		if (button_pressed && (millis_counter - last_button_time > 300)) {                      // 1s debounce
			is_adc_on = !is_adc_on;
			last_button_time = millis_counter;
			button_pressed = false;
		}

		if(is_adc_on){
			is_green_led_on = true;
			uint32_t adc_sum = 0;
			for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
				ADCSRA |= (1 << ADSC);                        // Start ADC
				while (ADCSRA & (1 << ADSC));                 // Wait for conversion
				adc_sum += (ADCL | (ADCH << 8));              // Accumulate result
				timer_delay_ms(1);                            // Small delay between samples
			}

			adc_value = adc_sum / NUM_SAMPLES;                // Average result
			char adc_value_string[10];
			sprintf(adc_value_string, "%u ", adc_value);
			LCD_set_cursor(0,3);
			LCD_print(adc_value_string);
			float calibrated_max_volts = 0;
			if(is_green_led_on & !is_orange_led_on & !is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.037;
			else if(is_orange_led_on & !is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.029;
			else if(is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.017;
			
			voltage_value = ((float)(adc_value) * calibrated_max_volts) / ADC_MAX_VALUE;                    // convert value
			if(is_green_led_on & !is_orange_led_on & !is_red_led_on & (adc_value > 0))
				voltage_value += 0.010
			else if(is_red_led_on)
				voltage_value -= 0.008;
			
			
			char voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);                              // convert to millivolts
			sprintf(voltage_value_string, "     %u.%03uV    ", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			LCD_set_cursor(1, 0);
			LCD_print(voltage_value_string);

			PORTC |= (1 << GREEN_LED);                                                           // Green LED always on if ADC is on
			
			if(voltage_value > 1.5){                                                              // Orange LED turned on if voltage is above 1.5V
				PORTC |= (1 << ORANGE_LED);
				is_orange_led_on = true;
			}
			else if(voltage_value < 1.5){
				PORTC &= ~(1 << ORANGE_LED);
				is_orange_led_on = false;
			}
			if(voltage_value >= 3.5){                                                             // Red LED turned on if voltage is above 3.5V
				PORTC |= (1 << RED_LED);
				is_red_led_on = true;
			}
			else if(voltage_value < 3.5){
				PORTC &= ~(1 << RED_LED);
				is_red_led_on = false;
			}

			timer_delay_ms(1000);
		}else{
			is_green_led_on = false;
			is_orange_led_on = false;
			is_red_led_on = false;
			PORTC &= ~((1 << GREEN_LED) | (1 << ORANGE_LED) | (1 << RED_LED));                   // Green LED always off if ADC is on & turn off the other LEDs
			LCD_set_cursor(1, 0);
			LCD_print("   System OFF   ");
		}
	}
}

