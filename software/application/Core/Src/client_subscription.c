/*
 * client_subscription.c
 *
 *  Created on: Sep 12, 2023
 *      Author: SHAKO
 */


#include "client_subscription.h"
#include "EEPROM_data.h"
#include "string.h"
#include "EEPROM.h"
#include "stdlib.h"


uint8_t subscription_enable = 0;
extern char mobile_country_code[5];

#define PAGE_SIZE	32


general_status add_user(char * phone, char * lower_limits, char * upper_limits){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	char new_user_data[32] = {};
	uint16_t lowers[3] = {};
	uint16_t uppers[3] = {};
	uint8_t lowers_matched, uppers_matched;

	phone_ret = phone_number_transformation(phone, phone_str);
	if(phone_ret != OK) return phone_ret;

	EEPROM_enable();
	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	// check if space is available
	if(current_page > page_end){
		EEPROM_disable();
		ret = SUBSCRIPTION_NO_MORE_SPACE_TO_ADD_NEW_USER;
		goto return_from_function;
	}

	// check if user already exists
	if(current_page - page_start > 0){
		clients_buffer = (char *)malloc((current_page-page_start) * PAGE_SIZE);
		if(clients_buffer == NULL){
			ret = SUBSCRIPTION_MEMORY_ALLOCATION_ERROR;
			goto return_from_function;
		}
		EEPROM_Read(page_start, 0, (uint8_t *)clients_buffer, (current_page-page_start)*PAGE_SIZE);
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){
		if(strncmp(phone_str, clients_buffer+(i*PAGE_SIZE), 8) == 0){
			ret = SUBSCRIPTION_USER_ALREADY_EXISTS;
			goto return_from_function;
		}
	}

	// register new user
	lowers_matched = sscanf(lower_limits, "%hu,%hu,%hu", &lowers[0], &lowers[1], &lowers[2]);
	uppers_matched = sscanf(upper_limits, "%hu,%hu,%hu", &uppers[0], &uppers[1], &uppers[2]);

	if(lowers_matched > 3){
		ret = SUBSCRIPTION_LOWER_THRESHOLDS_OUT_OF_BOUNDS;
		goto return_from_function;
	}
	if(uppers_matched > 3){
		ret = SUBSCRIPTION_UPPER_THRESHOLDS_OUT_OF_BOUNDS;
		goto return_from_function;
	}

	strncpy(new_user_data, phone_str, 8);
	new_user_data[8] = 1;  			// enable
	memcpy(&new_user_data[9], lowers, 6);
	memcpy(&new_user_data[15], uppers, 6);

	EEPROM_Write(current_page, 0, (uint8_t *)new_user_data, 32);
	current_page += 1;
	write_current_page(current_page, 0);
	ret = OK;

	return_from_function:

	EEPROM_disable();
	free(clients_buffer);
	return ret;
}


general_status phone_number_transformation(const char * phone, char * ret_phone){

	char country_code[5];
	uint8_t mobile_format;
	char phone_str[9] = {};

	if(strlen(mobile_country_code) != 0){
		strcpy(country_code, mobile_country_code);
	}else strcpy(country_code, "9955");


	mobile_format = strlen(phone);

	if(mobile_format == 8){
		strcpy(phone_str, phone);
	}else if(mobile_format == 9){
		strcpy(phone_str, phone+1);
	}else if(mobile_format == 12){
		if(strncmp(phone, country_code, 4) != 0){
			return SUBSCRIPTION_COUNTRY_CODE_DOES_NOT_MATCH;
		}
		strcpy(phone_str, phone+4);
	}else if(mobile_format == 13){
		if(strncmp(phone+1, country_code, 4) != 0){
			return SUBSCRIPTION_COUNTRY_CODE_DOES_NOT_MATCH;
		}
		strcpy(phone_str, phone+5);
	}else{
		return SUBSCRIPTION_UNSUPPORTED_NUMBER_FORMAT;
	}

	strncpy(ret_phone, phone_str, 9);
	return OK;
}


general_status delete_user(char * phone){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	char empty_or_latest[32] = {};

	phone_ret = phone_number_transformation(phone, phone_str);
	if(phone_ret != OK) return phone_ret;


	EEPROM_enable();
	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	// copy data
	if(current_page - page_start > 0){
		clients_buffer = (char *)malloc((current_page-page_start) * PAGE_SIZE);
		if(clients_buffer == NULL){
			ret = SUBSCRIPTION_MEMORY_ALLOCATION_ERROR;
			goto return_from_function;
		}
		EEPROM_Read(page_start, 0, (uint8_t *)clients_buffer, (current_page-page_start)*PAGE_SIZE);
	}else{
		ret = SUBSCRIPTION_USER_DOES_NOT_EXIST;
		goto return_from_function;
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){
		if(strncmp(phone_str, clients_buffer+(i*PAGE_SIZE), 8) == 0){

			memset(empty_or_latest, 0, PAGE_SIZE);
			EEPROM_Write(page_start+i, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);

			if(current_page-page_start > 1){
				memcpy(empty_or_latest, clients_buffer+((current_page-page_start-1)*PAGE_SIZE), PAGE_SIZE);
				EEPROM_Write(page_start+i, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);
				memset(empty_or_latest, 0, PAGE_SIZE);
				EEPROM_Write(current_page-1, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);
			}

			current_page -= 1;
			write_current_page(current_page, 0);
			ret = OK;

			goto return_from_function;
		}
	}

	ret = SUBSCRIPTION_USER_DOES_NOT_EXIST;

	return_from_function:

	EEPROM_disable();
	free(clients_buffer);
	return ret;
}


general_status read_user(char * phone){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	char *user = NULL;
	uint16_t lowers[3] = {};
	uint16_t uppers[3] = {};
	time_t last_reach;

	phone_ret = phone_number_transformation(phone, phone_str);
	if(phone_ret != OK) return phone_ret;


	EEPROM_enable();
	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	if(current_page - page_start > 0){
		clients_buffer = (char *)malloc((current_page-page_start) * PAGE_SIZE);
		if(clients_buffer == NULL){
			ret = SUBSCRIPTION_MEMORY_ALLOCATION_ERROR;
			goto return_from_function;
		}
		EEPROM_Read(page_start, 0, (uint8_t *)clients_buffer, (current_page-page_start)*PAGE_SIZE);
	}else{
		ret = SUBSCRIPTION_USER_DOES_NOT_EXIST;
		goto return_from_function;
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){
		if(strncmp(phone_str, clients_buffer+(i*PAGE_SIZE), 8) == 0){

			user = clients_buffer+(i*PAGE_SIZE);
			memcpy(phone_str, user, 8); phone_str[8] = 0;
			memcpy(lowers, user+9, 6);
			memcpy(uppers, user+15, 6);
			memcpy(&last_reach, user+23, 8);

//			D(printf("Plain content:\n"));
//			for(uint8_t j = 0; j < 32; j++){
//				D(printf("%d: %x\n", j, *(user+j)));
//			}
			D(printf("Number:       %s\n", phone_str));
			D(printf("Enable:       %d\n", *(user+8)));
			D(printf("Lower limits: %d,%d,%d\n", lowers[0], lowers[1], lowers[2]));
			D(printf("Upper limits: %d,%d,%d\n", uppers[0], uppers[1], uppers[2]));
			D(printf("Send   count: %d\n", *(user+21)));
			D(printf("Latest reach: %s\n", ctime(&last_reach)));

			ret = OK;
			goto return_from_function;
		}
	}

	ret = SUBSCRIPTION_USER_DOES_NOT_EXIST;

	return_from_function:

	EEPROM_disable();
	free(clients_buffer);
	return ret;
}

