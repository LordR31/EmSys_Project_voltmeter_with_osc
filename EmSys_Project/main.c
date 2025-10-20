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
	sei();                                                                                               // enable interrupts                                                               
	Timer0_Init(); 					                                                                     // init timer 0 for button debounce
	Timer1_Init(); 					                                                                     // init timer 1 for delay_ms and delay_us
	start_button_init(); 			                                                                     // init GPIO start button
	cursor_buttons_init(); 			                                                                     // init GPIO cursor buttons
	toggle_votalge_button_init(); 	                                                                     // init GPIO voltage type button
	ADC_Init(); 					                                                                     // init the ADC
	SPI_init(); 					                                                                     // init SPI 
	Touch_SPI_Init(); 				                                                                     // init the pins for the touchscreen side (should merge SPI and Touch_SPI)
	Touch_IRQ_Init(); 				                                                                     // init Touch Interrupt pin
	Display_Init(); 				                                                                     // init the display
	
	display_set_rotation(3);                                                                             // landscape inverted
	display_fill_color(COLOR_BLACK);
	
	uint16_t adc_value;
	float voltage_value;
	float max_value = 0;
	float min_value = 5;
	float plot_points[25] = {0};
	int index = 0;
	
	//draw_cursor();
	while (1){
		if (start_button_pressed && (millis_counter - last_button_time > 300)) {                         // 300ms debounce
			is_adc_on = !is_adc_on;
			last_button_time = millis_counter;
			display_fill_color(COLOR_BLACK);
			start_button_pressed = false;
		}
		
		if (voltage_button_pressed && (millis_counter - last_button_time > 300)) {                       // 300ms debounce
			is_high_voltage = !is_high_voltage;
			last_button_time = millis_counter;
			voltage_button_pressed = false;
		}

		if(is_adc_on){		
			set_text_size(2);                                                                            // set text size 2
			display_set_color(COLOR_BLUE);                                                               // set text colour blue
			if(!is_cursor_on)
				display_print("Voltage:", 2, 10);                                                        // top left corner of the screen, Voltmeter text
			else{
				display_print("Voltage:", 2, 0);
				display_print("Cursor:", 2, 15);
			}

			display_set_color(COLOR_GREEN);
			if(is_high_voltage)
				display_print("H", 188, 10);
			else
				display_print("L", 188, 10);

			display_draw_line(0, 35, display_width, 35, COLOR_WHITE);                                    // horizontal voltage line
			display_draw_line(184, 35, 184, 0, COLOR_WHITE);                                             // vertical voltage line
			
			display_draw_line(280, 57, 280, 217, COLOR_WHITE);                                           // top horizontal button menu line
			display_draw_line(280, 57, 340, 57, COLOR_WHITE);                                            // bottom horizontal button menu line
			display_draw_line(280, 97, display_width, 97, COLOR_WHITE);                                  // 1st button horizontal line
			display_draw_line(280, 137, display_width, 137, COLOR_WHITE);                                // 2nd button horizontal line
			display_draw_line(280, 177, display_width, 177, COLOR_WHITE);                                // 3rd button horizontal line
			display_draw_line(280, 217, display_width, 217, COLOR_WHITE);                                // 4th button horizontal line
			
			                                                                       
			set_text_size(1);                                                                            // touchscreen button text (right side of the screen)
			display_set_color(COLOR_GREEN);
			display_print("Cursor", 285, 72);
			display_print("Hold", 290, 112);
			display_print("Wave", 290, 152);
			display_print("Toggle", 283, 192);
			
			if (touch_pending) {
				touch_pending = false;
				
				uint16_t x = touch_spi_transfer(0x90);                                                   // get touchscreen x
				uint16_t y = touch_spi_transfer(0xD0);                                                   // get touchscreen y

				uint8_t which_button = check_touch_buttons(x, y);                                        // check if it is a button press or not
				switch(which_button){                                                                    // do what the button needs to do
					case 1: 
						if(!is_plot_on){                                                                 // if the waveform viewer is not ENABLED but the cursor button was pressed
							set_text_size(2);
							display_set_color(COLOR_RED);
							display_print("Plot must be enabled", 25, 60);                               // warn the user about the problem
							display_print("for cursor usage!   ", 22, 80);
						}else{
							if(!is_cursor_on){                                                           // otherwise, if the waveform viewer is on and the cursor is off
								draw_cursor();                                                           // turn the cursor on
								is_cursor_on = true;
								}else{                                                                   // but if the cursor was already on when the button was pressed
									erase_cursor();                                                      // erase the cursor (turn it off)
									erase_voltage_zone();                                                // and clear the Voltmeter area (clear "Cursor: xx.xxxV")
									is_cursor_on = false; 
							}
						}
						break;
					case 2:
						is_holding = !is_holding;                                                        // if HOLD is off, turn it on and vice-versa
						break;
					case 3:
						if(is_plot_on){                                                                  // if the waveform viewer is on
							is_plot_on = false;                                                          // turn it off
							is_cursor_on = false;                                                        // turn off the cursor
							is_holding = false;                                                          // turn off HOLD
							erase_cursor();                                                              // erase the cursor
							erase_voltage_zone();                                                        // clear the Voltmeter area
							clear_plot();                                                                // clear the waveform viewer area (erase the plot)
						}else{                                                                           // otherwise,
							clear_plot();                                                                // clear the waveform viewer area (erase the text to make space for the viewer)
							is_plot_on = true;                                                           // and turn on the plotting
						}
						break;
					case 4:
						if(!is_plot_on){                                                                 // if the waveform viewer is not ENABLED but the toggle button was pressed
							set_text_size(2);
							display_set_color(COLOR_RED);
							display_print("Plot must be enabled", 25, 60);                               // warn the user about the problem
							display_print("to toggle plot type!", 22, 80);	
						}else{                                                                           // otherwise, change from the simple waveform viewer type to the one with samples (vertical lines) and vice-versa
							if(!is_digital_line){
								is_digital_line = true;
								plot_points_digital(plot_points, index, max_value, min_value);
								if(is_cursor_on){                                                        // if the cursor was on, erase and redraw it
									erase_cursor();
									draw_cursor();
								}
							}else{
								is_digital_line = false;
								plot_points_line(plot_points, index, max_value, min_value);
								if(is_cursor_on){                                                        // if the cursor was on, erase and redraw it
									erase_cursor();
									draw_cursor();
								}
							}		
						}
						break;
				}
			}
			
			voltage_value = ADC_measure(is_high_voltage);                                                // get voltage value from adc, based on voltage type (high or low)
			if(!is_holding)                                                                              // if HOLD is not on, record the values (up to the 25th value)
				if(index < 25){
					plot_points[index] = voltage_value;                                                  // add the voltage values to the end of the array until array is full
					index++;
				}else
					insert_plot_points(plot_points, voltage_value);                                      // insert the points into the plot_points array by disarding the first point (oldest point)
			
			min_value = plot_points[0];
			max_value = plot_points[0];
			for(int i = 0; i < index; i++){                                                              // get min and max from the recorded points for plot scaling
				if(plot_points[i] < min_value)         
					min_value = plot_points[i];
				if(plot_points[i] > max_value)    
					max_value = plot_points[i];
			}
			
			
			if(!is_holding & is_plot_on)                                                                 // if HOLD is off and the waveform viewer is enabled, keep updating it when new points are recorded
				if(is_digital_line)
					plot_points_digital(plot_points, index, max_value, min_value);
				else
					plot_points_line(plot_points, index, max_value, min_value);
			
			char voltage_value_string[10];
			uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);                                      // convert to mV for float -> int conversion
			if(voltage_value > 10)                                                                       // if voltage < 10 add a space before the value, to clean up previous measurements > 10
				sprintf(voltage_value_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));       // convert mV to V in string (sprintf doesn't seem to work with float to string directly...)
			else
				sprintf(voltage_value_string, " %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));
			set_text_size(2);
			display_set_color(COLOR_BLUE);
			if(!is_cursor_on)                                                                            // if the cursor was enabled, move the voltage to the top of the voltmeter zone (instead of in the middle of the zone)
				display_print(voltage_value_string, 100, 9);                                             // to make space for the cursor voltage
			else
				display_print(voltage_value_string, 100, 0);
			char max_value_string[32];
			voltage_mV = (uint16_t)(max_value * 1000);                                                   // same as above, convert to mV
			sprintf(max_value_string, "Max Voltage: %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));  // convert mV to V in string
			
			char min_value_string[32];
			voltage_mV = (uint16_t)(min_value * 1000);                                                   // same as above, conver to mV
			sprintf(min_value_string, "Min Voltage: %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));  // convert mV to V in string
			
			if(is_plot_on){                                                                              // if the plot is enable (waveform viewer)
				set_text_size(1);
				display_draw_line (20, 57, 260, 57, COLOR_WHITE);                                        // draw the upper line for the plot
				display_draw_line (20, 217, 260, 217, COLOR_WHITE);                                      // draw the lower line for the plot
				display_print(max_value_string, 90, 47);                                                 // place max_value on top of the top line
				display_print(min_value_string, 90, 222);                                                // place min_valu below the bottom line
			}else{
				display_print(max_value_string, 25, 120);                                                // otherwise, keep them both in the middle of the waveform viewer area
				display_print(min_value_string, 25, 140);
			}
			
			if (is_cursor_on) {                                                          
				if (right_button_pressed && (millis_counter - last_right_time > 300)) {                  // 300ms debounce for the move cursor right button
					last_right_time = millis_counter;
					move_cursor(1);                                                                      // move the cursor to the right one point
					right_button_pressed = false;
				}

				if (left_button_pressed && (millis_counter - last_left_time > 300)) {                    // 300ms debounce for the move cursor left button
					last_left_time = millis_counter;
					move_cursor(-1);                                                                     // move the cursor to the left one point
					left_button_pressed = false;
				}
				cursor_voltage = get_cursor_voltage();                                                   // get the voltage of the point at the cursor position
				char cursor_voltage_string[10];
				voltage_mV = (uint16_t)(cursor_voltage * 1000);                                          // same as above, convert to mV
				if(voltage_value > 10)
					sprintf(cursor_voltage_string, "%u.%03uV", voltage_mV / 1000, (voltage_mV % 1000));  // convert mV to V in string
				else
					sprintf(cursor_voltage_string, " %u.%03uV", voltage_mV / 1000, (voltage_mV % 1000)); // add a space before the value, if voltage < 10, to clean up previous >10 measurements
				display_set_color(COLOR_BLUE);
				set_text_size(2);
					display_print(cursor_voltage_string, 100, 15);                                       // print the cursor voltage value (in the Voltmeter area)
			}

			
			draw_indicator_leds(voltage_value);                                                          // draw the voltage level indicator "LEDs"
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

