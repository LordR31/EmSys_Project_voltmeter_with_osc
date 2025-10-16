#ifndef TIMERS_H_
#define TIMERS_H_

#include <avr/io.h>

void Timer0_Init(void);
void Timer1_Init(void);
void timer_delay_ms(uint16_t ms);
void timer_delay_us(uint16_t us);

#endif /* TIMERS_H_ */