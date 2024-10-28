/*
 * ds18b20.c
 *
 *  Created on: Jun 25, 2023
 *      Author: SHAKO
 */

#include "ds18b20.h"


float tank_temp;
uint8_t ds18_first_meas_flag = 1;
uint8_t consecutive_discards = 0;
uint8_t consecutive_discard_flag = 0;
uint8_t ds18b20_precense_flag = 0;


void delay_us (uint16_t us)
{
	__HAL_TIM_SET_COUNTER(&htim4,0);  // set the counter value a 0
	while (__HAL_TIM_GET_COUNTER(&htim4) < us);  // wait for the counter to reach the us input in the parameter
}

void Set_Pin_Output (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

void Set_Pin_Input (GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}


uint8_t DS18B20_Start()
{
	uint8_t Response = 0;
	Set_Pin_Output(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);   // set the pin as output
	HAL_GPIO_WritePin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin, 0);  // pull the pin low
	delay_us (480);   // delay according to datasheet

	Set_Pin_Input(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);    // set the pin as input
	delay_us (80);    // delay according to datasheet

	if (!(HAL_GPIO_ReadPin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin))) Response = 1;    // if the pin is low i.e the presence pulse is detected
	else Response = 0;

	delay_us (400); // 480 us delay totally.

	return Response;
}



void DS18B20_Write (uint8_t data)
{
//	Set_Pin_Output(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);  // set as output

	for (int i=0; i<8; i++)
	{

		if ((data & (1<<i))!=0)  // if the bit is high
		{
			// write 1

			Set_Pin_Output(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);  // set as output
			HAL_GPIO_WritePin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin, 0);  // pull the pin LOW
			delay_us (3);  // wait for 1 us

			Set_Pin_Input(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);  // set as input
			delay_us (50);  // wait for 60 us
		}

		else  // if the bit is low
		{
			// write 0

			Set_Pin_Output(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);
			HAL_GPIO_WritePin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin, 0);  // pull the pin LOW
			delay_us (50);  // wait for 60 us

			Set_Pin_Input(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);
			delay_us (3);
		}
	}
}


uint8_t DS18B20_Read()
{
	uint8_t value=0;
//	Set_Pin_Input(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);

	for (int i=0;i<8;i++)
	{
		Set_Pin_Output(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);   // set as output

		HAL_GPIO_WritePin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin, 0);  // pull the data pin LOW
		delay_us (2);  // wait for 2 us

		Set_Pin_Input(DS18B20_TX_GPIO_Port, DS18B20_TX_Pin);  // set as input
//		delay_us (2);
		if (HAL_GPIO_ReadPin (DS18B20_TX_GPIO_Port, DS18B20_TX_Pin))  // if the pin is HIGH
		{
			value |= 1<<i;  // read = 1
		}
		delay_us (50);  // wait for 60 us
	}
	return value;
}


general_status getTempDS18B20(float *Temperature){
	uint8_t Temp_byte1, Temp_byte2;
	int16_t TEMP;


	HAL_GPIO_WritePin(DS18_CONTROL_GPIO_Port, DS18_CONTROL_Pin, 0);
	HAL_Delay(3);

	if(!DS18B20_Start()){
		return DS18B20_NO_RESPONSE;
	}
	HAL_Delay (1);
	DS18B20_Write (0xCC);  // skip ROM
	DS18B20_Write (0x44);  // convert t
	HAL_Delay (800);

	if(!DS18B20_Start()){
		return DS18B20_NO_RESPONSE;
	}
	HAL_Delay(1);
	DS18B20_Write (0xCC);  // skip ROM
	DS18B20_Write (0xBE);  // Read Scratch-pad

	Temp_byte1 = DS18B20_Read();
	Temp_byte2 = DS18B20_Read();

	TEMP = ((Temp_byte2<<8) | Temp_byte1);
	*Temperature = ((float) (TEMP/16.0) + *Temperature) / 2.0;

	HAL_GPIO_WritePin(DS18_CONTROL_GPIO_Port, DS18_CONTROL_Pin, 1);
	if((*Temperature < -55) || (*Temperature > 125)){
		D(printf("Invalid temperature from DS18B20"));
		return DS18B20_INVALID_TEMP;
	}
	D(printf("DS18B20 Temperature: %f\n", *Temperature));

	return OK;
}

