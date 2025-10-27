#include "buttons.h"

void button_init(char port, int pin, int isc0, int isc1, int interrupt){
	switch (port){
	case 'E':
		DDRE &= (1 << pin);                     // set pin as input
		PORTE |= (1 << pin);                    // set pull-up resistor
		EICRB &= ((1 << pin + 1) | (1 << pin)); // clear both ISC regs
		if(isc0)                                // set ISCnn
			EICRB |= (1 << pin);
		if(isc1)
			EICRB |= (1 << pin + 1);
		break;
	case 'D':
		DDRD &= (1 << pin);
		PORTD |= (1 << pin);
		EICRA &= ~((1 << pin + 1) | (1 << pin)); // clear regs for ISC
		if(isc0)                                // set ISCnn
			EICRA |= (1 << pin);
		if(isc1)
			EICRA |= (1 << pin + 1);
		break;
	}
	EIMSK |= (1 << interrupt);              // enable interrupt
}