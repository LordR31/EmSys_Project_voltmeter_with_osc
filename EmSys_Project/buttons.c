#include "buttons.h"

volatile bool button_pressed = false;

void start_button_init(void){
	DDRE &= ~(1 << PE4);                     // set button as input
	PORTE |= (1 << PE4);                     // pull-up resistor
	EICRB &= ~((1 << ISC41) | (1 << ISC40)); // clear both regs
	EICRB |= (1 << ISC41);                   // set ISC40 (01)

	EIMSK |= (1 << INT4);                    // interrupt mask reg
}

