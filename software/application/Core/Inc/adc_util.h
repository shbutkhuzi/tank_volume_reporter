/*
 * adc_util.h
 *
 *  Created on: Jun 25, 2023
 *      Author: SHAKO
 */

#ifndef INC_ADC_UTIL_H_
#define INC_ADC_UTIL_H_

#include "main.h"
#include "adc.h"


#define LIPO_FACTOR 				1.297495
#define SOLAR_FACTOR 				2.415246

#define V25 						1.4
#define AVG_SLOPE 					0.004478


struct adcMeas{
	float lipo;
	float solar;
	float temp;
};



void ADC_Select_CHTemp (void);
void ADC_Select_CH1 (void);
void ADC_Select_CH4 (void);
general_status getADCMeasurements(struct adcMeas* meas);


#endif /* INC_ADC_UTIL_H_ */
