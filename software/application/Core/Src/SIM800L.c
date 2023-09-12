/*
 * SIM800L.c
 *
 *  Created on: Aug 18, 2022
 *      Author: shako
 */

#include "SIM800L.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "usart.h"
#include "dma.h"
#include "EEPROM_data.h"
#include "rtc.h"
#include "time_my.h"
#include "time.h"

extern UART_HandleTypeDef huart1;
uint8_t buffer[MAX_RECEIVE_BUF_SIZE];

extern RTC_HandleTypeDef hrtc;

extern char mobile_country_code[5];



general_status SIM800L_init(void){

	struct tm localTime;
	general_status ret;
	char output[MAX_OUTPUT_SIZE] = {};


	D(printf("Initializing SIM800L...\n"));
	HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 1);

	HAL_Delay(2000);

	init:
	SIM800L_RESTART();

	while(SIM800L_Test_AT() != OK){
		SIM800L_RESTART();
	}

	if(SIM800L_Send("ATE0+CMEE=2;+CFUN=1;+CMGF=1;+CIURC=0;+CSCS=\"IRA\";+IPR=115200;+GSMBUSY=1;&W\r", "\r\n", "\r\n", output, 200)){
		memset(output, 0, MAX_OUTPUT_SIZE);
		if(SIM800L_WAIT_FOR("\r\nOK", "\r\n", output, 30, 500) != 1){
			D(printf("Setting parameters failed\n"));
			goto init;
		}else{
			D(printf("Initial parameters set successfully\n"));
		}
	}else{
		D(printf("Failed to set initial parameters\n"));
	}


	// synchronizing time if not already synchronized
	init_time:

	ret = SIM800L_GET_LOCAL_TIME(&localTime);
	if(ret == OK){
		if(localTime.tm_year+1900 < 2023){
			D(printf("Time is not synchronized\n"));
			if(SIM800L_Send("AT+CLTS=1;&W\r", "\r\nO", "K\r\n", output, 200)){
				goto init;
			}else{
				HAL_Delay(500);
				goto init_time;
			}
		}
	}else D(printf("Error getting current time during initialization: Error no: %d\n", ret));


	syncSTMandSIM800time();

	SIM800L_RECONNECT();

	// set SIM800 clock to mode=1 to enable power saving
	if(SIM800L_Send("AT+CSCLK=1\r", "\r\n: ", "OK\r\n", output, 100)){
		D(printf("CSCLK = 1 was set successfully\n"));
	}

	return 0;
}


general_status SIM800L_Test_AT(){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT\r", "\r\nO", "K\r\n", output, 50)){
		D(printf("SIM800L response on AT success\n"));
		return OK;
	}

	return SIM800_DOES_NOT_RESPOND;
}


general_status SIM800L_RECONNECT(){

	for(uint8_t i = 0; i < 3; i++){
		if(!SIM800L_Check_Conn()){
			for(uint8_t trial = 0; trial < 60; trial++){
				D(printf("Waiting for connection, trial No: %d\n", trial));
				if(SIM800L_Check_Conn()){
					return OK;
				}
				HAL_Delay(1000);
			}
			D(printf("SIM800L not connected to network, resetting SIM800...\n"));
			SIM800L_RESTART();
		}else{
			return OK;
		}
	}
	// all trials have been used, now restart STM board
	NVIC_SystemReset();

	return OK;
}


general_status SIM800L_CSQ(uint8_t* rssi, uint8_t* ber){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT+CSQ\r", "\r\n+CSQ: ", "\r\n\r\nOK", output, 50)){
		char rssi_c[5] = {};
		char ber_c[5] = {};

		char* terminationPos = strchr(output, ',');
		uint8_t numChars = terminationPos - output;
		strncpy(rssi_c, output, numChars);
		strcpy(ber_c, terminationPos+1);
		if(rssi != NULL) *rssi = string_to_number(rssi_c);
		if(ber != NULL) *ber = string_to_number(ber_c);

		return OK;
	}

	return SIM800_ERROR_READING_CSQ;
}


general_status SIM800L_CBC(uint8_t* bcs, uint8_t* bcl, uint16_t* voltage){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT+CBC\r", "\r\n+CBC: ", "\r\n\r\nOK", output, 50)){
		char bcs_c[5] = {};
		char bcl_c[5] = {};
		char voltage_c[5] = {};

		char* terminationPos = strchr(output, ',');
		uint8_t numChars1 = terminationPos - output;
		strncpy(bcs_c, output, numChars1);
		terminationPos = strchr(output+numChars1+1, ',');
		uint8_t numChars2 = terminationPos - (output+numChars1+1);
		strncpy(bcl_c, output+numChars1+1, numChars2);
		strcpy(voltage_c, output+numChars1+1+numChars2+1);
		if(bcs != NULL) *bcs = string_to_number(bcs_c);
		if(bcl != NULL) *bcl = string_to_number(bcl_c);
		if(voltage != NULL) *voltage = string_to_number(voltage_c);

		return OK;
	}

	return SIM800_ERROR_READING_CBC;
}


volatile general_status SIM800L_GET_LOCAL_TIME(struct tm* time){
	char current_time[30] = {};
	if(!SIM800L_Send("AT+CCLK?\r", "\r\n+CCLK: \"", "\"\r\n\r\nOK", current_time, 100)){
		D(printf("Error getting current time from SIM800\n"));
		return SIM800_ERROR_GETTING_CURRENT_TIME;
	}

	int year, month, day, hour, minute, second;
	char tmp[4] = {};
	strncpy(tmp, current_time, 2);
	year = string_to_number(tmp);

	strncpy(tmp, current_time+3, 2);
	month = string_to_number(tmp);

	strncpy(tmp, current_time+6, 2);
	day = string_to_number(tmp);

	strncpy(tmp, current_time+9, 2);
	hour = string_to_number(tmp);

	strncpy(tmp, current_time+12, 2);
	minute = string_to_number(tmp);

	strncpy(tmp, current_time+15, 2);
	second = string_to_number(tmp);

	time->tm_year = year + 2000 - 1900;
	time->tm_mon  = month - 1;
	time->tm_mday = day;
	time->tm_hour = hour;
	time->tm_min  = minute;
	time->tm_sec  = second;

	D(printf("Current time: %d/%d/%d/,%d:%d:%d\n", 	time->tm_year+1900,
													time->tm_mon+1,
													time->tm_mday,
													time->tm_hour,
													time->tm_min,
													time->tm_sec));

	return OK;
}


void SIM800L_Just_Send(char * cmd, uint32_t delay_time){
	HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);
	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, cmd);
	HAL_Delay(delay_time);
}

uint8_t SIM800L_Send(char * cmd, char * expect_before, char * expect_after, char * output, uint32_t response_time){

	HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);
	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, cmd);
	HAL_Delay(response_time);

	char * buffp = (char *)buffer;

	if(strncmp(expect_before, (char *)buffer, strlen((char *)expect_before)) == 0){
		buffp += strlen((char *)expect_before);
		if(strstr((char *)buffp, expect_after) != NULL){
			char * tempp = strstr((char *)buffp, expect_after);
			if((char *)tempp >= (char *)buffer){
				strncpy(output, (char *)buffp, tempp - (char *)buffp);
				return 1;
			}else{
				return 0;
			}
		}
	}

	return 0;
}

uint8_t SIM800L_Check_Conn(){
	char output[MAX_OUTPUT_SIZE] = {};

	if(SIM800L_Send("AT+CREG?\r", "\r\n+CREG: ", "\r\n\r\nOK", output, 100)){}
	else return 0;

	if(strcmp(output, "0,1") == 0){return 1;}
	else return 0;
}
/*
 * returns 1 if successful
 */
uint8_t SIM800L_SMS(char * phone, char * message, uint8_t time_append){
	char output[MAX_OUTPUT_SIZE] = {};
	char country_code[5];
	char phone_str[14] = {};

	if(time_append){
		struct tm currTime;
		char strtime[30] = {};
		getSTMdatetime(&currTime, strtime);
		strcat(message, "\r\n\r\n");
		strncat(message, strtime, strlen(strtime));
	}

	if(strlen(mobile_country_code) != 0){
//		D(printf("--------> mobile_country_code valid\n"));
		strcpy(country_code, mobile_country_code);
	}
	else strcpy(country_code, "9955");

	uint8_t mobile_format = strlen(phone);

	phone_str[0] = '+';
	if(mobile_format == 8){
		strcat(phone_str, country_code);
		strcat(phone_str, phone);
	}else if(mobile_format == 9){
		strncat(phone_str, country_code, 3);
		strcat(phone_str, phone);
	}else if(mobile_format == 12){
		strcat(phone_str, phone);
	}else if(mobile_format == 13){
		strcpy(phone_str, phone);
	}else{
		strcpy(phone_str, phone);
	}

	char cmd_temp[50] = {};
	strcpy(cmd_temp, "AT+CMGS=");
	strcat(cmd_temp, "\"");
	strcat(cmd_temp, phone_str);
	strcat(cmd_temp, "\"");
	strcat(cmd_temp, "\r");
	if(SIM800L_Send(cmd_temp, "\r\n", " ", output, 5)){
		if(strcmp(output, ">") == 0){

			memset(buffer, 0, MAX_RECEIVE_BUF_SIZE);
			__HAL_DMA_DISABLE(huart1.hdmarx);
			huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
			__HAL_DMA_ENABLE(huart1.hdmarx);

			USART_SendText(huart1.Instance, message);
			USART_SendData(huart1.Instance, 26);
			HAL_Delay(100);

			if(!SIM800L_WAIT_FOR("\r\nOK", "", output, 90, 1000)){
				D(printf("SMS Send timeout\n"));
				return SMS_SEND_TIMEOUT_ERROR;
			}
			return 1;
		}
		else return 0;
	}

	return 0;
}


/*
 * characters inside {} are not converted
 * characters inside [] are converted directly, without translation schema
 */
general_status SIM800L_SMS_GEO(char * phone, char * message, uint8_t time_append){

	char output[MAX_OUTPUT_SIZE] = {};
	general_status ret;

	if(!SIM800L_Send("AT+CSMP=17,167,0,8\r", "\r\nOK", "\r\n", output, 100)){
		return SIM800_CSMP_SETTING_ERROR;
	}

	if(!SIM800L_Send("AT+CSCS=\"HEX\"\r", "\r\nOK", "\r\n", output, 100)){
		return SIM800_CSCS_SETTING_ERROR;
	}

	if(time_append){
		struct tm currTime;
		char strtime[30] = {};
		getSTMdatetime(&currTime, strtime);
		strcat(message, "\r\n\r\n[");
		strncat(message, strtime, strlen(strtime));
		strcat(message, "]");
	}
	char text_ucs2[1025] = {};
	ret = ToUTF16(message, text_ucs2);
	if(ret != OK){
		return ret;
	}

	if(!SIM800L_SMS(phone, text_ucs2, 0)){
		return SIM800_ERROR_SENDING_SMS;
	}

	if(!SIM800L_Send("AT+CSMP=17,167,0,0\r", "\r\nOK", "\r\n", output, 100)){
		return SIM800_CSMP_RESETTING_ERROR;
	}

	if(!SIM800L_Send("AT+CSCS=\"IRA\"\r", "\r\nOK", "\r\n", output, 100)){
		return SIM800_CSCS_RESETTING_ERROR;
	}

	return OK;
}

/*
 * Converts string to UTF-16 representation
 * characters inside {} are not converted
 * characters inside [] are converted directly, without translation schema
 */
general_status ToUTF16(const char *text, char *return_text){

	char geo[] = "abgdevzTiklmnopJrstufqRySCcZwWxjh";
//	char text_ucs2[1025] = {};
	int base_hex_val_geo = 0x10d0;
	char onechar[5] = {};
	char curr_chr;
	char text_ucs2[UTF16_MAX_SIZE] = {};
	int no_trans_flag = 0;

	int text_length = strlen(text);

	for (int i=0; i < text_length; i++){
		curr_chr = *(text+i);
		if(curr_chr == '['){
			if(no_trans_flag == 0){
				no_trans_flag = 1;
				char* terminationPos = strchr(text+i, ']');
				if(terminationPos == NULL){
					D(printf("Error: ] not found in string\n"));
					return TO_UTF_CONV_ERROR_TERMINATION_CHAR_NOT_FOUND;
				}
			}
			// sprintf(onechar, "%04X", curr_chr);
			// strcat(text_ucs2, onechar);
			if(*(text+i+1) == ']'){
				no_trans_flag = 0;
			}
			// int numChars = terminationPos - (text + i + 1);
			// for(int j=0; j < numChars; j++){
			//     curr_chr = *(text+i+1+j);
			//     sprintf(onechar, "%04X", curr_chr);
			//     strcat(text_ucs2, onechar);
			// }
			// i += numChars + 1;
		}else if(curr_chr == '{'){
			char* terminationPos = strchr(text, '}');
			if(terminationPos == NULL){
				D(printf("Error: } not found in string\n"));
				return TO_UTF_CONV_ERROR_TERMINATION_CHAR_NOT_FOUND;
			}
			int numChars = terminationPos - (text + i + 1);
			strncat(text_ucs2, text+i+1, numChars);
			i += numChars + 1;
		}else if(no_trans_flag == 1){
			if(curr_chr != ']'){
				sprintf(onechar, "%04X", curr_chr);
				strcat(text_ucs2, onechar);
			}
			if(*(text+i+1) == ']'){
				no_trans_flag = 0;
			}
		}else if(curr_chr == ']'){

		}else{
			if(strchr(geo, curr_chr) == NULL){
				sprintf(onechar, "%04X", curr_chr);
			}else{
				sprintf(onechar, "%04X", base_hex_val_geo + strchr(geo, curr_chr) - geo);
			}
//			D(printf("%c    %s\n", curr_chr, onechar));

			strcat(text_ucs2, onechar);
		}
		if(strlen(text_ucs2) > 1024){
			D(printf("Message will not fit\n"));
			return TO_UTF_CONV_ERROR_MESSAGE_WILL_NOT_FIT;
		}
	}

//	for (int i=0; i < text_length; i++){
//		curr_chr = *(text+i);
//		if(curr_chr == '['){
//			char* terminationPos = strchr(text, ']');
//			if(terminationPos == NULL){
//				D(printf("Error: ] not found in string\n"));
//				return TO_UTF_CONV_ERROR_TERMINATION_CHAR_NOT_FOUND;
//			}
//			int numChars = terminationPos - (text + i + 1);
//			for(int j=0; j < numChars; j++){
//				curr_chr = *(text+i+1+j);
//				sprintf(onechar, "%04X", curr_chr);
//				strcat(text_ucs2, onechar);
//			}
//			i += numChars + 1;
//		}else if(curr_chr == '{'){
//			char* terminationPos = strchr(text, '}');
//			if(terminationPos == NULL){
//				D(printf("Error: } not found in string\n"));
//				return TO_UTF_CONV_ERROR_TERMINATION_CHAR_NOT_FOUND;
//			}
//			int numChars = terminationPos - (text + i + 1);
//			strncat(text_ucs2, text+i+1, numChars);
//			i += numChars + 1;
//		}else{
//			if(strchr(geo, curr_chr) == NULL){
//				sprintf(onechar, "%04X", curr_chr);
//			}else{
//				sprintf(onechar, "%04X", base_hex_val_geo + strchr(geo, curr_chr) - geo);
//			}
////			D(printf("%c    %s\n", curr_chr, onechar));
//
//			strcat(text_ucs2, onechar);
//		}
//		if(strlen(text_ucs2) > 1024){
//			D(printf("Message will not fit\n"));
//			return TO_UTF_CONV_ERROR_MESSAGE_WILL_NOT_FIT;
//		}
//	}

	D(printf("%s\n", text_ucs2));
	text_length = strlen(text_ucs2);
	memset(return_text, 0, text_length);
	strncpy(return_text, text_ucs2, text_length);

	return OK;
}


/*
 * function clears receive dma count before waiting for string
 * max_delay_time is multiple of 10ms
 * string: string to wait for
 * expect_after: string which after the wanted substring
 * output: desired substring *
 */
uint8_t SIM800L_WAIT_FOR(char * string, char * expect_after, char * output, uint32_t max_delay_time, uint32_t interval){

	for(uint8_t i = 0; i < max_delay_time; i++){
//		D(printf("%s\n", (char *)buffer));
		for(int j = 0; j < MAX_RECEIVE_BUF_SIZE; j++){
			D(printf("%x.", (uint8_t)buffer[j]));
		}
		D(printf("\n\n"));
		char * substr = search_str((char *)buffer, MAX_RECEIVE_BUF_SIZE, string, 0);
		if(substr != NULL){
//			strncmp((char *)buffer, string, length) == 0
			D(printf("-----------------------string detected---------------\n"));
			char * buffp = substr;
			buffp += strlen((char *)string);
			if(strstr((char *)buffp, expect_after) != NULL){
				char * tempp = strstr((char *)buffp, expect_after);
				if((char *)tempp >= (char *)buffer){
					strncpy(output, (char *)buffp, tempp - (char *)buffp);
					return 1;
				}else{
					return 0;
				}
			}
			return 1;
		}
		HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);
		__HAL_DMA_DISABLE(huart1.hdmarx);
		huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
		__HAL_DMA_ENABLE(huart1.hdmarx);
		memset(buffer, 0, MAX_RECEIVE_BUF_SIZE);
		HAL_Delay(interval);
	}

	return 0;
}

/*
 * returns number of messages in module memory
 * returns -1 if error
 */
uint8_t SIM800L_Message_Count(){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT+CPMS?\r", "\r\n+CPMS: \"SM\",", ",30,", output, 100)){
		return string_to_number(output);
	}
	return -1;
}
/*
 * returns first message index found in module memory
 * returns -1 if error
 */
uint8_t SIM800L_Get_Message_Index(){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT+CMGL=\"ALL\"\r", "\r\n+CMGL: ", ",\"", output, 100)){
		return string_to_number(output);
	}
	return -1;
}

/*
 * read first unread message found in module memory
 * returns -1 if error
 * returns 0 if no unread messages exist
 * returns 1 if message was found and read
 */
uint8_t SIM800L_Read_First_Message(uint8_t * index, char * status, char * sender, char * alpha, char * datetime, char * data){
	char output[MAX_OUTPUT_SIZE] = {};
	if(SIM800L_Send("AT+CMGL=\"ALL\"\r", "\r\n+CMGL: ", ",\"", output, 1000)){
		*index = string_to_number(output);
		char * buffp = (char *)buffer;
		char * tempp;
		for(uint8_t i = 0; i < 4; i++){
			buffp = strstr((char *)buffp, ",\"");
			buffp = buffp + 2;
			tempp = strstr((char *)buffp, "\"");
			switch(i){
			case 0:
				strncpy(status, (char *)buffp, tempp - buffp);
				break;
			case 1:
				strncpy(sender, (char *)buffp, tempp - buffp);
				break;
			case 2:
				strncpy(alpha, (char *)buffp, tempp - buffp);
				break;
			case 3:
				strncpy(datetime, (char *)buffp, tempp - buffp);
				break;
			}
			buffp = tempp;
		}
		char * data_temp = buffp + 3;
		char * data_end = strstr((char *)data_temp, "\r\n+CMGL:");
		if(data_end != NULL){
			strncpy(data, (char *)data_temp, data_end - data_temp);
			data_end = 0;
		}else if(strstr((char *)data_temp, "\r\nOK") != NULL){
			data_end = strstr((char *)data_temp, "\r\nOK");
			strncpy(data, (char *)data_temp, data_end - data_temp);
			data_end = 0;
		}
		return 1;
	}else if(SIM800L_Send("AT+CMGL=\"REC UNREAD\"\r", "\r\n", "K\r\n", output, 100)){
		if(output[0] == 'O'){
			return 0;
		}
	}
	return -1;
}

/*
 *  read message by index
 *  returns 1 if successful
 *  returns 0 if no message found in index number
 *  returns -1 if error
 */

uint8_t SIM800L_Read_Message(uint8_t index, char * status, char * sender, char * alpha, char * datetime, char * data){
	char output[MAX_OUTPUT_SIZE] = {};
	char num_str[5] = {};
	number_to_string(index, num_str);
	char cmd[15] = {};
	strcpy(cmd, "AT+CMGR=");
	strcat(cmd, num_str);
	strcat(cmd, "\r");
	if(SIM800L_Send(cmd, "\r\n+CMGR:", "\"", output, 2000)){
		char * buffp = (char *)buffer;
		char * tempp;
		for(uint8_t i = 0; i < 4; i++){
			if(i == 0){
				buffp = strstr((char *)buffp, "\"");
				buffp = buffp + 1;
				tempp = strstr((char *)buffp, "\"");
			}else{
				buffp = strstr((char *)buffp, ",\"");
				buffp = buffp + 2;
				tempp = strstr((char *)buffp, "\"");
			}

			switch(i){
			case 0:
				strncpy(status, (char *)buffp, tempp - buffp);
				break;
			case 1:
				strncpy(sender, (char *)buffp, tempp - buffp);
				break;
			case 2:
				strncpy(alpha, (char *)buffp, tempp - buffp);
				break;
			case 3:
				strncpy(datetime, (char *)buffp, tempp - buffp);
				break;
			}
			buffp = tempp;
		}
		char * data_temp = buffp + 3;
		char * data_end = strstr((char *)data_temp, "\r\nOK");
		if(data_end != NULL){
			data_end = strstr((char *)data_temp, "\r\nOK");
			strncpy(data, (char *)data_temp, data_end - data_temp);
			data_end = 0;
		}
		return 1;
	}else if(SIM800L_Send(cmd, "\r\n", "K\r\n", output, 100)){
		if(output[0] == 'O'){
			return 0;
		}
	}
	return -1;
}
/*
 * deletes message from internal memory
 * index specifies message
 * returns 1 if successful
 * returns 0 if unsuccessful
 */
uint8_t SIM800L_Delete_Message(uint8_t index){
	D(printf("Deleting message at index: %d\n", index));

	char cmd[20] = {};
	char temp[5] = {};
	char output[MAX_OUTPUT_SIZE] = {};

	if(index > 0){
		number_to_string(index, temp);
		strcpy(cmd, "AT+CMGD=");
		strcat(cmd, temp);
		strcat(cmd, "\r");
		if(SIM800L_Send(cmd, "\r\n", "K\r\n", output, 200)){
			if(output[0] == 'O') return 1;
		}
	}
	return 0;
}


uint8_t SIM800L_Forward_Message(uint8_t index, char * number){
	D(printf("Forwaring message\n"));

	char output[MAX_OUTPUT_SIZE] = {};
	char cmd[50] = {};
	char temp[5] = {};

	if(index > 0){
		number_to_string(index, temp);
		strcpy(cmd, "AT+CMSS=");
		strcat(cmd, temp);
		strcat(cmd, ",\"");
		strcat(cmd, number);
		strcat(cmd, "\"\r");
		D(printf("cmd: %s\n", cmd));
		memset(buffer, 0, MAX_RECEIVE_BUF_SIZE);
		SIM800L_Just_Send(cmd, 1);
		if(SIM800L_WAIT_FOR("\r\n+CMSS: ", "\r\n\r\nOK", output, 60, 1000)){
			return string_to_number(output);
		}
	}
	return 0;
}


general_status SIM800L_RESTART(){
	D(printf("Resetting SIM800L module by POWERKEY...\n"));
	HAL_GPIO_WritePin(SIM_800_RESET_GPIO_Port, SIM_800_RESET_Pin, 0);
	HAL_Delay(1000);
	HAL_GPIO_WritePin(SIM_800_RESET_GPIO_Port, SIM_800_RESET_Pin, 1);
	HAL_Delay(500);

	char output[MAX_OUTPUT_SIZE] = {};
	SIM800L_WAIT_FOR("\r\nSMS Ready", "\r\n", output, 30, 1000);
	char time[30] = {};
	if(SIM800L_WAIT_FOR("\r\n*PSUTTZ: ", "\r\n", time, 50, 200)){
		D(printf("Time synchronized successfully: %s\n", time));
		D(printf("Resetting CLTS setting...\n"));
		HAL_Delay(1000);
		uint8_t clts = 1;
		while(1){
			SIM800L_Send("AT+CLTS=0;&W\r", "\r\nO", "\r\n", output, 400);
			memset(output, 0, MAX_OUTPUT_SIZE);
			SIM800L_Send("AT+CLTS?\r", "\r\n+CLTS: ", "\r\n\r\nOK", output, 300);
			clts = string_to_number(output);
			if(clts == 0) {
				D(printf("CLTS setting was resetted\n"));
				break;
			}
			HAL_Delay(500);
		}
	}

	HAL_Delay(2000);

	return OK;
}


general_status SIM800L_SHUTDOWN(){
	char output[MAX_OUTPUT_SIZE] = {};
	D(printf("SIM800L powering down...\n"));

	SIM800L_Just_Send("AT+CPOWD=1\r", 100);
	if(SIM800L_WAIT_FOR("\r\nNORMAL POWER DOWN", "\r\n", output, 30, 100)){
		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 0);
		D(printf("SIM800L power down successful\n"));
		return OK;
	}

	return SIM800_NORMAL_POWERDOWN_UNSUCCESSFUL;
}


uint32_t string_to_number(char *string) {
	uint32_t number = 0;

	while (*string != '\0') {
		uint8_t chr = *string;
		if (chr >= 48 && chr <= 57) {
			number = number * 10 + (chr - 48);
		}
		string++;
	}
	return number;
}
/*
 * convert unsigned number to string
 */
void number_to_string(uint32_t number, char * string){
	uint8_t cnt = 0;
	while(number){
		*(string + cnt) = number%10 + 48;
		number = number/10;
		cnt++;
		if(!number) *(string + cnt) = 0;
	}
	strrev((char *)string);
}

void strrev(char *s)
{
   int length, c;
   char *begin, *end, temp;

   length = strlen(s);
   begin  = s;
   end    = s;

   for (c = 0; c < length - 1; c++)
      end++;

   for (c = 0; c < length/2; c++)
   {
      temp   = *end;
      *end   = *begin;
      *begin = temp;

      begin++;
      end--;
   }
}

char * search_str(char * mainstr, uint32_t mainstr_size, char * substr, uint32_t substr_size){
	uint32_t cnt = 0;
	if(substr_size == 0) substr_size = strlen(substr);
	for(uint32_t i = 0; i < mainstr_size; i++){
		cnt = 0;
		for(uint32_t j = 0; j < substr_size; j++){
			if(*(mainstr + i + j) == *(substr + j)){
				cnt++;
			}else{
				break;
			}
		}
		if(cnt == substr_size){
			return mainstr + i;
		}
	}
	return 0;
}
