#include "spi.h"
#include "touch.h"

void Touch_IRQ_Init(void){
	DDRD &= ~(1 << TOUCH_IRQ_PIN);                       // PD2 as input
	PORTD |= (1 << TOUCH_IRQ_PIN);                       // Pull-up ON
	
	EICRA &= ~(1 << ISC20);                              // clear ISC20
	EICRA |= (1 << ISC21);                               // set ISC21 (10 falling edge)
	EIMSK |= (1 << INT2);                                // Enable INT2
}

uint16_t touch_spi_transfer(uint8_t command) {
	CS_TOUCH_LOW();                                      // set Touch Chip Select low to signal we talk to it

	SPI_transfer(command);                               // discard first response (garbage)
	uint8_t high = SPI_transfer(0x00);                   // get the high byte
	uint8_t low  = SPI_transfer(0x00);                   // get the low byte

	CS_TOUCH_HIGH();                                     // set  Touch Chip Select high to signal we stopped talking to it

	return ((high << 8) | low) >> 3;                     // return the response, shift by 3 to the right because XPT2046 returns 12 bytes, not 16 (it's a 12bit ADC)
}

uint16_t read_touch_x(void) {                            // function to get the x coord
	return touch_spi_transfer(GET_X_COMMAND);
}

uint16_t read_touch_y(void) {                            // function to get the y coord
	return touch_spi_transfer(GET_Y_COMMAND);
}

uint8_t check_touch_buttons(uint16_t x, uint16_t y){     // function to if the touch was accidental or on purpose (it was a button press) 
	if(x >= BUTTON_X_START & x <= BUTTON_X_END)          // if we are within the limits of the button menu for X
		if(y >= CURSOR_Y_TOP & y < HOLD_Y_TOP)           // then, if we are within the vertical limits of the Cursor Button
			return CURSOR_BUTTON;                        // return that it was a Cursor Button press
		else if(y >= HOLD_Y_TOP & y < WAVE_Y_TOP)        // otherwise, if we are within the vertical limits of the Hold Button
			return HOLD_BUTTON;                          // return that it was a Hold Button press
		else if(y >= WAVE_Y_TOP & y < TOGGLE_Y_TOP)      // otherwise, if we are within the vertical limits of the Waveform Viewer Button
			return WAVE_BUTTON;                          // return that it was a Waveform Viewer Button press
		else if(y >= TOGGLE_Y_TOP & y < TOGGLE_Y_BOTTOM) // otherwise, if we are within the vertical limits of the Toggle Waveform Type Button
			return TOGGLE_BUTTON;                        // return that it was a Toggle Waveform Type Button press
	
	return 0;                                            // if we aren't within the menu limits, return 0 (no button press / accidental press)
}
