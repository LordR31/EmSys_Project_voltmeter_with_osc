#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

#include "adc.h"
#include "buttons.h"
#include "timers.h"
#include "spi.h"
#include "display.h"
#include "touch.h"

#define F_CPU 16000000UL // cpu freq
#define MAX_POINTS 25

// FLAGS & VARIABLES
volatile bool is_adc_on = false;

volatile bool start_button_pressed;
volatile bool left_button_pressed;
volatile bool right_button_pressed;

volatile uint32_t last_button_time = 0;
volatile uint32_t last_left_time = 0;
volatile uint32_t last_right_time = 0;
extern uint32_t millis_counter;

volatile bool touch_pending = false;
bool is_cursor_on = false;
bool is_plot_on = false;
bool is_digital_line = false;
bool is_holding = false;

bool voltage_button_pressed = false;
bool is_high_voltage = true;

extern uint16_t display_width;
extern uint16_t display_height;

float cursor_voltage = 0;

ISR(TIMER0_OVF_vect){
	TCNT0 = 6;           // reload for next 1 ms
	millis_counter++;    // increment ms counter
}

ISR(INT4_vect) {
	start_button_pressed = true;
}

ISR(INT3_vect){
	voltage_button_pressed = true;
}

ISR(INT2_vect) {
	touch_pending = true;
}

ISR(INT1_vect){
	right_button_pressed = true;
}

ISR(INT0_vect){
	left_button_pressed = true;
}

void insert_plot_points(float *plot_points, float point){
	for(int i = 1; i < MAX_POINTS; i++)
	plot_points[i - 1] = plot_points[i];
	plot_points[MAX_POINTS - 1] = point;
}

int main(void){
	sei();
	Timer0_Init();
	Timer1_Init();
	start_button_init();
	cursor_buttons_init();
	toggle_votalge_button_init();
	ADC_Init();
	SPI_init();
	Touch_SPI_Init();
	Touch_IRQ_Init();
	Display_Init();
	
	display_set_rotation(3);             // Landscape
	display_fill_color(COLOR_BLACK);
	
	uint16_t adc_value;
	float voltage_value;
	float max_value = 0;
	float min_value = 5;
	float plot_points[25] = {0};
	int index = 0;
	
	//draw_cursor();
	while (1){
		if (start_button_pressed && (millis_counter - last_button_time > 300)) {                      // 1s debounce
			is_adc_on = !is_adc_on;
			last_button_time = millis_counter;
			display_fill_color(COLOR_BLACK);
			start_button_pressed = false;
		}
		
		if (voltage_button_pressed && (millis_counter - last_button_time > 300)) {                      // 1s debounce
			is_high_voltage = !is_high_voltage;
			last_button_time = millis_counter;
			voltage_button_pressed = false;
		}

		if(is_adc_on){		
			set_text_size(2);
			display_set_color(COLOR_BLUE);  // Blue
			if(!is_cursor_on)
				display_print("Voltage:", 2, 10);
			else{
				display_print("Voltage:", 2, 0);
				display_print("Cursor:", 2, 15);
			}

			display_set_color(COLOR_GREEN);
			if(is_high_voltage)
				display_print("H", 188, 10);
			else
				display_print("L", 188, 10);

			display_draw_line(0, 35, display_width, 35, COLOR_WHITE);     // horizontal voltage line
			display_draw_line(184, 35, 184, 0, COLOR_WHITE);              // vertical voltage line
			
			display_draw_line(280, 57, 280, 217, COLOR_WHITE);            // horizontal button line
			display_draw_line(280, 57, 340, 57, COLOR_WHITE);
			display_draw_line(280, 97, display_width, 97, COLOR_WHITE);   // 1st button
			display_draw_line(280, 137, display_width, 137, COLOR_WHITE); // 2nd button
			display_draw_line(280, 177, display_width, 177, COLOR_WHITE); // 3rd button
			display_draw_line(280, 217, display_width, 217, COLOR_WHITE); // 4th button
			
			// button text
			set_text_size(1);
			display_set_color(COLOR_GREEN);
			display_print("Cursor", 285, 72);
			display_print("Hold", 290, 112);
			display_print("Wave", 290, 152);
			display_print("Toggle", 283, 192);
			
			if (touch_pending) {
				touch_pending = false;
				
				uint16_t x = touch_spi_transfer(0x90);
				uint16_t y = touch_spi_transfer(0xD0);

				uint8_t which_button = check_touch_buttons(x, y); 
				switch(which_button){
					case 1: 
						if(!is_plot_on){
							set_text_size(2);
							display_set_color(COLOR_RED);
							display_print("Plot must be enabled", 25, 60);
							display_print("for cursor usage!   ", 22, 80);
						}else{
							if(!is_cursor_on){
								draw_cursor();
								is_cursor_on = true;
								}else{
								erase_cursor();
								erase_voltage_zone();
								is_cursor_on = false;
							}
						}
						break;
					case 2:
						is_holding = !is_holding;
						break;
					case 3:
						if(is_plot_on){
							is_plot_on = false;
							is_cursor_on = false;
							is_holding = false;
							erase_cursor();
							erase_voltage_zone();
							clear_plot();
						}else{
							clear_plot();
							is_plot_on = true;
						}
						break;
					case 4:
						if(!is_plot_on){
							set_text_size(2);
							display_set_color(COLOR_RED);
							display_print("Plot must be enabled", 25, 60);
							display_print("to toggle plot type!", 22, 80);	
						}else{
							if(!is_digital_line){
								is_digital_line = true;
								plot_points_digital(plot_points, index, max_value, min_value);
								if(is_cursor_on){
									erase_cursor();
									draw_cursor();
								}
							}else{
								is_digital_line = false;
								plot_points_line(plot_points, index, max_value, min_value);
								if(is_cursor_on){
									erase_cursor();
									draw_cursor();
								}
							}		
						}
						break;
				}
			}
			
			voltage_value = ADC_measure(is_high_voltage);        // get voltage value from adc
			if(!is_holding)
				if(index < 25){
					plot_points[index] = voltage_value;
					index++;
				}else
				insert_plot_points(plot_points, voltage_value);
			
			min_value = plot_points[0];
			max_value = plot_points[0];
			for(int i = 0; i < index; i++){
				if(plot_points[i] < min_value)         // compare with min
					min_value = plot_points[i];
				if(plot_points[i] > max_value)         // compare with max
					max_value = plot_points[i];
			}
			
			
			if(!is_holding & is_plot_on)
				if(is_digital_line)
					plot_points_digital(plot_points, index, max_value, min_value);
				else
					plot_points_line(plot_points, index, max_value, min_value);
			
			char voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);                            // convert to millivolts
			if(voltage_value > 10)
				sprintf(voltage_value_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			else
				sprintf(voltage_value_string, " %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));
			set_text_size(2);
			display_set_color(COLOR_BLUE);
			if(!is_cursor_on)
				display_print(voltage_value_string, 100, 9);
			else
				display_print(voltage_value_string, 100, 0);
			char max_value_string[32];
			voltage_mV = (uint16_t)(max_value * 1000);                            // convert to millivolts
			sprintf(max_value_string, "Max Voltage: %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			
			char min_value_string[32];
			voltage_mV = (uint16_t)(min_value * 1000);                            // convert to millivolts
			sprintf(min_value_string, "Min Voltage: %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			
			if(is_plot_on){
				set_text_size(1);
				display_draw_line (20, 57, 260, 57, COLOR_WHITE);    // Upper line for the plot
				display_draw_line (20, 217, 260, 217, COLOR_WHITE);  // Lower line for the plot
				display_print(max_value_string, 90, 47);
				display_print(min_value_string, 90, 222);
			}else{
				display_print(max_value_string, 25, 120);
				display_print(min_value_string, 25, 140);
			}
			
			if (is_cursor_on) {
				if (right_button_pressed && (millis_counter - last_right_time > 300)) {
					last_right_time = millis_counter;
					move_cursor(1);
					right_button_pressed = false;
				}

				if (left_button_pressed && (millis_counter - last_left_time > 300)) {
					last_left_time = millis_counter;
					move_cursor(-1);
					left_button_pressed = false;
				}
				cursor_voltage = get_cursor_voltage();
				char cursor_voltage_string[10];
				voltage_mV = (uint16_t)(cursor_voltage * 1000);                            // convert to millivolts
				if(voltage_value > 10)
					sprintf(cursor_voltage_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
				else
					sprintf(cursor_voltage_string, " %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));
				display_set_color(COLOR_BLUE);
				set_text_size(2);
					display_print(cursor_voltage_string, 100, 15);
			}

			
			draw_indicator_leds(voltage_value);
		}else{
			set_text_size(3);
			display_set_color(COLOR_GREEN);
			display_print("Power      ON", 50, 60);
			display_print("Display    ON", 50, 90);
			display_print("System     OFF", 50, 120);
			display_print("Waiting...", 50, 180);
		}
	}
}

