#include "timers.h"

volatile uint32_t millis_counter = 0;


//////////////////////
////    TIMER0    ////
//////////////////////


void Timer0_Init(void){
	TCCR0A = 0x00;                          // reset counter config
	
	TCCR0B &= ~(1 << CS02);                 // clear CS02
	TCCR0B |= (1 << CS01) | (1 << CS00);    // set CS00 & CS01 for 64 prescaler (but this is a bit too slow still, 1.024ms instead of 1 ms flat for ovf => preload the counter)
	
	TCNT0 = 6;                              // preload counter value to get exactly 1ms for ovf
	TIMSK0 |= (1 << TOIE0);                 // enable Timer0 overflow interrupt
}


//////////////////////
////    TIMER1    ////
//////////////////////


void Timer1_Init(void){
	TCCR1A = 0x00;                          // reset counter config
	
	TCCR1B &= ~((1 << CS12) | (1 << CS11)); // clear CS12 and CS11
	TCCR1B |= (1 << CS10);                  // set CS10 for no prescaler
	
	TCNT1 = 0;                              // clear counter value
	TIFR1 |= (1 << OCF1A);                  // clear compare flag
}


void timer_delay_ms(uint16_t ms){
	while (ms--){
		uint16_t ticks = 16000; // 1 ms = 16000 ticks at 16 MHz

		OCR1AH = (ticks >> 8);
		OCR1AL = (ticks & 0xFF);

		TCNT1 = 0;
		TIFR1 |= (1 << OCF1A);

		while (!(TIFR1 & (1 << OCF1A)));
	}
}

void timer_delay_us(uint16_t us){
	uint16_t ticks = us * 16; // 1 us = 16 ticks

	OCR1AH = (ticks >> 8);
	OCR1AL = (ticks & 0xFF);

	TCNT1 = 0;
	TIFR1 |= (1 << OCF1A);

	while (!(TIFR1 & (1 << OCF1A)));
}