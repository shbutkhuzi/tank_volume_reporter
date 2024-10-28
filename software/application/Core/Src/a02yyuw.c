/*
 * a02yyuw.c
 *
 *  Created on: Jun 26, 2023
 *      Author: SHAKO
 */

#include "a02yyuw.h"
#include "math.h"
#include "predict.h"
#include "ds18b20.h"


float curr_volume;
uint16_t a02yyuw_measurement = 0;
extern volatile timeDist_obj accum_data;
extern uint8_t ds18_first_meas_flag;
extern uint8_t consecutive_discards;
extern uint8_t consecutive_discard_flag;

extern volatile float cylinder_height;
extern volatile float cylinder_radius;
extern volatile float dome_height;

extern uint8_t ds18b20_precense_flag;


general_status getMeasA02(uint16_t *distance){

	uint32_t tickstart;
	uint8_t buffer_a02[A02YYUW_RX_SIZE];

	HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 0);
	HAL_Delay(600);  // allow sensor to power up

	tickstart = HAL_GetTick();
	while(1){
		if(HAL_UART_Receive(&huart2, buffer_a02, 1, 300) != HAL_OK){
			D(printf("Error in reception of data\n"));
			continue;
		}
		if(buffer_a02[0] == 0xFF){
			HAL_UART_Receive(&huart2, buffer_a02+1, 1, 300);
			HAL_UART_Receive(&huart2, buffer_a02+2, 1, 300);
			HAL_UART_Receive(&huart2, buffer_a02+3, 1, 300);

			if((uint8_t)(buffer_a02[0] + buffer_a02[1] + buffer_a02[2]) == buffer_a02[3]){
				*distance = (buffer_a02[1] << 8) | buffer_a02[2];
//				D(printf("Distance: %d mm\n", *distance));
				break;
			}
		}
		if(HAL_GetTick() - tickstart > 1000){
			return A02YYUW_TIMEOUT;
		}
	}

	HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 1);

	return OK;
}



general_status getWaterVolume(){
	uint16_t distance = accum_data.timeDist[accum_data.head].distance;

	curr_volume = getVolumeFromDistance(distance);

	return OK;
}


float getVolumeFromDistance(uint16_t distance){

	float vol = 0;
	float dist_meter = distance/1000.0;

	float cylinder_volume = M_PI*cylinder_radius*cylinder_radius*cylinder_height;
	float dome_sphere_radius = dome_height/2.0 + (cylinder_radius*cylinder_radius)/(2.0*dome_height);

//	((M_PI/3)*pow(dist_meter, 3)) - (2.15151*M_PI*pow(dist_meter, 2)) - (0.381849*dist_meter) + 30;

	if(dist_meter > dome_height){
		vol = cylinder_volume - M_PI*cylinder_radius*cylinder_radius * (float)(dist_meter - dome_height);
	}else{
		vol = cylinder_volume + (-M_PI/3)*(dome_height*dome_height*dome_height - dist_meter*dist_meter*dist_meter)
						      + (M_PI*dome_sphere_radius)*(dome_height*dome_height - dist_meter);
	}

	vol = vol * 4;

	return vol * 1000;
}


general_status combined_meas(uint16_t *distance, float *temp, uint8_t ds18b20_en){

	uint32_t tickstart;
	uint8_t buffer_a02[A02YYUW_RX_SIZE];

	uint8_t Temp_byte1, Temp_byte2;
	int16_t TEMP;
	float curr_ds18_meas;

	uint8_t trials = 0;

	// allow sensor to power up while ds18b20 measurements are taken
	HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 0);

	if(ds18b20_en){
		HAL_GPIO_WritePin(DS18_CONTROL_GPIO_Port, DS18_CONTROL_Pin, 0);
		HAL_Delay(3);

		if(!DS18B20_Start()){
//			return DS18B20_NO_RESPONSE;
			ds18b20_precense_flag = 0;
			goto wait;
		}
		HAL_Delay (1);
		DS18B20_Write (0xCC);  // skip ROM
		DS18B20_Write (0x44);  // convert t
	}

	wait:

	HAL_Delay(800);

	if(ds18b20_en){
		if(!DS18B20_Start()){
//			return DS18B20_NO_RESPONSE;
			ds18b20_precense_flag = 0;
			goto a02yyuw;
		}
		HAL_Delay(1);
		DS18B20_Write (0xCC);  // skip ROM
		DS18B20_Write (0xBE);  // Read Scratch-pad

		Temp_byte1 = DS18B20_Read();
		Temp_byte2 = DS18B20_Read();

		TEMP = ((Temp_byte2<<8) | Temp_byte1);
		curr_ds18_meas = (float) (TEMP/16.0);

		if(consecutive_discards >= MAX_CONSECUTIVE_MEAS_DISCARDS){
			ds18_first_meas_flag = 1;
			consecutive_discard_flag = 0;
			consecutive_discards = 0;
		}

		if(ds18_first_meas_flag){
			*temp = curr_ds18_meas;
			ds18_first_meas_flag = 0;
		}else{
			// discard new measurement if it is too far from previous measurement
			if(fabsf(curr_ds18_meas - *temp) < 2.0){
				*temp = (curr_ds18_meas + *temp) / 2.0;
				D(printf("DS18B20 pure temperature: %f  -----> accepted\n", curr_ds18_meas));

				consecutive_discard_flag = 0;
				consecutive_discards = 0;
			}else{
				consecutive_discard_flag = 1;
				consecutive_discards += 1;
				D(printf("DS18B20 pure temperature: %f  -----> discarded\n", curr_ds18_meas));
			}

		}


		HAL_GPIO_WritePin(DS18_CONTROL_GPIO_Port, DS18_CONTROL_Pin, 1);
		if((*temp < -55) || (*temp > 125)){
			D(printf("Invalid temperature from DS18B20\n"));
//			return DS18B20_INVALID_TEMP;
			goto a02yyuw;
		}
		D(printf("DS18B20 Temperature: %f\n", *temp));
		ds18b20_precense_flag = 1;
	}

	a02yyuw:

	tickstart = HAL_GetTick();
	while(1){
		if(HAL_UART_Receive(&huart2, buffer_a02, 1, 300) != HAL_OK){
			D(printf("Error in reception of data\n"));
			if(trials >= A02YYUW_MAX_MEAS_TRIALS){
				return A02YYUW_ERROR_READING_MEASUREMENT;
			}
			trials += 1;
			continue;
		}
		if(buffer_a02[0] == 0xFF){
			HAL_UART_Receive(&huart2, buffer_a02+1, 1, 300);
			HAL_UART_Receive(&huart2, buffer_a02+2, 1, 300);
			HAL_UART_Receive(&huart2, buffer_a02+3, 1, 300);

			if((uint8_t)(buffer_a02[0] + buffer_a02[1] + buffer_a02[2]) == buffer_a02[3]){
				*distance = (buffer_a02[1] << 8) | buffer_a02[2];
				a02yyuw_measurement = *distance;
//				D(printf("Distance: %d mm\n", *distance));
				break;
			}
		}
		if(HAL_GetTick() - tickstart > 1000){
			HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 1);
			return A02YYUW_TIMEOUT;
		}
	}

	HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 1);

	return OK;

}

