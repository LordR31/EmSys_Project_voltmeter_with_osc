#include "buttons.h"

extern bool start_button_pressed;
extern bool voltage_button_pressed;
extern bool right_button_pressed;
extern bool left_button_pressed;

void button_init(char port, int pin, int isc0, int isc1, int interrupt){
	switch (port){
	case 'E':
		DDRE &= ~(1 << pin);                    // set pin as input
		PORTE |= (1 << pin);                    // set pull-up resistor
		EICRB &= ~((1 << (2 * interrupt + 1)) | (1 << (2 * interrupt))); // clear both ISC regs
		if (isc0)
			EICRB |= (1 << (2 * interrupt));
		if (isc1)
			EICRB |= (1 << (2 * interrupt + 1));
		break;
	case 'D':
		DDRD &= ~(1 << pin);
		PORTD |= (1 << pin);
		EICRA &= ~((1 << (2 * interrupt + 1)) | (1 << (2 * interrupt))); // clear regs for ISC
		if (isc0)
			EICRA |= (1 << (2 * interrupt));
		if (isc1)
			EICRA |= (1 << (2 * interrupt + 1));
		break;
	}
	EIMSK |= (1 << interrupt);                  // enable interrupt
}