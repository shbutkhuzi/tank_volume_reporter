/*
 * critical.h
 *
 *  Created on: Jul 17, 2023
 *      Author: SHAKO
 */

#ifndef INC_CRITICAL_H_
#define INC_CRITICAL_H_


#include "main.h"
#include "time.h"

general_status critical_check();
general_status struct_tm_time_after_x_minutes(struct tm* time_base, struct tm* time_after_x_minutes, uint16_t x_minutes);


#endif /* INC_CRITICAL_H_ */
