/*
 * EEPROM_data.h
 *
 *  Created on: Oct 2, 2022
 *      Author: SHAKO
 */

#ifndef INC_EEPROM_DATA_H_
#define INC_EEPROM_DATA_H_

#include "stdint.h"
#include "time_my.h"

#define CONF_REPORT_MAX_LENGTH		1100 // (characters)
#define MAX_ADMINS					4

void EEPROM_enable();
void EEPROM_disable();
uint8_t write_bootloder_enable(uint8_t mode, uint8_t auto_en);
uint8_t read_bootloder_enable(uint8_t auto_en);
uint8_t write_bootloder_wakeup(uint8_t flag, uint8_t auto_en);
uint8_t read_bootloder_wakeup(uint8_t auto_en);
uint8_t write_request_mode(uint8_t mode, uint8_t auto_en);
uint8_t read_request_mode(uint8_t auto_en);
uint8_t write_ds18b20_mode(uint8_t mode, uint8_t auto_en);
uint8_t read_ds18b20_mode(uint8_t auto_en);
uint8_t write_rate_of_change_mode(uint8_t mode, uint8_t auto_en);
uint8_t read_rate_of_change_mode(uint8_t auto_en);
uint8_t write_batch_data_send_mode(uint8_t mode, uint8_t auto_en);
uint8_t read_batch_data_send_mode(uint8_t auto_en);
uint8_t write_log_requested_num_mode(uint8_t mode, uint8_t auto_en);
uint8_t read_log_requested_num_mode(uint8_t auto_en);
uint8_t write_standby_mode_enable(uint8_t mode, uint8_t auto_en);
uint8_t read_standby_mode_enable(uint8_t auto_en);
uint8_t write_subscription_enable(uint8_t mode, uint8_t auto_en);
uint8_t read_subscription_enable(uint8_t auto_en);

uint8_t write_current_page(uint16_t curr_page, uint8_t auto_en);
uint16_t read_current_page(uint8_t auto_en);
uint8_t write_page_start(uint16_t page_start, uint8_t auto_en);
uint16_t read_page_start(uint8_t auto_en);
uint8_t write_page_end(uint16_t page_end, uint8_t auto_en);
uint16_t read_page_end(uint8_t auto_en);
uint8_t read_subscr_meta_info(uint16_t* page_start, uint16_t* current_page, uint16_t* page_end, uint8_t auto_en);
uint8_t write_mobile_country_code(char * code, uint8_t auto_en);
uint8_t read_mobile_country_code(char * code, uint8_t auto_en);
uint8_t read_APN(char * APN, uint8_t auto_en);
uint8_t write_APN(char * APN, uint8_t auto_en);
uint8_t read_server_address(char * server_address, uint8_t auto_en);
uint8_t write_server_adress(char * server_address, uint8_t auto_en);
uint8_t read_username(char * username, uint8_t auto_en);
uint8_t write_username(char * username, uint8_t auto_en);
uint8_t read_password(char * password, uint8_t auto_en);
uint8_t write_password(char * password, uint8_t auto_en);
uint8_t read_file_name(char * file_name, uint8_t auto_en);
uint8_t write_file_name(char * file_name, uint8_t auto_en);
uint8_t read_file_path(char * file_path, uint8_t auto_en);
uint8_t write_file_path(char * file_path, uint8_t auto_en);
uint8_t read_local_file_name(char * loc_file_name, uint8_t auto_en);
uint8_t write_local_file_name(char * loc_file_name, uint8_t auto_en);
uint8_t read_admin_number(char * admin_number, uint8_t index, uint8_t auto_en);
uint8_t write_admin_number(char * admin_number, uint8_t index, uint8_t auto_en);
uint8_t read_mobile_num(char * number, uint16_t page, uint16_t offset, uint8_t auto_en);
uint8_t write_mobile_num(char * number, uint16_t page, uint16_t offset, uint8_t auto_en);
uint8_t clear_admin_number(uint8_t index, uint8_t auto_en);
uint8_t write_provider_identifier1(char* provider, uint8_t auto_en);
uint8_t read_provider_identifier1(char* provider, uint8_t auto_en);
uint8_t write_provider_identifier2(char* provider, uint8_t auto_en);
uint8_t read_provider_identifier2(char* provider, uint8_t auto_en);
uint8_t write_min_distance(uint16_t mndstance, uint8_t auto_en);
uint16_t read_min_distance(uint8_t auto_en);
uint8_t write_max_distance(uint16_t mxdstance, uint8_t auto_en);
uint16_t read_max_distance(uint8_t auto_en);
uint8_t write_cylinder_radius(float radius, uint8_t auto_en);
float read_cylinder_radius(uint8_t auto_en);
uint8_t write_cylinder_height(float height, uint8_t auto_en);
float read_cylinder_height(uint8_t auto_en);
uint8_t write_dome_height(float height, uint8_t auto_en);
float read_dome_height(uint8_t auto_en);
uint8_t write_wakeup_time(struct standby_alarm wake_time, uint8_t auto_en);
struct standby_alarm read_wakeup_time(uint8_t auto_en);
uint8_t write_standby_time(struct standby_alarm standby, uint8_t auto_en);
struct standby_alarm read_standby_time(uint8_t auto_en);
general_status read_key_word(uint8_t index, char* key_word, uint8_t auto_en);
general_status write_key_word(uint8_t index, char* key_word, uint8_t auto_en);


uint8_t read_all_conf(char* conf);
general_status EEPROM_set_to_default();

void display_mem(void* mem, int mem_size, int line_len);


#endif /* INC_EEPROM_DATA_H_ */
