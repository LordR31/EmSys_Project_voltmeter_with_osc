#include "buttons.h"

void start_button_init(void){
	DDRE &= ~(1 << PE4);                     // set button as input
	PORTE |= (1 << PE4);                     // pull-up resistor
	EICRB &= ~((1 << ISC41) | (1 << ISC40)); // clear both regs
	EICRB |= (1 << ISC41);                   // set ISC40 (10) Falling Edge
	EIMSK |= (1 << INT4);                    // enable INT4
}

void cursor_buttons_init(void){
	DDRD &= (1 << PD0);                      // go left button
	DDRD &= (1 << PD1);                      // go right button
	PORTD |= (1 << PD0) | (1 << PD1);        // pull-up resistors
	EICRA &= ~((1 << ISC01) | (1 << ISC00)); // clear regs for INT 0
	EICRA &= ~((1 << ISC11) | (1 << ISC10)); // clear regs for INT 1
	
	EICRA |= (1 << ISC01) | (1 << ISC11);    // (10) Falling Edge
	EIMSK |= (1 << INT1) | (1 << INT0);      // enable INT1 & INT0
}


void toggle_votalge_button_init(void){
	DDRD &= (1 << PD3);                      // toggle voltage type button (high or low)
	PORTD |= (1 << PD3);                     // enable pull-up resistor
	EICRA &= ~((1 << ISC31) | (1 << ISC30)); // clear EICRA reg for PD3
	EICRA |= (1 << ISC31);                   // (10) Falling Edge
	EIMSK |= (1 << INT3);                    // enable INT3
}
