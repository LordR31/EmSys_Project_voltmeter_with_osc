#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>

// ADC
#define BAUD 9600
#define MYUBRR ((F_CPU / (16UL * BAUD)) - 1)
#define MAX_VOLTS 5 // Volts (max value for adc, for AVCC)
#define ADC_MAX_VALUE 1023 // 10 bit adc -> 0 - 1023 range
#define NUM_SAMPLES 10

void ADC_Init(void);
float ADC_measure(void);

#endif /* ADC_H_ */