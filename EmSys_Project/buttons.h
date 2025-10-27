#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <avr/io.h>
#include <stdbool.h>

void button_init(char port, int pin, int isc0, int isc1, int interrupt);

#endif /* BUTTONS_H_ */