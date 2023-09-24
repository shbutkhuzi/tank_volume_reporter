/*
 * client_subscription.h
 *
 *  Created on: Sep 12, 2023
 *      Author: SHAKO
 */

#ifndef INC_CLIENT_SUBSCRIPTION_H_
#define INC_CLIENT_SUBSCRIPTION_H_


#include "main.h"


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


general_status add_user(char * phone, char * lower_limits, char * upper_limits);
general_status phone_number_transformation(const char * phone, char * ret_phone);
general_status delete_user(char * phone);
general_status read_user(char * phone);
general_status write_user_enable(char * phone, uint8_t enable);
general_status update_user(char * phone, char * lower_limits, char * upper_limits);

threshold * find_threshold(threshold * head, char type, uint16_t value);
numbers * find_number(numbers * head, char * number);
numbers * add_number(numbers * head, char * number);
threshold * add_threshold(threshold * head, char type, uint16_t value, char * number);
threshold * remove_threshold(threshold * head, char type, uint16_t value, char * number);
void print_thresholds(threshold * head);


#endif /* INC_CLIENT_SUBSCRIPTION_H_ */
