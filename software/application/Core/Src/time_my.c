/*
 * time.c
 *
 *  Created on: Jul 7, 2023
 *      Author: SHAKO
 */


#include <time_my.h>
#include "time.h"
#include "SIM800L.h"


extern RTC_HandleTypeDef hrtc;

struct standby_alarm wakeup_time = {};
struct standby_alarm standby_time = {};


general_status getSTMdatetime(struct tm* datetime, char* strtime){
	RTC_TimeTypeDef newTime = {0};
	RTC_DateTypeDef newDate = {0};

	if (HAL_RTC_GetDate(&hrtc, &newDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_RTC_GetTime(&hrtc, &newTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	datetime->tm_year = newDate.Year + 2000 - 1900;
	datetime->tm_mon  = newDate.Month - 1;
	datetime->tm_mday = newDate.Date;
	datetime->tm_hour = newTime.Hours;
	datetime->tm_min  = newTime.Minutes;
	datetime->tm_sec  = newTime.Seconds;

	sprintf(strtime, "Time: %4d-%02d-%02d %02d:%02d:%02d",  datetime->tm_year + 1900,
															datetime->tm_mon + 1,
															datetime->tm_mday,
															datetime->tm_hour,
															datetime->tm_min,
															datetime->tm_sec);

//	strftime(strtime, 30, "Time: %Y-%m-%d %H:%M:%S", datetime);

//	D(printf("STM time: %s\n", strtime));

	return OK;
}


general_status syncSTMandSIM800time(){
	general_status ret;
	struct tm localTime;

	ret = SIM800L_GET_LOCAL_TIME(&localTime);
	if(ret == OK){
		D(printf("Synchronizing STM time with SIM800\n"));

		RTC_TimeTypeDef newTime = {0};
		RTC_DateTypeDef newDate = {0};

		newTime.Hours 	= localTime.tm_hour;
		newTime.Minutes = localTime.tm_min;
		newTime.Seconds = localTime.tm_sec;
		newDate.Date    = localTime.tm_mday;
		newDate.Month	= localTime.tm_mon + 1;
		newDate.Year	= localTime.tm_year + 1900 - 2000;

		if (HAL_RTC_SetTime(&hrtc, &newTime, RTC_FORMAT_BIN) != HAL_OK)
		{
			Error_Handler();
		}

		if (HAL_RTC_SetDate(&hrtc, &newDate, RTC_FORMAT_BIN) != HAL_OK)
		{
			Error_Handler();
		}

		struct tm currTime;
		char strtime[30] = {};
		getSTMdatetime(&currTime, strtime);

	}else D(printf("Error getting current time during synchronization with STM: Error no: %d\n", ret));

	return OK;
}


