/*
 * new_firmware.c
 *
 *  Created on: Oct 9, 2022
 *      Author: SHAKO
 */

#include "new_firmware.h"
#include "EEPROM_data.h"
#include "SIM800L.h"
#include "string.h"
#include "stdio.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "rtc.h"

#define CMD_OUT_SIZE 50
#define CMD_SIZE 200

extern UART_HandleTypeDef huart1;
extern uint8_t buffer[MAX_RECEIVE_BUF_SIZE];

general_status Download_New_Firmware(){

	char APN[32] = {};
	char server_address[64] = {};
	char username[32] = {};
	char password[32] = {};
	char file_name[64] = {};
	char file_path[96] = {};
	char local_file_name[64] = {};

	EEPROM_enable();
	read_APN(APN, 0);
	read_server_address(server_address, 0);
	read_username(username, 0);
	read_password(password, 0);
	read_file_name(file_name, 0);
	read_file_path(file_path, 0);
	read_local_file_name(local_file_name, 0);
	EEPROM_disable();

	char output[CMD_OUT_SIZE] = {};
	char cmd[CMD_SIZE] = {};
	if(!SIM800L_Check_Conn()){
		D(printf("SIM800L is not connected to network\n"));
		return SIM800_NOT_CONNECTED_TO_NETWORK;
	}

	D(printf("Connection Passed!\n"));
	if(!SIM800L_Send("AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r", "\r\n", "K\r\n", output, 500)){
		D(printf("GPRS context configuration failed while sending AT cmd\n"));
		return GPRS_CONTEXT_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("GPRS context configuration failed\n"));
		return GPRS_CONTEXT_CONF_SETTING_ERROR;
	}
	D(printf("GPRS context configuration success\n"));
	D(printf("Contype=GPRS\n"));
	strcpy(cmd, "AT+SAPBR=3,1,\"APN\",\"");
	strcat(cmd, APN);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("APN configuration failed while sending AT cmd\n"));
		return APN_CONF_ERROR;
	}

	if(output[0] != 'O'){
		D(printf("APN configuration failed\n"));
		return APN_CONF_SETTING_ERROR;
	}
	D(printf("APN Configuration Successful!\n"));
	char exp_out[5] = {};
	exp_out[0] = 238;
	exp_out[1] = 222;
	strcat(exp_out, "\r\n");
	memset(output, 0, CMD_OUT_SIZE);

	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, "AT+SAPBR=0,1\r");
	HAL_Delay(2000);

	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, "AT+SAPBR=1,1\r");
	HAL_Delay(2000);

	if((buffer[0] != '\r') || (buffer[1] != '\n')){
		D(printf("GPRS context open failed\n"));
		return GPRS_CONTEXT_OPEN_FAIL;
	}
	D(printf("GPRS context open success\n"));
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send("AT+FTPCID=1\r", "\r\n", "K\r\n", output, 500)){
		D(printf("FTPCID fail\n"));
		return FTPCID_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("FTPCID setting failed\n"));
		return FTPCID_CONF_SETTING_ERROR;
	}
	D(printf("\nFTPCID set\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPSERV=\"");
	strcat(cmd, server_address);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPSERV failed while sending AT cmd\n"));
		return FTPSERV_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("FTPSERV setting failed\n"));
		return FTPSERV_CONF_SETTING_ERROR;
	}

	D(printf("Server name entered\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPUN=\"");
	strcat(cmd, username);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPUN failed while sending AT cmd\n"));
		return FTPUN_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("FTPUN setting failed\n"));
		return FTPUN_CONF_SETTING_ERROR;
	}

	D(printf("Username entered\n\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPPW=\"");
	strcat(cmd, password);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPPW failed while sending AT cmd\n"));
		return FTPPW_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("FTPPW setting failed\n"));
		return FTPPW_CONF_SETTING_ERROR;
	}

	D(printf("Password entered\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPGETNAME=\"");
	strcat(cmd, file_name);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPGETNAME failed while sending AT cmd\n"));
		return FTPGETNAME_CONF_ERROR;
	}
	if(output[0] != 'O'){
		D(printf("FTPGETNAME setting failed\n"));
		return FTPGETNAME_CONF_SETTING_ERROR;
	}

	D(printf("File name entered\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPGETPATH=\"");
	strcat(cmd, file_path);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPGETPATH failed while sending AT cmd\n"));
		return FTPGETPATH_CONF_ERROR;
	}

	if(output[0] != 'O'){
		D(printf("FTPGETPATH setting failed\n"));
		return FTPGETPATH_CONF_SETTING_ERROR;
	}

	D(printf("File path entered\n"));
	memset(cmd, 0, CMD_SIZE);
	strcpy(cmd, "AT+FTPGETTOFS=0,\"");
	strcat(cmd, local_file_name);
	strcat(cmd, "\"\r");
	memset(output, 0, CMD_OUT_SIZE);

	if(!SIM800L_Send(cmd, "\r\n", "K\r\n", output, 500)){
		D(printf("FTPGETTOFS failed while sending AT cmd\n"));
		return FTPGETTOFS_CONF_ERROR;
	}

	if(output[0] != 'O'){
		D(printf("FTPGETTOFS setting failed\n"));
		return FTPGETTOFS_CONF_SETTING_ERROR;
	}
	D(printf("Local file name entered\n"));
	D(printf("Downloading...\n"));
	memset(output, 0, CMD_OUT_SIZE);
	if(!SIM800L_WAIT_FOR("\r\n+FTPGETTOFS: 0,", "\r\n", output, 180, 1000)){
		D(printf("Download timeout\n"));
		return FIRMWARE_DOWNLOAD_TIMEOUT_ERROR;
	}
	D(printf("Download Finished with output: %d\n", (int)string_to_number(output)));

	memset(buffer, 0, MAX_RECEIVE_BUF_SIZE);
	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, "AT+SAPBR=0,1\r");

	if(!SIM800L_WAIT_FOR("\r\nOK", "", output, 10, 1000)){
		D(printf("GPRS context close timeout\n"));
		return GPRS_CONTEXT_CLOSE_TIMEOUT_ERROR;
	}
	D(printf("New file is saved in file system, GPRS context is closed\n"));
	return OK;
}


void jump_to(const uint32_t addr) {
	const vector_t *vector_p = (vector_t*) addr;

	//peripheral deinit
	HAL_RTC_DeInit(&hrtc);
	HAL_UART_DeInit(&huart3);
	HAL_UART_DeInit(&huart2);
	HAL_UART_DeInit(&huart1);
	HAL_I2C_DeInit(&hi2c1);
	HAL_ADC_DeInit(&hadc1);
	HAL_DMA_DeInit(huart1.hdmarx);
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_All);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_All);
	HAL_GPIO_DeInit(GPIOD, GPIO_PIN_All);
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_All);
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();

	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	/* Disable interrupt */
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICPR[0] = 0xFFFFFFFF;
#if defined(__NRF_NVIC_ISER_COUNT) && __NRF_NVIC_ISER_COUNT == 2
	NVIC->ICER[1]=0xFFFFFFFF;
	NVIC->ICPR[1]=0xFFFFFFFF;
#endif

	/* Set new vector table */
	SCB->VTOR = (uint32_t) addr;

	/* Jump, used asm to avoid stack optimization */
	asm("\n\
        msr msp, %0; \n\
        bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
}
