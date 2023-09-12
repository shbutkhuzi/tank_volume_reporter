/*
 * client_subscription.h
 *
 *  Created on: Sep 12, 2023
 *      Author: SHAKO
 */

#ifndef INC_CLIENT_SUBSCRIPTION_H_
#define INC_CLIENT_SUBSCRIPTION_H_


#include "main.h"


general_status add_user(char * phone, char * lower_limits, char * upper_limits);
general_status phone_number_transformation(const char * phone, char * ret_phone);
general_status delete_user(char * phone);
general_status read_user(char * phone);


#endif /* INC_CLIENT_SUBSCRIPTION_H_ */
