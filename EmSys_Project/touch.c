#include "spi.h"
#include "touch.h"

void Touch_IRQ_Init(void){
	DDRD &= ~(1 << PD2);     // PD2 as input
	PORTD |= (1 << PD2);  // Pull-up ON
	
	EICRA |= (1 << ISC21);   // ISC2 = 10 ? falling edge
	EICRA &= ~(1 << ISC20);
	EIMSK |= (1 << INT2);    // Enable INT2
}

uint16_t touch_spi_transfer(uint8_t command) {
	CS_TOUCH_LOW();

	SPI_transfer(command);           // discard first response
	uint8_t high = SPI_transfer(0x00);
	uint8_t low  = SPI_transfer(0x00);

	CS_TOUCH_HIGH();

	return ((high << 8) | low) >> 3;
}

uint16_t read_touch_x(void) {
	return touch_spi_transfer(0x90);
}

uint16_t read_touch_y(void) {
	return touch_spi_transfer(0xD0);
}


uint8_t check_touch_buttons(uint16_t x, uint16_t y){
	if(x >= BUTTON_X_START & x <= BUTTON_X_END)
		if(y >= CURSOR_Y_TOP & y < HOLD_Y_TOP)
			return CURSOR_BUTTON;
		else if(y >= HOLD_Y_TOP & y < WAVE_Y_TOP)
			return HOLD_BUTTON;
		else if(y >= WAVE_Y_TOP & y < TOGGLE_Y_TOP)
			return WAVE_BUTTON;
		else if(y >= TOGGLE_Y_TOP & y < TOGGLE_Y_BOTTOM)
			return TOGGLE_BUTTON;
	
	return 0;
}
