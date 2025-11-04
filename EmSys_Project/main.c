#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "adc.h"
#include "buttons.h"
#include "timers.h"
#include "spi.h"
#include "display.h"
#include "touch.h"
#include "globals.h"

ISR(INT4_vect) {
	start_button_pressed = true;
}

ISR(INT3_vect){
	voltage_button_pressed = true;
}

ISR(INT1_vect){
	right_button_pressed = true;
}

ISR(INT0_vect){
	left_button_pressed = true;
}

ISR(INT2_vect) {
	touch_pending = true;
}

int main(void){
	sei();                                                                                               // enable interrupts                                                               
	Timer1_Init(); 					                                                                     // init timer 1 for delay_ms and delay_us
	button_init('E', START_BUTTON_PIN, ISC0, ISC1, EXT_INT4);                                            // init start button
	button_init('D', CURSOR_LEFT_PIN, ISC0, ISC1, EXT_INT0);                                             // init cursor left button
	button_init('D', CURSOR_RIGHT_PIN, ISC0, ISC1, EXT_INT1);                                            // init cursor right button
	button_init('D', VOLTAGE_TYPE_TOGGLE_PIN, ISC0, ISC1, EXT_INT3);                                     // init toggle voltage type button
	ADC_Init(); 					                                                                     // init the ADC
	SPI_init(); 					                                                                     // init SPI 
	Touch_SPI_Init(); 				                                                                     // init the pins for the touchscreen side (should merge SPI and Touch_SPI)
	Touch_IRQ_Init(); 				                                                                     // init Touch Interrupt pin
	Display_Init(); 				                                                                     // init the display
	
	display_set_rotation(3);                                                                             // landscape inverted
	display_fill_color(COLOR_BLACK);
	
	while (1){
		if (start_button_pressed) {                     
			is_system_on = !is_system_on;
			display_fill_color(COLOR_BLACK);
			set_ADC_state(is_system_on);
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
				
				uint16_t x = touch_spi_transfer(GET_X_COMMAND);                                                   // get touchscreen x
				uint16_t y = touch_spi_transfer(GET_Y_COMMAND);                                                   // get touchscreen y

				uint8_t which_button = check_touch_buttons(x, y);                                        // check if it is a button press or not
				execute_button_command(which_button);                                                    // execute the touch button command
			}
			
			voltage_value = ADC_measure(is_high_voltage);                                                // get voltage value from adc, based on voltage type (high or low)
			if(!is_holding) {                                                                            // if HOLD is not on, record the values (up to the 25th value)
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
			draw_indicator_leds(voltage_value, is_high_voltage);                                                          // draw the voltage level indicator "LEDs"
		}else{
			draw_power_on_screen();
		}
	}
}

