#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <stdbool.h>

// ADC
#define ADC_MAX_VALUE 1023 // 10 bit adc -> 0 - 1023 range
#define NUM_SAMPLES 10

#define ADC_STEP_LOW 4.760 / 1023.0
#define ADC_GAIN_LOW 1.05
#define ADC_OFFSET_LOW 0.009

#define ADC_STEP_HIGH 0.024
#define ADC_GAIN_HIGH 1
#define ADC_OFFSET_HIGH 0.024

void ADC_Init(void);
float ADC_measure(bool is_high_voltage);
#endif /* ADC_H_ */