/*
 * critical.c
 *
 *  Created on: Jul 17, 2023
 *      Author: SHAKO
 */


#include "critical.h"
#include "adc_util.h"
#include "SIM800L.h"
#include "EEPROM_data.h"
#include "string.h"
#include "rtc.h"


extern char admins[MAX_ADMINS][14];


//uint8_t low_battery_shutdown_flag = 0;
uint8_t solar_precense, solar_precense_prev;
uint8_t solar_prec_detect_on_powerup = 0;
//time_t shutdown_time;


general_status critical_check(){

	struct adcMeas adc_meas;
	general_status ret;
	char message[200] = {};
	struct tm currTime, afterXMinutes;
	char strtime[30] = {};

	ret = getADCMeasurements(&adc_meas);
	if(ret != OK){
		return ret;
	}


	if((adc_meas.lipo >= 3.55) && __HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET){

		D(printf("Woke up from standby mode, battery level satisfactory\n"));

	}else if(adc_meas.lipo >= 3.55){

		D(printf("Battery level satisfactory\n"));

	}else if((adc_meas.lipo < 3.55) && __HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET){
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);  // clear the flag
		D(printf("Woke up from standby, lipo battery level critical, going back to standby\n"));


		ret = getSTMdatetime(&currTime, strtime);
		if(ret != OK){
			return ret;
		}
		struct_tm_time_after_x_minutes(&currTime, &afterXMinutes, 15);

		RTC_AlarmTypeDef sAlarm;
		sAlarm.Alarm = RTC_ALARM_A;
		sAlarm.AlarmTime.Hours = afterXMinutes.tm_hour;
		sAlarm.AlarmTime.Minutes = afterXMinutes.tm_min;
		sAlarm.AlarmTime.Seconds = afterXMinutes.tm_sec;
		HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);

		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		/* clear the RTC Wake UP (WU) flag */
		//  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
		__HAL_RTC_CLEAR_FLAG();

		HAL_PWR_EnterSTANDBYMode();
	}else if(adc_meas.lipo > 3.45){

		D(printf("Battery low\n"));

	}else{
		// critical shutdown
		D(printf("Lipo battery level critical\n"));
		strcpy(message, "Lipo battery level critical. Shutting down SIM800L module");

		// wake sim800 from sleep
		HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
		HAL_Delay(500);

		if(SIM800L_Check_Conn()){
			if(!SIM800L_SMS(admins[0], message, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
		}

		SIM800L_SHUTDOWN();
		// in case shutdown command did not succeed, cut power
		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 0);


		ret = getSTMdatetime(&currTime, strtime);
		if(ret != OK){
			return ret;
		}
		struct_tm_time_after_x_minutes(&currTime, &afterXMinutes, 15);

		RTC_AlarmTypeDef sAlarm;
		sAlarm.Alarm = RTC_ALARM_A;
		sAlarm.AlarmTime.Hours = afterXMinutes.tm_hour;
		sAlarm.AlarmTime.Minutes = afterXMinutes.tm_min;
		sAlarm.AlarmTime.Seconds = afterXMinutes.tm_sec;
		HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);

		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		/* clear the RTC Wake UP (WU) flag */
		//  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
		__HAL_RTC_CLEAR_FLAG();

		HAL_PWR_EnterSTANDBYMode();
	}

//	if((adc_meas.lipo < 3.45) && (low_battery_shutdown_flag == 0)){
//		D(printf("Lipo battery level critical\n"));
//		strcpy(message, "Lipo battery level critical. Shutting down SIM800L module");
//
//
//		ret = getSTMdatetime(&currTime, strtime);
//		if(ret != OK){
//			return ret;
//		}
//		shutdown_time = mktime(&currTime);
//
//		// wake sim800 from sleep
//		HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
//		HAL_Delay(500);
//
//		if(SIM800L_Check_Conn()){
//			if(!SIM800L_SMS(admins[0], message, 1)){
//				return SIM800_ERROR_SENDING_SMS;
//			}
//		}
//
//		SIM800L_SHUTDOWN();
//		// in case shutdown command did not succeed, cut power
//		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 0);
//
//		low_battery_shutdown_flag = 1;
//	}else{
//		ret = getSTMdatetime(&currTime, strtime);
//		if(ret != OK){
//			return ret;
//		}
//
//		if(low_battery_shutdown_flag && (adc_meas.lipo >= 3.5) && (mktime(&currTime) - shutdown_time > 2*60)){
//			low_battery_shutdown_flag = 0;
//
//			SIM800L_init();
//			D(printf("Woke up from SIM800L critical shutdown\n"));
//			strcpy(message, "Woke up from SIM800L critical shutdown");
//
//			// wake sim800 from sleep
//			HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
//			HAL_Delay(500);
//
//			if(SIM800L_Check_Conn()){
//				if(!SIM800L_SMS(admins[0], message, 1)){
//					return SIM800_ERROR_SENDING_SMS;
//				}
//			}
//		}
//	}

	if(solar_prec_detect_on_powerup){
		strcpy(message, "Solar panel is not connected");

		// wake sim800 from sleep
		HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
		HAL_Delay(500);

		if(SIM800L_Check_Conn()){
			if(!SIM800L_SMS(admins[0], message, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			solar_prec_detect_on_powerup = 0;
		}
	}

	solar_precense = HAL_GPIO_ReadPin(SOLAR_PRESENSE_GPIO_Port, SOLAR_PRESENSE_Pin);

//	detect solar panel absense on power up
	if ((__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) && solar_precense){
		solar_prec_detect_on_powerup = 1;
	}

	if((solar_precense == 1) && (solar_precense_prev == 0)){
		strcpy(message, "Solar panel is disconnected");

		// wake sim800 from sleep
		HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
		HAL_Delay(500);

		if(SIM800L_Check_Conn()){
			if(!SIM800L_SMS(admins[0], message, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
		}

	}else if((solar_precense == 0) && (solar_precense_prev == 1)){
		strcpy(message, "Solar panel is reconnected");

		// wake sim800 from sleep
		HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
		HAL_Delay(500);

		if(SIM800L_Check_Conn()){
			if(!SIM800L_SMS(admins[0], message, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
		}
	}
	solar_precense_prev = solar_precense;




	return OK;
}



general_status struct_tm_time_after_x_minutes(struct tm* time_base, struct tm* time_after_x_minutes, uint16_t x_minutes){
	time_t time_base_t = mktime(time_base);

	time_base_t = time_base_t + x_minutes * 60;

	localtime_r(&time_base_t, time_after_x_minutes);

	return OK;
}

