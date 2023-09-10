/*
 * Process_Data.h
 *
 *  Created on: Oct 7, 2022
 *      Author: SHAKO
 */

#ifndef INC_PROCESS_DATA_H_
#define INC_PROCESS_DATA_H_


#define REQ_NUM_SIZE							1024


void check_requests();
uint8_t isAdmin(char* number);
general_status process_admin_cmd(char* amdin, char* cmd);
general_status process_client_request(char* client, char* message);
void toLowercase(char* str);

#endif /* INC_PROCESS_DATA_H_ */
