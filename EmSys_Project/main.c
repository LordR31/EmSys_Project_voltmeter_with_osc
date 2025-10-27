#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "adc.h"
#include "buttons.h"
#include "timers.h"
#include "spi.h"
#include "display.h"
#include "touch.h"

#define F_CPU 16000000UL // cpu freq
#define MAX_POINTS 25

// FLAGS & VARIABLES
bool is_system_on = false;

volatile bool start_button_pressed;
volatile bool left_button_pressed;
volatile bool right_button_pressed;

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

#define START_BUTTON_PIN         PE4
#define VOLTAGE_TYPE_TOGGLE_PIN  PD3
#define CURSOR_LEFT_PIN          PD0
#define CURSOR_RIGHT_PIN         PD1

#define ISC0                     0
#define ISC1                     1

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
	Timer1_Init(); 					                                                                     // init timer 1 for delay_ms and delay_us
	button_init('E', START_BUTTON_PIN, ISC0, ISC1, 4);                                                   // init start button
	button_init('D', CURSOR_LEFT_PIN, ISC0, ISC1, 0);                                                    // init cursor left button
	button_init('D', CURSOR_RIGHT_PIN, ISC0, ISC1, 1);                                                   // init cursor right button
	button_init('D', VOLTAGE_TYPE_TOGGLE_PIN, ISC0, ISC1, 3);                                            // init toggle voltage type button
	ADC_Init(); 					                                                                     // init the ADC
	SPI_init(); 					                                                                     // init SPI 
	Touch_SPI_Init(); 				                                                                     // init the pins for the touchscreen side (should merge SPI and Touch_SPI)
	Touch_IRQ_Init(); 				                                                                     // init Touch Interrupt pin
	Display_Init(); 				                                                                     // init the display
	
	display_set_rotation(3);                                                                             // landscape inverted
	display_fill_color(COLOR_BLACK);
	
	float voltage_value;
	float max_value = 0;
	float min_value = 5;
	float plot_points[25] = {0};
	int index = 0;
	
	while (1){
		if (start_button_pressed) {                     
			is_system_on = !is_system_on;
			display_fill_color(COLOR_BLACK);
			Toggle_ADC();
			start_button_pressed = false;
		}
		
		if (voltage_button_pressed) {       
			is_high_voltage = !is_high_voltage;
			voltage_button_pressed = false;
		}
		if(is_system_on){		
			draw_voltmeter(is_cursor_on);
			draw_voltage_type(is_high_voltage);
			draw_ui();
			
			if (touch_pending) {
				touch_pending = false;
				
				uint16_t x = touch_spi_transfer(0x90);                                                   // get touchscreen x
				uint16_t y = touch_spi_transfer(0xD0);                                                   // get touchscreen y

				uint8_t which_button = check_touch_buttons(x, y);                                        // check if it is a button press or not
				switch(which_button){                                                                    // do what the button needs to do
					case 1: 
						if(!is_plot_on){                                                                 // if the waveform viewer is not ENABLED but the cursor button was pressed
							draw_cursor_warning();
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
							draw_toggle_warning();
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
			if(!is_holding) {                                                                             // if HOLD is not on, record the values (up to the 25th value)
				if(index < 25){
					plot_points[index] = voltage_value;                                                  // add the voltage values to the end of the array until array is full
					index++;
				}else
					insert_plot_points(plot_points, voltage_value);                                      // insert the points into the plot_points array by disarding the first point (oldest point)
			}
			min_value = plot_points[0];
			max_value = plot_points[0];
			for(int i = 0; i < index; i++){                                                              // get min and max from the recorded points for plot scaling
				if(plot_points[i] < min_value)         
					min_value = plot_points[i];
				if(plot_points[i] > max_value)    
					max_value = plot_points[i];
			}
			
			if(!is_holding & is_plot_on){                                                                 // if HOLD is off and the waveform viewer is enabled, keep updating it when new points are recorded
				if(is_digital_line)
					plot_points_digital(plot_points, index, max_value, min_value);
				else
					plot_points_line(plot_points, index, max_value, min_value);
			}
			
			print_voltage(is_cursor_on, voltage_value);
			print_min_max_voltage(is_plot_on, min_value, max_value);

			if (is_cursor_on) {                                                          
				if (right_button_pressed) {                                                              // move cursor right button
					move_cursor(1);                                                                      // move the cursor to the right one point
					right_button_pressed = false;
				}

				if (left_button_pressed) {                                                               // move cursor left button
					move_cursor(-1);                                                                     // move the cursor to the left one point
					left_button_pressed = false;
				}
				cursor_voltage = get_cursor_voltage();                                                   // get the voltage of the point at the cursor position
				print_cursor_voltage(cursor_voltage);
			}
			draw_indicator_leds(voltage_value);                                                          // draw the voltage level indicator "LEDs"
		}else{
			draw_power_on_screen();
		}
	}
}

