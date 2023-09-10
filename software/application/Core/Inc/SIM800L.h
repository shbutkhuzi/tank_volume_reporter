/*
 * SIM800L.h
 *
 *  Created on: Aug 18, 2022
 *      Author: shako
 */

#ifndef INC_SIM800L_H_
#define INC_SIM800L_H_

#define MAX_RECEIVE_BUF_SIZE 				300 		// characters
#define MAX_OUTPUT_SIZE      				20
#define UTF16_MAX_SIZE		 				1024		// characters


#include "stm32f1xx_hal.h"
#include "main.h"
#include <time_my.h>
#include "time.h"


general_status SIM800L_init(void);
general_status SIM800L_Test_AT();
general_status SIM800L_RECONNECT();
general_status SIM800L_CSQ(uint8_t* rssi, uint8_t* ber);
general_status SIM800L_CBC(uint8_t* bcs, uint8_t* bcl, uint16_t* voltage);
volatile general_status SIM800L_GET_LOCAL_TIME(struct tm* time);
volatile void SIM800L_Just_Send(char * cmd, uint32_t delay_time);
volatile uint8_t SIM800L_Send(char * cmd, char * expect_before, char * expect_after, char * output, uint32_t response_time);
uint8_t SIM800L_Check_Conn();
uint8_t SIM800L_SMS(char * phone, char * message, uint8_t time_append);
general_status SIM800L_SMS_GEO(char * phone, char * message, uint8_t time_append);
general_status ToUTF16(const char *text, char *text_ucs2);
uint8_t SIM800L_WAIT_FOR(char * string, char * expect_after, char * output, uint32_t max_delay_time, uint32_t interval);
uint8_t SIM800L_Message_Count();
uint8_t SIM800L_Get_Message_Index();
uint8_t SIM800L_Read_First_Message(uint8_t * index, char * status, char * sender, char * alpha, char * datetime, char * data);
uint8_t SIM800L_Read_Message(uint8_t index, char * status, char * sender, char * alpha, char * datetime, char * data);
uint8_t SIM800L_Delete_Message(uint8_t index);
uint8_t SIM800L_Forward_Message(uint8_t index, char * number);
general_status SIM800L_RESTART();
general_status SIM800L_SHUTDOWN();

void strrev(char *s);
uint32_t string_to_number(char *string);
void number_to_string(uint32_t number, char * string);
char * search_str(char * mainstr, uint32_t mainstr_size, char * substr, uint32_t substr_size);

#endif /* INC_SIM800L_H_ */
