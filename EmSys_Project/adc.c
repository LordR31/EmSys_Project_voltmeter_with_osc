#include "adc.h"

////////////////////
////    ADC0    ////
////////////////////


void ADC_Init(void){
	ADMUX = 1 << REFS0;                                                // select avcc & adc0
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable adc & set prescaler to 128
}

float ADC_measure(void){
	uint32_t adc_sum = 0;
	for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
		ADCSRA |= (1 << ADSC);                                    // start ADC
		while (ADCSRA & (1 << ADSC));                             // wait for conversion
		adc_sum += (ADCL | (ADCH << 8));                          // add result
		timer_delay_ms(1);                                        // small delay between samples
	}
	float average_value = adc_sum / NUM_SAMPLES;                  // get the average
	return ((float)(average_value) * MAX_VOLTS) / ADC_MAX_VALUE;  // convert value and return it

}