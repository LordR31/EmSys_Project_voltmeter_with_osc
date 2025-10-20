#include "adc.h"

void ADC_Init(void){
	ADMUX = 1 << REFS0;                                                // select avcc & adc0
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable adc & set prescaler to 128
}

float ADC_measure(bool is_high_voltage){
	uint32_t adc_sum = 0;
	for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
		ADCSRA |= (1 << ADSC);                                    // start ADC
		while (ADCSRA & (1 << ADSC));                             // wait for conversion
		adc_sum += (ADCL | (ADCH << 8));                          // add result
		timer_delay_ms(1);                                        // small delay between samples
	}
	float average_value = adc_sum / NUM_SAMPLES;                  // get the average
	if(average_value == 0)
		return 0;
	
	if(!is_high_voltage){
		float calibration_offset = 0;
		float temp_voltage = ADC_GAIN_LOW * (average_value * ADC_STEP_LOW) + ADC_OFFSET_LOW; 
		if(temp_voltage < 0.8)
			calibration_offset = 0.01;
		else if(temp_voltage < 1.2)
			calibration_offset = 0.006;
		else if(temp_voltage < 2.2)
			calibration_offset = 0.005;
		
		return ADC_GAIN_LOW * (average_value * ADC_STEP_LOW) + ADC_OFFSET_LOW + calibration_offset; 
	
	}
	return ADC_GAIN_HIGH * (average_value * ADC_STEP_HIGH) + ADC_OFFSET_HIGH;
}
