#include "adc.h"

bool is_adc_on = false;

void ADC_Init(void){
	ADMUX = 1 << REFS0;                                                 // select avcc & adc0
	ADCSRA = ~(1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // enable adc & set prescaler to 128
	is_adc_on = false;
}

void Toggle_ADC(){
	if(!is_adc_on){            // if the adc is off
		ADCSRA = (1 << ADEN);  // turn it on
		is_adc_on = true;      // update the flag
	}else{                     // otherwise
		ADCSRA = ~(1 << ADEN); // turn it off
		is_adc_on = false;     // and update the flag
	}
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
	
	if(!is_high_voltage){                                                                           // calibration for low voltage mode
		float calibration_offset = 0;                                                               // temp offset for current reading
		float temp_voltage = ADC_GAIN_LOW * (average_value * ADC_STEP_LOW) + ADC_OFFSET_LOW;        // get the current reading in volts 
		if(temp_voltage < 0.8)                                                                      // defined thresholds where calibration is needed 
			calibration_offset = 0.05;
		else if(temp_voltage < 1.2)
			calibration_offset = 0.013;
		else if(temp_voltage < 2.2)
			calibration_offset = 0.005;
		else if(temp_voltage < 3.5)
			calibration_offset = 0.005;
		return ADC_GAIN_LOW * (average_value * ADC_STEP_LOW) + ADC_OFFSET_LOW + calibration_offset; // calculate the true voltage with the calibration offset added
	
	}
	return ADC_GAIN_HIGH * (average_value * ADC_STEP_HIGH) + ADC_OFFSET_HIGH; // return for voltage if we are in high voltage mode
}
