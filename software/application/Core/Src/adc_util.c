/*
 * adc_util.c
 *
 *  Created on: Jun 25, 2023
 *      Author: SHAKO
 */


#include "adc_util.h"


void ADC_Select_CH4 (void)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	  */
	sConfig.Channel = ADC_CHANNEL_4;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	  Error_Handler();
	}
}

void ADC_Select_CH1 (void)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	  */
	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	  Error_Handler();
	}
}

void ADC_Select_CHTemp (void)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	  */
	sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	  Error_Handler();
	}
}



general_status getADCMeasurements(struct adcMeas* meas){

	HAL_GPIO_WritePin(SOLAR_ADC_EN_GPIO_Port, SOLAR_ADC_EN_Pin, 1);
	HAL_GPIO_WritePin(LIPO_ADC_EN_GPIO_Port, LIPO_ADC_EN_Pin, 1);
	HAL_Delay(5);

	uint32_t ADC_VAL[3] = {};

	ADC_VAL[0] = 0;
	ADC_VAL[1] = 0;
	ADC_VAL[2] = 0;

	for(uint8_t i = 0; i < 10 ; i++){
		ADC_Select_CH1();
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1000);
		ADC_VAL[0] += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);

		ADC_Select_CH4();
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1000);
		ADC_VAL[1] += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);

		ADC_Select_CHTemp();
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1000);
		ADC_VAL[2] += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
	}
	ADC_VAL[0] = ADC_VAL[0]/10.0;
	ADC_VAL[1] = ADC_VAL[1]/10.0;
	ADC_VAL[2] = ADC_VAL[2]/10.0;

	meas->temp = ((V25 - 3.325*ADC_VAL[2]/4095.0 )/AVG_SLOPE)+25;
	meas->lipo = (((ADC_VAL[0]+69)*3.306/4095.0)) * LIPO_FACTOR + 0.02;
	meas->solar = (((ADC_VAL[1]+69)*3.306/4095.0)) * SOLAR_FACTOR + 0.011;

//	D(printf("ADC:  %8lu          %8lu           %8lu\n", ADC_VAL[2], ADC_VAL[0], ADC_VAL[1]));
	D(printf("temp: %8.4f  lipo: %8.4f  solar: %8.4f\n", meas->temp, meas->lipo, meas->solar));

	HAL_GPIO_WritePin(SOLAR_ADC_EN_GPIO_Port, SOLAR_ADC_EN_Pin, 0);
	HAL_GPIO_WritePin(LIPO_ADC_EN_GPIO_Port, LIPO_ADC_EN_Pin, 0);

	return OK;
}
