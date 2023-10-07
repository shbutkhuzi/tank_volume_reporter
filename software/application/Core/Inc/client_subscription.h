/*
 * client_subscription.h
 *
 *  Created on: Sep 12, 2023
 *      Author: SHAKO
 */

#ifndef INC_CLIENT_SUBSCRIPTION_H_
#define INC_CLIENT_SUBSCRIPTION_H_


#include "main.h"


typedef struct client_struct{
	uint16_t page_address;
    char phone_number[9];
    uint8_t enable;
	char lower_types[3], upper_types[3];
    uint16_t lowers[3], uppers[3];
	uint16_t send_count;
	time_t last_reach_time;
} client;


typedef struct __attribute__((packed)) numbers_struct{
    char number[9];
    struct numbers_struct* next_ptr;
} numbers;


typedef struct __attribute__((packed)) threshold_struct{
    char type;                              		// L or U    U - Upper, L - Lower
    uint16_t value;                        		    // value to be triggered on
    uint8_t trigger_flag;                   		// flag that indicates whether this threshold has already triggered or not
    numbers* num_ptr;                       		// pointer to number list
    struct threshold_struct* next_ptr;      	    // pointer to next member
} threshold;


typedef struct __attribute__((packed)) page_addr_struct{
    char number[9];
    uint16_t address;
    struct page_addr_struct* next_ptr;
} page_addr;


typedef struct __attribute__((packed)) data_addr_struct{
	threshold * volume_thresholds;
	threshold * time_thresholds;
	page_addr * number_addr;
} data_addr;



general_status find_user(char * phone, uint16_t * page_number, char * user_data);
general_status get_user_data(char * phone, uint16_t * page_addr, client * client_data);
general_status parse_limits_data(char * lower_limits, char * upper_limits, client * client_data);
general_status client_struct_to_str(const client * client_data, char * return_data);
general_status str_to_client_struct(const char * str_data, client * client_data);
general_status add_user(char * phone, char * lower_limits, char * upper_limits);
general_status phone_number_transformation(const char * phone, char * ret_phone);
general_status delete_user(char * phone, uint16_t * page_address);
general_status read_user(char * phone);
general_status user_data_print_repr(client * user_data, char * return_str);
general_status write_user_enable(char * phone, uint8_t enable);
general_status update_user(char * phone, char * lower_limits, char * upper_limits);
general_status init_read_users(data_addr * data);


threshold * find_threshold(threshold * head, char type, uint16_t value);
numbers * find_number(numbers * head, char * number);
numbers * add_number(numbers * head, char * number);
threshold * add_threshold(threshold * head, char type, uint16_t value, char * number);
threshold * remove_threshold(threshold * head, char type, uint16_t value, char * number);
void print_thresholds(threshold * head);
page_addr * find_number_page(page_addr * head, const char * number);
page_addr * add_number_page(page_addr * head, const char * number, const uint16_t address);
page_addr * delete_number_page(page_addr * head, char * number);
void print_number_page(page_addr * head);
general_status move_client_data_to_RAM(client * client_data, data_addr * data);
general_status remove_client_data_from_RAM(client * client_data, data_addr * data);

#endif /* INC_CLIENT_SUBSCRIPTION_H_ */
