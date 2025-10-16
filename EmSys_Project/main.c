#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

#include "adc.h"
#include "buttons.h"
#include "timers.h"
#include "spi.h"
#include "display.h"

#define F_CPU 16000000UL // cpu freq

#define MAX_POINTS 25

// FLAGS & VARIABLES
volatile bool is_adc_on = false;
extern bool button_pressed;
volatile uint32_t last_button_time = 0;
extern uint32_t millis_counter;

bool is_cursor_on = false;
bool is_plot_on = true;
bool is_digital_line = true;

extern uint16_t display_width;
extern uint16_t display_height;

ISR(TIMER0_OVF_vect){
	TCNT0 = 6;           // reload for next 1 ms
	millis_counter++;    // increment ms counter
}

ISR(INT4_vect) {
	button_pressed = true;
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
	ADC_Init();
	SPI_init();
	Display_Init();
	
	display_set_rotation(3);             // Landscape
	display_fill_color(COLOR_BLACK);
	
	uint16_t adc_value;
	float voltage_value;
	float max_value = 0;
	float min_value = 5;
	float plot_points[25] = {0};
	int index = 0;
	while (1){
		if (button_pressed && (millis_counter - last_button_time > 300)) {                      // 1s debounce
			is_adc_on = !is_adc_on;
			last_button_time = millis_counter;
			display_fill_color(COLOR_BLACK);
			button_pressed = false;
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
			display_print("*", 188, 10);

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
			
			voltage_value = ADC_measure();        // get voltage value from adc
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
			
			
			if(is_plot_on)
				if(is_digital_line)
					plot_points_digital(plot_points, index, max_value, min_value);
				else
					plot_points_line(plot_points, index, max_value, min_value);
			
			char voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);                            // convert to millivolts
			sprintf(voltage_value_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // convert milivolts to Volts in string (sprintf doesn't seem to work from float to string directly...)
			set_text_size(2);
			display_set_color(COLOR_BLUE);
			if(!is_cursor_on)
				display_print(voltage_value_string, 110, 9);
			else
				display_print(voltage_value_string, 110, 0);
			
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
			
			draw_indicator_leds(voltage_value);
			timer_delay_ms(1000);
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

