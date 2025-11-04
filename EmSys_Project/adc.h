#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <stdbool.h>

// ADC
#define ADC_MAX_VALUE 1023 // 10 bit adc -> 0 - 1023 range -> 1024 values
#define NUM_SAMPLES 10

#define ADC_STEP_LOW 4.810 / 1024.0
#define ADC_GAIN_LOW 1.04
#define ADC_OFFSET_LOW 0.014

#define ADC_STEP_HIGH 0.024
#define ADC_GAIN_HIGH 1
#define ADC_OFFSET_HIGH 0.024

void ADC_Init(void);
void set_ADC_state(bool is_system_on);
float ADC_measure(bool is_high_voltage);
#endif /* ADC_H_ */