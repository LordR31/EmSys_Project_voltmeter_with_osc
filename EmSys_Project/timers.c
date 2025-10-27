#include "timers.h"

void Timer1_Init(void){
	TCCR1A = 0x00;                          // reset counter config
	
	TCCR1B &= ~((1 << CS12) | (1 << CS11)); // clear CS12 and CS11
	TCCR1B |= (1 << CS10);                  // set CS10 for no prescaler
	
	TCNT1 = 0;                              // clear counter value
	TIFR1 |= (1 << OCF1A);                  // clear compare flag
}


void timer_delay_ms(uint16_t ms){
	while (ms--){                           // until the user ms is waited / counted:
		uint16_t ticks = 16000;             // 1 ms = 16000 ticks at 16 MHz

		OCR1AH = (ticks >> 8);              // load the value inside OCR
		OCR1AL = (ticks & 0xFF);

		TCNT1 = 0;                          // reset the counter value
		TIFR1 |= (1 << OCF1A);

		while (!(TIFR1 & (1 << OCF1A)));    // wait for it to count
	}
}

void timer_delay_us(uint16_t us){           // until the user us is waited / counted:
	uint16_t ticks = us * 16;               // 1 us = 16 ticks

	OCR1AH = (ticks >> 8);                  // load the value inside OCR
	OCR1AL = (ticks & 0xFF);

	TCNT1 = 0;                              // reset the counter value
	TIFR1 |= (1 << OCF1A);

	while (!(TIFR1 & (1 << OCF1A)));        // wait for it to count
}