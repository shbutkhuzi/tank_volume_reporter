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



/*
 * EEPROM must be uenabled before calling this function
 */
general_status find_user(char * phone, uint16_t * page_number, char * user_data){

	uint16_t current_page, page_end, page_start;

	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	if(current_page == page_start){
		return SUBSCRIPTION_USER_DOES_NOT_EXIST;
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){

		EEPROM_Read(page_start, 0, (uint8_t *)clients_buffer, (current_page-page_start)*PAGE_SIZE);

		if(strncmp(phone_str, clients_buffer+(i*PAGE_SIZE), 8) == 0){
			ret = SUBSCRIPTION_USER_ALREADY_EXISTS;
			goto return_from_function;
		}
	}

}


general_status add_user(char * phone, char * lower_limits, char * upper_limits){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	char new_user_data[32] = {};
	uint16_t lowers[3] = {};
	uint16_t uppers[3] = {};
	char lower_types[3] = {};
	char upper_types[3] = {};
	uint8_t en_low_upp_flags = 0;
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
	lowers_matched = sscanf(lower_limits, "%hu(%c),%hu(%c),%hu(%c)", &lowers[0], &lower_types[0], &lowers[1], &lower_types[1], &lowers[2], &lower_types[2]);
	uppers_matched = sscanf(upper_limits, "%hu(%c),%hu(%c),%hu(%c)", &uppers[0], &upper_types[0], &uppers[1], &upper_types[1], &uppers[2], &upper_types[2]);

	if((lowers_matched > 6) || (lowers_matched % 2 != 0)){
		ret = SUBSCRIPTION_LOWER_THRESHOLDS_NOT_RECOGNIZED;
		goto return_from_function;
	}
	if((uppers_matched > 6) || (uppers_matched % 2 != 0)){
		ret = SUBSCRIPTION_UPPER_THRESHOLDS_NOT_RECOGNIZED;
		goto return_from_function;
	}

	// enable bit        for limits: 0 if volume, 1 if time
	//     EN       _              L L L U U U

	// write lower flags
	for(uint8_t i = 0; i < 3; i++){
		if(lower_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(lower_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(lower_types[i] == '\0'){

		}else{
			ret = SUBSCRIPTION_LOWER_LIMIT_TYPE_NOT_RECOGNIZED;
			goto return_from_function;
		}
		en_low_upp_flags = en_low_upp_flags << 1;
	}
	// write upper flags
	for(uint8_t i = 0; i < 3; i++){
		if(upper_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(upper_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(upper_types[i] == '\0'){

		}else{
			ret = SUBSCRIPTION_UPPER_LIMIT_TYPE_NOT_RECOGNIZED;
			goto return_from_function;
		}
		if(i != 2){
			en_low_upp_flags = en_low_upp_flags << 1;
		}
	}
	// write enable to 1
	en_low_upp_flags = en_low_upp_flags | 0b10000000;

	strncpy(new_user_data, phone_str, 8);
	new_user_data[8] = en_low_upp_flags;
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
	char lower_types[3], upper_types[3];
	uint8_t en_low_upp_flags;
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
			memcpy(&en_low_upp_flags, user+8, 1);
			memcpy(lowers, user+9, 6);
			memcpy(uppers, user+15, 6);
			memcpy(&last_reach, user+23, 8);

			for(int8_t j = 5; j >= 3; j--){
				if((en_low_upp_flags >> j) & 1){
					lower_types[5-j] = 'T';
				}else{
					lower_types[5-j] = 'V';
				}
			}
			for(int8_t j = 2; j >= 0; j--){
				if((en_low_upp_flags >> j) & 1){
					upper_types[2-j] = 'T';
				}else{
					upper_types[2-j] = 'V';
				}
			}

//			D(printf("Plain content:\n"));
//			for(uint8_t j = 0; j < 32; j++){
//				D(printf("%d: %x\n", j, *(user+j)));
//			}
			D(printf("Page address: %d\n", page_start+i));
			D(printf("Number:       %s\n", phone_str));
			D(printf("Enable:       %d\n", en_low_upp_flags >> 7));
			D(printf("Lower limits: %d(%c),%d(%c),%d(%c)\n", lowers[0], lower_types[0], lowers[1], lower_types[1], lowers[2], lower_types[2]));
			D(printf("Upper limits: %d(%c),%d(%c),%d(%c)\n", uppers[0], upper_types[0], uppers[1], upper_types[1], uppers[2], upper_types[2]));
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


general_status write_user_enable(char * phone, uint8_t enable){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	uint8_t user_enable_byte = 0;
	char user_data[32] = {};

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

			memcpy(user_data, clients_buffer+(i*PAGE_SIZE), PAGE_SIZE);
			memcpy(&user_enable_byte, user_data+8, 1);
			if(enable){
				user_enable_byte = user_enable_byte | 0b10000000;
			}else{
				user_enable_byte = user_enable_byte & 0b01111111;
			}

			memcpy(user_data+8, &user_enable_byte, 1);
			EEPROM_Write(page_start+i, 0, (uint8_t *)user_data, PAGE_SIZE);

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


general_status update_user(char * phone, char * lower_limits, char * upper_limits){

	general_status ret, phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;
	char *clients_buffer = NULL;
	char user_data[32] = {};
	uint16_t lowers[3] = {};
	uint16_t uppers[3] = {};
	char lower_types[3] = {};
	char upper_types[3] = {};
	uint8_t en_low_upp_flags = 0;
	uint8_t lowers_matched, uppers_matched;

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

	// register new user
	lowers_matched = sscanf(lower_limits, "%hu(%c),%hu(%c),%hu(%c)", &lowers[0], &lower_types[0], &lowers[1], &lower_types[1], &lowers[2], &lower_types[2]);
	uppers_matched = sscanf(upper_limits, "%hu(%c),%hu(%c),%hu(%c)", &uppers[0], &upper_types[0], &uppers[1], &upper_types[1], &uppers[2], &upper_types[2]);

	if((lowers_matched > 6) || (lowers_matched % 2 != 0)){
		ret = SUBSCRIPTION_LOWER_THRESHOLDS_NOT_RECOGNIZED;
		goto return_from_function;
	}
	if((uppers_matched > 6) || (uppers_matched % 2 != 0)){
		ret = SUBSCRIPTION_UPPER_THRESHOLDS_NOT_RECOGNIZED;
		goto return_from_function;
	}

	// enable bit        for limits: 0 if volume, 1 if time
	//     EN       _              L L L U U U

	// write lower flags
	for(uint8_t i = 0; i < 3; i++){
		if(lower_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(lower_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(lower_types[i] == '\0'){

		}else{
			ret = SUBSCRIPTION_LOWER_LIMIT_TYPE_NOT_RECOGNIZED;
			goto return_from_function;
		}
		en_low_upp_flags = en_low_upp_flags << 1;
	}
	// write upper flags
	for(uint8_t i = 0; i < 3; i++){
		if(upper_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(upper_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(upper_types[i] == '\0'){

		}else{
			ret = SUBSCRIPTION_UPPER_LIMIT_TYPE_NOT_RECOGNIZED;
			goto return_from_function;
		}
		if(i != 2){
			en_low_upp_flags = en_low_upp_flags << 1;
		}
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){
		if(strncmp(phone_str, clients_buffer+(i*PAGE_SIZE), 8) == 0){

			uint8_t flags;

			memcpy(user_data, clients_buffer+(i*PAGE_SIZE), PAGE_SIZE);
			memcpy(&flags, user_data + 8, 1);
			flags = flags & 0b11000000;
			flags = flags | (en_low_upp_flags & 0b00111111);
			memcpy(user_data + 8, &flags, 1);
			memcpy(user_data + 9, lowers, 6);
			memcpy(user_data + 15, uppers, 6);

			EEPROM_Write(page_start+i, 0, (uint8_t *)user_data, 32);

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


// function that returns pointer to the member that contains `value`
// returns NULL if not found
threshold * find_threshold(threshold * head, char type, uint16_t value){

    if(head == NULL) return NULL;

    threshold * this = head;

    while (this != NULL){
        if((this->value == value) && (this->type == type)){
            return this;
        }
        this = this->next_ptr;
    }

    return NULL;
}

// function that searches for a number in numbers linked list
// returns NULL if not found
numbers * find_number(numbers * head, char * number){

    if(head == NULL) return NULL;

    numbers * this = head;

    while (this != NULL){
        if(strncmp(this->number, number, 8) == 0){
            return this;
        }
        this = this->next_ptr;
    }

    return NULL;
}

// adds new number to numers linked list
// returns pointer to newly added number member, or NULL if unsuccessful
numbers * add_number(numbers * head, char * number){

    if(head == NULL) return NULL;

    numbers * temp = head;

    while(temp->next_ptr != NULL){
        temp = temp->next_ptr;
    }

    numbers * new_record = NULL;
    new_record = (numbers *)malloc(sizeof(numbers));
    if(new_record == NULL) return NULL;

    temp->next_ptr = new_record;
    memset(new_record->number, 0, 9);
    strncpy(new_record->number, number, 8);
    new_record->next_ptr = NULL;

    return new_record;
}

// adds new threshold
// checks if such record already exists
// if not exists, creates new one
// always returns the head
threshold * add_threshold(threshold * head, char type, uint16_t value, char * number){

    if(type != 'U' && type != 'L') return NULL;

    if(head == NULL){
        // initialize threshold linked list
        head = (threshold *)malloc(sizeof(threshold));
        if(head == NULL) return NULL;

        head->next_ptr = NULL;
        head->num_ptr = (numbers *)malloc(sizeof(numbers));
        if(head->num_ptr == NULL) return NULL;
        head->num_ptr->next_ptr = NULL;
        memset(head->num_ptr->number, 0, 9);
        strncpy(head->num_ptr->number, number, 8);

        head->type = type;
        head->value = value;
        head->trigger_flag = 0;

        return head;
    }

    threshold * if_exists = NULL;
    if_exists = find_threshold(head, type, value);
    if(if_exists != NULL){
        // this threshold already exists
        numbers * if_num_exists = NULL;
        if_num_exists = find_number(if_exists->num_ptr, number);
        if(if_num_exists != NULL) return head;

        if(add_number(if_exists->num_ptr, number) == NULL) return NULL;
        return head;
    }

    // add new threshold member
    threshold * temp = head;
    while (temp->next_ptr != NULL){
        temp = temp->next_ptr;
    }

    threshold * new_threshold = NULL;
    new_threshold = (threshold *)malloc(sizeof(threshold));
    if(new_threshold == NULL) return NULL;

    temp->next_ptr = new_threshold;
    new_threshold->next_ptr = NULL;
    new_threshold->num_ptr = (numbers *)malloc(sizeof(numbers));
    if(new_threshold->num_ptr == NULL) return NULL;
    new_threshold->num_ptr->next_ptr = NULL;
    memset(new_threshold->num_ptr->number, 0, 9);
    strncpy(new_threshold->num_ptr->number, number, 8);

    new_threshold->type = type;
    new_threshold->value = value;
    new_threshold->trigger_flag = 0;

    return head;
}


threshold * remove_threshold(threshold * head, char type, uint16_t value, char * number){

    if(head == NULL) return NULL;
    if(strlen(number) == 0) return NULL;

    // if type and value both are specified, remove only corresponding member
    if(type != 0 && value != 0){
        if(type != 'U' && type != 'L') return NULL;

        uint8_t found_flag = 0;
        threshold * this_thr = head;
        threshold * prev_thr = NULL;
        while (this_thr != NULL){
            if((this_thr->value == value) && (this_thr->type == type)){

                numbers * this_num = this_thr->num_ptr;
                numbers * prev_num = NULL;
                while (this_num != NULL){
                    if(strncmp(this_num->number, number, 8) == 0){
                        found_flag = 1;
                        if(this_num->next_ptr == NULL && prev_num == NULL){
                            free(this_num);
                            this_thr->num_ptr = NULL;
                        }else if(this_num->next_ptr == NULL && prev_num != NULL){
                            free(this_num);
                            prev_num->next_ptr = NULL;
                        }else if(this_num->next_ptr != NULL && prev_num != NULL){
                            prev_num->next_ptr = this_num->next_ptr;
                            free(this_num);
                        }else if(this_num->next_ptr != NULL && prev_num == NULL){
                            this_thr->num_ptr = this_num->next_ptr;
                            free(this_num);
                        }

                        break;
                    }
                    prev_num = this_num;
                    this_num = this_num->next_ptr;
                }

            }

            if(found_flag && this_thr->num_ptr == NULL){

                if(this_thr->next_ptr == NULL && prev_thr == NULL){
                    free(this_thr);
                    head = NULL;
                }else if(this_thr->next_ptr == NULL && prev_thr != NULL){
                    free(this_thr);
                    prev_thr->next_ptr = NULL;
                }else if(this_thr->next_ptr != NULL && prev_thr != NULL){
                    prev_thr->next_ptr = this_thr->next_ptr;
                    free(this_thr);
                }else if(this_thr->next_ptr != NULL && prev_thr == NULL){
                    head = this_thr->next_ptr;
                    free(this_thr);
                }

                return head;
            }

            prev_thr = this_thr;
            this_thr = this_thr->next_ptr;
        }

    }else if(type == 0 && value == 0){

        uint8_t found_flag = 0;
        threshold * this_thr = head;
        threshold * prev_thr = NULL;

        while(this_thr != NULL){

            numbers * this_num = this_thr->num_ptr;
            numbers * prev_num = NULL;

            found_flag = 0;
            while(this_num != NULL){

                if(strncmp(this_num->number, number, 8) == 0){
                    found_flag = 1;
                    if(this_num->next_ptr == NULL && prev_num == NULL){
                        free(this_num);
                        this_thr->num_ptr = NULL;
                    }else if(this_num->next_ptr == NULL && prev_num != NULL){
                        free(this_num);
                        prev_num->next_ptr = NULL;
                    }else if(this_num->next_ptr != NULL && prev_num != NULL){
                        prev_num->next_ptr = this_num->next_ptr;
                        free(this_num);
                    }else if(this_num->next_ptr != NULL && prev_num == NULL){
                        this_thr->num_ptr = this_num->next_ptr;
                        free(this_num);
                    }

                    break;
                }

                prev_num = this_num;
                this_num = this_num->next_ptr;
            }

            if(found_flag && this_thr->num_ptr == NULL){

                if(this_thr->next_ptr == NULL && prev_thr == NULL){
                    free(this_thr);
                    head = NULL;
                    return head;
                }else if(this_thr->next_ptr == NULL && prev_thr != NULL){
                    free(this_thr);
                    prev_thr->next_ptr = NULL;
                    return head;
                }else if(this_thr->next_ptr != NULL && prev_thr != NULL){
                    prev_thr->next_ptr = this_thr->next_ptr;
                    free(this_thr);
                    this_thr = prev_thr;
                }else if(this_thr->next_ptr != NULL && prev_thr == NULL){
                    head = this_thr->next_ptr;
                    free(this_thr);
                    this_thr = head;
                    prev_thr = NULL;
                    continue;
                }

            }

            prev_thr = this_thr;
            this_thr = this_thr->next_ptr;
        }

    }

    return head;

}


void print_thresholds(threshold * head){

    if(head == NULL) printf("Nothing to print\n");

    threshold * current = head;

    do{
        printf("value:             %d\n", current->value);
        printf("Type:              %c\n", current->type);
        printf("Trigger flag:      %d\n", current->trigger_flag);
        // printf("Pointer:           %d\n", current);
        // printf("Number pointer:    %d\n", current->num_ptr);
        if(current->num_ptr == NULL) {printf("                            No numbers for this threshold\n");}
        else{
            numbers * current_num = current->num_ptr;
            printf("                            Numbers:\n");
            do{

                printf("                                %s\n", current_num->number);
                current_num = current_num->next_ptr;
            } while (current_num != NULL);

        }
        current = current->next_ptr;
    } while (current != NULL);

}


general_status init_read_users(threshold * volume_thresholds, threshold * time_thresholds){

	if(subscription_enable != 1){
		return SUBSCRIPTION_NOT_ENABLED;
	}

	uint16_t current_page, page_end, page_start;
	general_status ret;
	char *clients_buffer = NULL;
	char *user = NULL;

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
		ret = SUBSCRIPTION_NO_USER_REGISTERED;;
		goto return_from_function;
	}


	for(uint16_t i = 0; i < current_page-page_start; i++){

		user = clients_buffer+(i*PAGE_SIZE);
		memcpy(phone_str, user, 8); phone_str[8] = 0;
		memcpy(&en_low_upp_flags, user+8, 1);
		memcpy(lowers, user+9, 6);
		memcpy(uppers, user+15, 6);
		memcpy(&last_reach, user+23, 8);

		for(int8_t j = 5; j >= 3; j--){
			if((en_low_upp_flags >> j) & 1){
				lower_types[5-j] = 'T';
			}else{
				lower_types[5-j] = 'V';
			}
		}
		for(int8_t j = 2; j >= 0; j--){
			if((en_low_upp_flags >> j) & 1){
				upper_types[2-j] = 'T';
			}else{
				upper_types[2-j] = 'V';
			}
		}

//			D(printf("Plain content:\n"));
//			for(uint8_t j = 0; j < 32; j++){
//				D(printf("%d: %x\n", j, *(user+j)));
//			}
		D(printf("Page address: %d\n", page_start+i));
		D(printf("Number:       %s\n", phone_str));
		D(printf("Enable:       %d\n", en_low_upp_flags >> 7));
		D(printf("Lower limits: %d(%c),%d(%c),%d(%c)\n", lowers[0], lower_types[0], lowers[1], lower_types[1], lowers[2], lower_types[2]));
		D(printf("Upper limits: %d(%c),%d(%c),%d(%c)\n", uppers[0], upper_types[0], uppers[1], upper_types[1], uppers[2], upper_types[2]));
		D(printf("Send   count: %d\n", *(user+21)));
		D(printf("Latest reach: %s\n", ctime(&last_reach)));

		ret = OK;
		goto return_from_function;
	}



	return_from_function:
	EEPROM_disable();
	free(clients_buffer);

}



