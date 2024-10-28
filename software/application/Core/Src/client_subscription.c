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

data_addr data = {};

uint8_t subscription_enable = 0;
extern char mobile_country_code[5];

#define PAGE_SIZE	32



/*
 * EEPROM must be uenabled before calling this function
 */
general_status find_user(char * phone, uint16_t * page_number, char * user_data){

	general_status phone_ret;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start;

	memcpy(phone_str, phone, 8);
	if(strlen(phone) != 8){
		phone_ret = phone_number_transformation(phone, phone_str);
		if(phone_ret != OK) return phone_ret;
	}

	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	if(current_page == page_start){
		return SUBSCRIPTION_USER_DOES_NOT_EXIST;
	}

	for(uint16_t i = 0; i < current_page-page_start; i++){

		*page_number = page_start + i;

		EEPROM_Read(*page_number, 0, (uint8_t *)user_data, PAGE_SIZE);

		if(strncmp(phone_str, user_data, 8) == 0){

			return OK;
		}
	}

	return SUBSCRIPTION_USER_DOES_NOT_EXIST;
}

/*
 * phone: 8 character long phone number
 */
general_status get_user_data(char * phone, uint16_t * page_addr, client * client_data){

	general_status ret, rett;
	uint16_t page_address;
	char user_data[33] = {};
	char phone_str[9] = {};

	memcpy(phone_str, phone, 8);
	if((strlen(phone) != 8) && (page_addr == NULL)){
		rett = phone_number_transformation(phone, phone_str);
		if(rett != OK) return rett;
	}

	// if page is specified, directly reading from that page
	if(page_addr != NULL){
		EEPROM_Read(*page_addr, 0, (uint8_t *)user_data, PAGE_SIZE);
		page_address = *page_addr;
		goto parse_data;
	}

	ret = find_user(phone_str, &page_address, user_data);
	if(ret != OK) return SUBSCRIPTION_USER_DOES_NOT_EXIST;


	parse_data:
	client_data->page_address = page_address;
	rett = str_to_client_struct(user_data, client_data);
	if(rett != OK) return rett;


//	memset(client_data->phone_number, 0, 9);
//	strncpy(client_data->phone_number, phone_str, 8);
//
//	memcpy(&en_low_upp_flags, user_data+8, 1);
//	memcpy(client_data->lowers, user_data+9, 6);
//	memcpy(client_data->uppers, user_data+15, 6);
//	memcpy(&client_data->send_count, user_data+21, 2);
//	memcpy(&client_data->last_reach_time, user_data+23, 8);
//
//	for(int8_t j = 5; j >= 3; j--){
//		if((en_low_upp_flags >> j) & 1){
//			client_data->lower_types[5-j] = 'T';
//		}else{
//			client_data->lower_types[5-j] = 'V';
//		}
//	}
//	for(int8_t j = 2; j >= 0; j--){
//		if((en_low_upp_flags >> j) & 1){
//			client_data->upper_types[2-j] = 'T';
//		}else{
//			client_data->upper_types[2-j] = 'V';
//		}
//	}
//
//	client_data->enable = en_low_upp_flags >> 7;

	return OK;
}


general_status parse_limits_data(char * lower_limits, char * upper_limits, client * client_data){

	uint8_t lowers_matched, uppers_matched;

	lowers_matched = sscanf(lower_limits, "%hu(%c),%hu(%c),%hu(%c)", &client_data->lowers[0],
																	 &client_data->lower_types[0],
																	 &client_data->lowers[1],
																	 &client_data->lower_types[1],
																	 &client_data->lowers[2],
																	 &client_data->lower_types[2]);
	uppers_matched = sscanf(upper_limits, "%hu(%c),%hu(%c),%hu(%c)", &client_data->uppers[0],
																	 &client_data->upper_types[0],
																	 &client_data->uppers[1],
																	 &client_data->upper_types[1],
																	 &client_data->uppers[2],
																	 &client_data->upper_types[2]);

	if((lowers_matched == 0) || (lowers_matched > 6) || (lowers_matched % 2 != 0)){
		return SUBSCRIPTION_LOWER_THRESHOLDS_NOT_RECOGNIZED;
	}
	if((lowers_matched == 0) || (uppers_matched > 6) || (uppers_matched % 2 != 0)){
		return SUBSCRIPTION_UPPER_THRESHOLDS_NOT_RECOGNIZED;
	}

	return OK;
}


general_status client_struct_to_str(const client * client_data, char * return_data){

	uint8_t en_low_upp_flags = 0;

	memset(return_data, 0, PAGE_SIZE);

	// enable bit        for limits: 0 if volume, 1 if time
	//     EN       _              L L L U U U

	// write lower flags
	for(uint8_t i = 0; i < 3; i++){
		if(client_data->lower_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(client_data->lower_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(client_data->lower_types[i] == '\0'){

		}else{
			return SUBSCRIPTION_LOWER_LIMIT_TYPE_NOT_RECOGNIZED;
		}
		en_low_upp_flags = en_low_upp_flags << 1;
	}
	// write upper flags
	for(uint8_t i = 0; i < 3; i++){
		if(client_data->upper_types[i] == 'V'){
			en_low_upp_flags = en_low_upp_flags & 0b11111110;
		}else if(client_data->upper_types[i] == 'T'){
			en_low_upp_flags = en_low_upp_flags | 0b00000001;
		}else if(client_data->upper_types[i] == '\0'){

		}else{
			return SUBSCRIPTION_UPPER_LIMIT_TYPE_NOT_RECOGNIZED;
		}
		if(i != 2){
			en_low_upp_flags = en_low_upp_flags << 1;
		}
	}

	if(client_data->enable){
		en_low_upp_flags = en_low_upp_flags | 0b10000000;
	}else{
		en_low_upp_flags = en_low_upp_flags & 0b01111111;
	}

	strncpy(return_data, client_data->phone_number, 8);
	return_data[8] = en_low_upp_flags;
	memcpy(&return_data[9], client_data->lowers, 6);
	memcpy(&return_data[15], client_data->uppers, 6);
	memcpy(&return_data[21], &client_data->send_count, 2);
	memcpy(&return_data[23], &client_data->last_reach_time, 8);

	return OK;
}


general_status str_to_client_struct(const char * user_data, client * client_data){

	uint8_t en_low_upp_flags = 0;

	memset(client_data->phone_number, 0, 9);
	strncpy(client_data->phone_number, user_data, 8);

	memcpy(&en_low_upp_flags, user_data+8, 1);
	memcpy(client_data->lowers, user_data+9, 6);
	memcpy(client_data->uppers, user_data+15, 6);
	memcpy(&client_data->send_count, user_data+21, 2);
	memcpy(&client_data->last_reach_time, user_data+23, 8);

	for(int8_t j = 5; j >= 3; j--){
		if((en_low_upp_flags >> j) & 1){
			client_data->lower_types[5-j] = 'T';
		}else{
			client_data->lower_types[5-j] = 'V';
		}
	}
	for(int8_t j = 2; j >= 0; j--){
		if((en_low_upp_flags >> j) & 1){
			client_data->upper_types[2-j] = 'T';
		}else{
			client_data->upper_types[2-j] = 'V';
		}
	}

	client_data->enable = en_low_upp_flags >> 7;

	return OK;
}


/*
 * !!!! depreciated, instead use update_user
 */
general_status add_user(char * phone, char * lower_limits, char * upper_limits){

	general_status ret, rett;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start, page_number;
	char user_data[32] = {};
	client client_data = {};

	rett = phone_number_transformation(phone, phone_str);
	if(rett != OK) return rett;

	EEPROM_enable();
	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	// check if space is available
	if(current_page > page_end){
		EEPROM_disable();
		ret = SUBSCRIPTION_NO_MORE_SPACE_TO_ADD_NEW_USER;
		goto return_from_function;
	}

	// check if user already exists
	rett = find_user(phone_str, &page_number, user_data);
	if(rett == OK){
		ret = SUBSCRIPTION_USER_ALREADY_EXISTS;
		goto return_from_function;
	}

	// register new user
	rett = parse_limits_data(lower_limits, upper_limits, &client_data);
	if(rett != OK){
		ret = rett;
		goto return_from_function;
	}

	client_data.enable = 1;
	strncpy(client_data.phone_number, phone_str, 8);

	rett = client_struct_to_str(&client_data, user_data);

	EEPROM_Write(current_page, 0, (uint8_t *)user_data, 32);
	current_page += 1;
	write_current_page(current_page, 0);
	ret = OK;

	return_from_function:

	EEPROM_disable();
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


general_status delete_user(char * phone, uint16_t * page_address){

	general_status ret, rett;
	char phone_str[9] = {};
	uint16_t current_page, page_end, page_start, page_number;
	char empty_or_latest[32] = {};
	char user_data[32] = {};

	EEPROM_enable();

	if(page_address == NULL){
		rett = phone_number_transformation(phone, phone_str);
		if(rett != OK) return rett;

		rett = find_user(phone_str, &page_number, user_data);
		if(rett != OK){
			ret = rett;
			goto return_from_function;
		}
	}else{
		page_number = *page_address;
	}

	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

	memset(empty_or_latest, 0, PAGE_SIZE);
	EEPROM_Write(page_number, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);

	if(current_page - 1 != page_number){
		EEPROM_Read(current_page-1, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);

		EEPROM_Write(page_number, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);
		memset(empty_or_latest, 0, PAGE_SIZE);
		EEPROM_Write(current_page-1, 0, (uint8_t *)empty_or_latest, PAGE_SIZE);
	}

	current_page -= 1;
	write_current_page(current_page, 0);
	ret = OK;

	return_from_function:

	EEPROM_disable();
	return ret;
}


general_status read_user(char * phone){

	general_status ret;
	client user_data = {};
	char bufr[500] = {};

	EEPROM_enable();

	ret = get_user_data(phone, NULL, &user_data);
	if(ret != OK) {
		EEPROM_disable();
		return ret;
	}

	user_data_print_repr(&user_data, bufr);
	D(printf(bufr));

	EEPROM_disable();
	return OK;
}


general_status user_data_print_repr(client * user_data, char * return_str){

	time_t time;

	*return_str = 0;
	sprintf(return_str + strlen(return_str), "Page address: %d\n", user_data->page_address);
	sprintf(return_str + strlen(return_str), "Number:       %s\n", user_data->phone_number);
	sprintf(return_str + strlen(return_str), "Enable:       %d\n", user_data->enable);
	sprintf(return_str + strlen(return_str), "Lower limits: %d(%c),%d(%c),%d(%c)\n", user_data->lowers[0],
																					 user_data->lower_types[0],
																					 user_data->lowers[1],
																					 user_data->lower_types[1],
																					 user_data->lowers[2],
																					 user_data->lower_types[2]);
	sprintf(return_str + strlen(return_str), "Upper limits: %d(%c),%d(%c),%d(%c)\n", user_data->uppers[0],
																					 user_data->upper_types[0],
																					 user_data->uppers[1],
																					 user_data->upper_types[1],
																					 user_data->uppers[2],
																					 user_data->upper_types[2]);
	sprintf(return_str + strlen(return_str), "Send   count: %d\n", user_data->send_count);
	time = user_data->last_reach_time;
	sprintf(return_str + strlen(return_str), "Latest reach: %s", ctime(&time));

	return OK;
}


general_status write_user_enable(char * phone, uint8_t enable){

	general_status ret, rett;
	char phone_str[9] = {};
	uint16_t page_number;
	uint8_t user_enable_byte = 0;
	char user_data[32] = {};

	rett = phone_number_transformation(phone, phone_str);
	if(rett != OK) return rett;


	EEPROM_enable();

	rett = find_user(phone_str, &page_number, user_data);
	if(rett != OK){
		ret = rett;
		goto return_from_function;
	}

	memcpy(&user_enable_byte, user_data+8, 1);
	if(enable){
		user_enable_byte = user_enable_byte | 0b10000000;
	}else{
		user_enable_byte = user_enable_byte & 0b01111111;
	}

	memcpy(user_data+8, &user_enable_byte, 1);
	EEPROM_Write(page_number, 0, (uint8_t *)user_data, PAGE_SIZE);

	ret = OK;

	return_from_function:

	EEPROM_disable();
	return ret;
}


/*
 * updates limits info if user exists
 * adds new user if it does not exist
 */
general_status update_user(char * phone, char * lower_limits, char * upper_limits){

	general_status ret, rett;
	char phone_str[9] = {};
	char user_data[32] = {};
	client client_data = {};
	uint16_t current_page, page_end, page_start;
	uint8_t adding_new_user_flag = 0;

	rett = phone_number_transformation(phone, phone_str);
	if(rett != OK) return rett;

	EEPROM_enable();

	rett = get_user_data(phone_str, NULL, &client_data);
	if(rett == SUBSCRIPTION_USER_DOES_NOT_EXIST){
		adding_new_user_flag = 1;
		read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

		// check if space is available
		if(current_page > page_end){
			ret = SUBSCRIPTION_NO_MORE_SPACE_TO_ADD_NEW_USER;
			goto return_from_function;
		}
	}
	else if(rett != OK){
		ret = rett;
		goto return_from_function;
	}

	rett = parse_limits_data(lower_limits, upper_limits, &client_data);
	if(rett != OK){
		ret = rett;
		goto return_from_function;
	}

	if(adding_new_user_flag){
		client_data.enable = 1;
		strncpy(client_data.phone_number, phone_str, 8);
	}

	rett = client_struct_to_str(&client_data, user_data);
	if(rett != OK){
		ret = rett;
		goto return_from_function;
	}

	if(adding_new_user_flag){
		EEPROM_Write(current_page, 0, (uint8_t *)user_data, PAGE_SIZE);
		current_page += 1;
		write_current_page(current_page, 0);
		ret = OK;
		goto return_from_function;
	}

	EEPROM_Write(client_data.page_address, 0, (uint8_t *)user_data, PAGE_SIZE);
	ret = OK;

	return_from_function:

	EEPROM_disable();
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

    if(head == NULL) {
    	D(printf("Nothing to print\n"));
    	return;
    }

    threshold * current = head;
    numbers * current_num;

    do{
        D(printf("value:             %d\n", current->value));
        D(printf("Type:              %c\n", current->type));
        D(printf("Trigger flag:      %d\n", current->trigger_flag));
        // D(printf("Pointer:           %d\n", current));
        // D(printf("Number pointer:    %d\n", current->num_ptr));
        if(current->num_ptr == NULL) {D(printf("                            No numbers for this threshold\n"));}
        else{
            current_num = current->num_ptr;
            D(printf("                            Numbers:\n"));
            do{

                D(printf("                                %s\n", current_num->number));
                current_num = current_num->next_ptr;
            } while (current_num != NULL);

        }
        current = current->next_ptr;
    } while (current != NULL);

}


page_addr * find_number_page(page_addr * head, const char * number){

    if(head == NULL) return NULL;

    page_addr * this = head;

    while (this != NULL){
        if(strncmp(this->number, number, 8) == 0){
            return this;
        }
        this = this->next_ptr;
    }

    return NULL;
}


page_addr * add_number_page(page_addr * head, const char * number, const uint16_t address){

    // if not initialized, initialize
    if(head == NULL) {
        head = (page_addr *)malloc(sizeof(page_addr));
        if(head == NULL) return NULL;

        head->address = address;
        memset(head->number, 0, 9);
        strncpy(head->number, number, 8);
        head->next_ptr = NULL;

        return head;
    }

    // if exists, update
    page_addr * temp = NULL;
    temp = find_number_page(head, number);
    if(temp != NULL){
        temp->address = address;
        return head;
    }

    // if not exists, add at the end
    temp = head;
    while(temp->next_ptr != NULL){
        temp = temp->next_ptr;
    }

    page_addr * new_record = NULL;
    new_record = (page_addr *)malloc(sizeof(page_addr));
    if(new_record == NULL) return NULL;

    temp->next_ptr = new_record;
    memset(new_record->number, 0, 9);
    strncpy(new_record->number, number, 8);
    new_record->address = address;
    new_record->next_ptr = NULL;

    return head;
}


page_addr * delete_number_page(page_addr * head, char * number){

    if(head == NULL) return NULL;

    page_addr * this = head;
    page_addr * prev = NULL;

    while (this != NULL){
        if(strncmp(this->number, number, 8) == 0){
            if(prev == NULL && this->next_ptr == NULL){
                free(this);
                head = NULL;
            }else if(prev != NULL && this->next_ptr == NULL){
                free(this);
                prev->next_ptr = NULL;
            }else if(prev == NULL && this->next_ptr != NULL){
                head = this->next_ptr;
                free(this);
            }else if(prev != NULL && this->next_ptr != NULL){
                prev->next_ptr = this->next_ptr;
                free(this);
            }

            break;
        }

        prev = this;
        this = this->next_ptr;
    }

    return head;
}


void print_number_page(page_addr * head){

    if(head == NULL){
    	D(printf("Nothing to print\n"));
    	return;
    }

    page_addr * this = head;
    while(this != NULL){

        D(printf("Number:     %s   Page: %4hu\n", this->number, this->address));

        this = this->next_ptr;
    }
}


general_status init_read_users(data_addr * data){

	general_status ret, rett;
	uint16_t current_page, page_end, page_start, page_number;
	char user_data[PAGE_SIZE];
	client client_data = {};

	if(subscription_enable != 1){
		return SUBSCRIPTION_NOT_ENABLED;
	}

	EEPROM_enable();

	read_subscr_meta_info(&page_start, &current_page, &page_end, 0);
	if(page_start == current_page){
		ret = SUBSCRIPTION_NO_USER_REGISTERED;
		goto return_from_function;
	}


	for(uint16_t i = 0; i < current_page-page_start; i++){

		page_number = page_start + i;

		EEPROM_Read(page_number, 0, (uint8_t *)user_data, PAGE_SIZE);

		rett = str_to_client_struct(user_data, &client_data);
		if(rett != OK){
			ret = rett;
			goto return_from_function;
		}

		client_data.page_address = page_number;

		rett = move_client_data_to_RAM(&client_data, data);
		if(rett != OK){
			ret = rett;
			goto return_from_function;
		}

//		*number_addr = add_number_page(*number_addr, client_data.phone_number, page_number);
//
//		for(uint8_t i = 0; i < 3; i++){
//			if(client_data.lowers[i] != 0){
//				if(client_data.lower_types[i] == 'V'){
//					*volume_thresholds = add_threshold(*volume_thresholds, 'L', client_data.lowers[i], client_data.phone_number);
//				}else if(client_data.lower_types[i] == 'T'){
//					*time_thresholds = add_threshold(*time_thresholds, 'L', client_data.lowers[i], client_data.phone_number);
//				}
//			}
//			if(client_data.uppers[i] != 0){
//				if(client_data.upper_types[i] == 'V'){
//					*volume_thresholds = add_threshold(*volume_thresholds, 'U', client_data.uppers[i], client_data.phone_number);
//				}else if(client_data.upper_types[i] == 'T'){
//					*time_thresholds = add_threshold(*time_thresholds, 'U', client_data.uppers[i], client_data.phone_number);
//				}
//			}
//		}
	}

	ret = OK;

	return_from_function:
	EEPROM_disable();
	return ret;
}


general_status move_client_data_to_RAM(client * client_data, data_addr * data){

	data->number_addr = add_number_page(data->number_addr, client_data->phone_number, client_data->page_address);

	if(data->number_addr == NULL){
		return SUBSCRIPTION_ADD_NUMBER_PAGE_ERROR_WHILE_MOVING_CLIENT_DATA_TO_RAM;
	}

	for(uint8_t i = 0; i < 3; i++){
		if(client_data->lowers[i] != 0){
			if(client_data->lower_types[i] == 'V'){
				data->volume_thresholds = add_threshold(data->volume_thresholds, 'L', client_data->lowers[i], client_data->phone_number);
				if(data->volume_thresholds == NULL) return SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_VOLUME_WHILE_MOVING_CLIENT_DATA_TO_RAM;
			}else if(client_data->lower_types[i] == 'T'){
				data->time_thresholds = add_threshold(data->time_thresholds, 'L', client_data->lowers[i], client_data->phone_number);
				if(data->volume_thresholds == NULL) return SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_TIME_WHILE_MOVING_CLIENT_DATA_TO_RAM;
			}
		}
		if(client_data->uppers[i] != 0){
			if(client_data->upper_types[i] == 'V'){
				data->volume_thresholds = add_threshold(data->volume_thresholds, 'U', client_data->uppers[i], client_data->phone_number);
				if(data->volume_thresholds == NULL) return SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_VOLUME_WHILE_MOVING_CLIENT_DATA_TO_RAM;
			}else if(client_data->upper_types[i] == 'T'){
				data->time_thresholds = add_threshold(data->time_thresholds, 'U', client_data->uppers[i], client_data->phone_number);
				if(data->volume_thresholds == NULL) return SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_TIME_WHILE_MOVING_CLIENT_DATA_TO_RAM;
			}
		}
	}

	return OK;
}


general_status remove_client_data_from_RAM(client * client_data, data_addr * data){

	data->number_addr = delete_number_page(data->number_addr, client_data->phone_number);
	data->volume_thresholds = remove_threshold(data->volume_thresholds, 0, 0, client_data->phone_number);
	data->time_thresholds = remove_threshold(data->time_thresholds, 0, 0, client_data->phone_number);

	return OK;
}
