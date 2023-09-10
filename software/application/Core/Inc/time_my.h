/*
 * time.h
 *
 *  Created on: Jul 7, 2023
 *      Author: SHAKO
 */

#ifndef INC_TIME_MY_H_
#define INC_TIME_MY_H_

#include "main.h"
#include "time.h"


struct standby_alarm{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};


general_status getSTMdatetime(struct tm* datetime, char* strtime);
general_status syncSTMandSIM800time();

#endif /* INC_TIME_MY_H_ */
