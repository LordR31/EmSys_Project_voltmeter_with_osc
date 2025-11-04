#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <avr/io.h>
#include <stdbool.h>

void button_init(char port, int pin, int isc0, int isc1, int interrupt);
ISR(INT0_vect);
ISR(INT1_vect);
ISR(INT2_vect);
ISR(INT3_vect);
#endif /* BUTTONS_H_ */