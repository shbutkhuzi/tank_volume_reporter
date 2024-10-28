/*
 * EEPROM_data.h
 *
 *  Created on: Oct 2, 2022
 *      Author: SHAKO
 */

#ifndef INC_EEPROM_DATA_H_
#define INC_EEPROM_DATA_H_

#include "stdint.h"

void EEPROM_enable();
void EEPROM_disable();
uint8_t write_bootloder_enable(uint8_t mode);
uint8_t read_bootloder_enable();
uint8_t write_bootloder_wakeup(uint8_t flag);
uint8_t write_mobile_country_code(char * code);
uint8_t read_mobile_country_code(char * code);

uint8_t read_APN(char * APN);
uint8_t write_APN(char * APN);
uint8_t read_server_address(char * server_address);
uint8_t write_server_adress(char * server_address);
uint8_t read_username(char * username);
uint8_t write_username(char * username);
uint8_t read_password(char * password);
uint8_t write_password(char * password);
uint8_t read_file_name(char * file_name);
uint8_t write_file_name(char * file_name);
uint8_t read_file_path(char * file_path);
uint8_t write_file_path(char * file_path);
uint8_t read_local_file_name(char * loc_file_name);
uint8_t write_local_file_name(char * loc_file_name);
uint8_t read_admin_number(char * admin_number, uint8_t index);
uint8_t write_admin_number(char * admin_number, uint8_t index);
uint8_t read_mobile_num(char * number, uint16_t page, uint16_t offset);
uint8_t write_mobile_num(char * number, uint16_t page, uint16_t offset);
uint8_t clear_admin_number(uint8_t index);


#endif /* INC_EEPROM_DATA_H_ */
