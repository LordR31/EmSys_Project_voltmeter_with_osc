#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

#include "adc.h"
#include "buttons.h"
#include "timers.h"

#define F_CPU 16000000UL // cpu freq

// FLAGS & VARIABLES
volatile bool is_adc_on = false;
extern bool button_pressed;
volatile uint32_t last_button_time = 0;
extern uint32_t millis_counter;

bool is_green_led_on = false;
bool is_orange_led_on = false;
bool is_red_led_on = false;

ISR(TIMER0_OVF_vect){
	TCNT0 = 6;           // reload for next 1 ms
	millis_counter++;    // increment ms counter
}

ISR(INT4_vect) {
	button_pressed = true;
}

int main(void){
	sei();
	Timer0_Init();
	Timer1_Init();
	start_button_init();
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
			adc_value = ADC_measure();
			char adc_value_string[10];
			sprintf(adc_value_string, "%u ", adc_value);
			float calibrated_max_volts = 0;
			if(is_green_led_on & !is_orange_led_on & !is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.037;
			else if(is_orange_led_on & !is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.029;
			else if(is_red_led_on)
				calibrated_max_volts = MAX_VOLTS + 0.017;
			
			voltage_value = ((float)(adc_value) * calibrated_max_volts) / ADC_MAX_VALUE;                // convert value
			if(is_green_led_on & !is_orange_led_on & !is_red_led_on & (adc_value > 0))
				voltage_value += 0.010;
			else if(is_red_led_on)
				voltage_value -= 0.008;
			
			
			char voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);                                     // convert to millivolts
			sprintf(voltage_value_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			
			if(voltage_value > 1.5){                                                                    // orange LED turned on if voltage is above 1.5V
				is_orange_led_on = true;
			}
			else if(voltage_value < 1.5){
				is_orange_led_on = false;
			}
			if(voltage_value >= 3.5){                                                                   // red LED turned on if voltage is above 3.5V
				is_red_led_on = true;
			}
			else if(voltage_value < 3.5){
				is_red_led_on = false;
			}

			timer_delay_ms(1000);
		}else{
			is_green_led_on = false;
			is_orange_led_on = false;
			is_red_led_on = false;
		}
	}
}

