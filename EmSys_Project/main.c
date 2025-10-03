#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)

#define MAX_VOLTS 5.048 // Volts (max value for adc, for AVCC)
#define ADC_MAX_VALUE 1023 // 10 bit adc -> 0 - 1023 range

#define CMD_BUFFER_SIZE 32
volatile char command_buffer[CMD_BUFFER_SIZE];
volatile uint8_t cmd_index = 0;

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

/*
Using PuTTY, to be able to send a character or characters, do the following:

In Terminal, set:
Local Echo -> Force on
Local Line editing -> Force on
*/

// FLAGS FOR USER COMMANDS
bool is_adc_on = true;
bool show_voltage = false;


///////////////////////
////    BUTTONs    ////
///////////////////////


void BUTTON_Init(void){
	DDRE &= ~(1 << PE4); // set button as input
	PORTE |= (1 << PE4); // pull-up resistor
	EICRB &= ~(1 << ISC41);  // enable interrput on any logical change (01)
	EICRB |= (1 << ISC40);
	EIMSK |= (1 << INT4); // interrupt mask reg

	sei();
} 

ISR(INT4_vect) {
	if(is_adc_on)
		is_adc_on = false;
	else
		is_adc_on = true;
}

////////////////////
////    LEDs    ////
////////////////////

void LED_Init(void){
	DDRC |= (1 << PC5) | (1 << PC4) | (1 << PC3); // set pins as output
	
	// startup animation
	for(int i = 0; i < 6; i++){
		PORTC ^= (1 << PC5) | (1 << PC4) | (1 << PC3);
		_delay_ms(5000);
	}
}


////////////////////
////    ADC0    ////
////////////////////


void ADC_Init(void){
	ADMUX = 1 << REFS0; // select avcc & adc0
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable adc & set prescaler to 128
}


///////////////////
////    LCD    ////
///////////////////


void LCD_GPIO_Init(void){
	LCD_DATA_DDR = 0xFF; // All PORTA pins as output
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
	_delay_ms(1);
	LCD_CTRL_PORT &= ~(1 << LCD_EN_BIT);
	_delay_ms(1);
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
	_delay_ms(40);

	LCD_send_command(0x38); // 8-bit, 2-line, 5x8 font
	LCD_send_command(0x0C); // Display ON, cursor OFF
	LCD_send_command(0x06); // Entry mode
	LCD_send_command(0x01); // Clear display
	_delay_ms(100);
}

void LCD_print(const char *str){
	while (*str){
		LCD_send_data(*str++);
	}
}

int main(void){
	LCD_Init();
	LCD_set_cursor(0, 1);       // Top-left corner
	LCD_print("VOLTAGE SCREEN"); // Writes to LCD
	
	LED_Init();
	BUTTON_Init();
	ADC_Init();
	
	uint16_t adc_value;
	float voltage_value;
	while (1){
		if(is_adc_on){
			ADCSRA |= (1 << ADSC);                                             // start adc
			while (ADCSRA & (1 << ADSC));                                      // wait for conversion
			adc_value = (ADCL | (ADCH << 8));                                  // get value
			
			voltage_value = ((float)(adc_value) * MAX_VOLTS) / ADC_MAX_VALUE;  // convert value
			const char *voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);  // Convert to millivolts
			sprintf(voltage_value_string, "%u.%02uV  ", voltage_mV / 1000, (voltage_mV % 1000));
			LCD_set_cursor(1, 1);
			LCD_print("Voltage ");
			LCD_print(voltage_value_string);

			if(voltage_value < 1.5)
				PORTC |= (1 << PC5);
			else if(voltage_value < 0)
				PORTC &= ~(1 << GREEN_LED);
			
			if(voltage_value > 1.5 && voltage_value < 3.5)
				PORTC |= (1 << ORANGE_LED);
			else if(voltage_value < 1.5)
				PORTC &= ~(1 << ORANGE_LED);
			
			if(voltage_value >= 3.5)
				PORTC |= (1 << RED_LED);
			else if(voltage_value < 3.5)
				PORTC &= ~(1 << RED_LED);

			for(volatile int i = 0; i < 30000; i++);
			for(volatile int i = 0; i < 30000; i++);
				//_delay_ms(10000);                                                  // 1000ms was instant, somehow, so i put 10000ms and it seems to work now
			}
		else
			LCD_set_cursor(1, 3);
			LCD_print("System OFF");
		}
}

