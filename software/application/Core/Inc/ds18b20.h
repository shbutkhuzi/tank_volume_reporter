/*
 * ds18b20.h
 *
 *  Created on: Jun 25, 2023
 *      Author: SHAKO
 */

#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "main.h"
#include "tim.h"


#define MAX_CONSECUTIVE_MEAS_DISCARDS					5


void delay_us (uint16_t us);
void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint8_t DS18B20_Start (void);
void DS18B20_Write (uint8_t data);
uint8_t DS18B20_Read (void);
general_status getTempDS18B20(float *Temperature);


#endif /* INC_DS18B20_H_ */
