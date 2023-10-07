/*
 * Process_Data.c
 *
 *  Created on: Oct 7, 2022
 *      Author: SHAKO
 */

#include "SIM800L.h"
#include "string.h"
#include "EEPROM_data.h"
#include "stdio.h"
#include "stdlib.h"
#include "new_firmware.h"
#include "Process_Data.h"
#include "ctype.h"
#include "adc_util.h"
#include "predict.h"
#include "time_my.h"
#include "rtc.h"
#include "client_subscription.h"


extern char admins[MAX_ADMINS][14];
extern char provider[2][32];

uint8_t bootloader_restart = 0;
uint8_t answer_to_requests_enable = 0;
uint8_t ds18b20_measurements_enable = 0;
uint8_t rate_of_change_enable = 0;
uint8_t batch_data_send_enable = 0;
uint8_t log_requested_num_en = 0;
char 	mobile_country_code[5] = {};
uint8_t restart_flag = 0;

extern volatile uint16_t min_distance;
extern volatile uint16_t max_distance;
extern volatile float cylinder_height;
extern volatile float cylinder_radius;
extern volatile float dome_height;

extern char key_word_geo[2][128];
extern char key_word_lat[2][32];

extern float tank_temp;
extern float curr_volume;
extern float accum_slope;

char req_nums[REQ_NUM_SIZE] = {};

extern uint8_t ds18_first_meas_flag;
extern uint16_t a02yyuw_measurement;
extern uint8_t ds18b20_precense_flag;

extern uint8_t standby_mode_enable;
extern uint8_t subscription_enable;

extern data_addr data;

extern struct standby_alarm wakeup_time;
extern struct standby_alarm standby_time;

void check_requests(){
	uint8_t index = 0;
	uint8_t flag = 0;
	char status[20] = {};
	char sender[20] = {};
	char alpha[20] = {};
	char datetime[30] = {};
	char data[MAX_RECEIVE_BUF_SIZE] = {};
	int8_t msg_cnt;


	process_messages:

	msg_cnt = SIM800L_Message_Count();
	if(msg_cnt <= 0){
		D(printf("No messages to process\n"));
		return;
	}

	D(printf("Messages in Inbox: %d\n", msg_cnt));
	for(uint8_t i = 0; i < msg_cnt; i++){
		flag = SIM800L_Read_First_Message(&index, status, sender, alpha, datetime, data);

		if(flag != 1){
			D(printf("No unread messages or error\n"));
			return;
		}

		if(index <= 0){
			D(printf("Message index error: index = %d\n", index));
			return;
		}

		D(printf("Current Message index: %d\n", index));
		D(printf("Message: %s\n", data));
		if((strcmp(sender, admins[0]) == 0) && (strncmp(data, "cmd:bootloader", 14) == 0)){
			uint8_t bcs, bcl;
			uint16_t voltage;
			general_status ret;

			ret = SIM800L_CBC(&bcs, &bcl, &voltage);
			if(ret == OK && voltage < 3650){
				char sendStr[100];
				sprintf(sendStr, "!Aborting: battery voltage is less than 3.65 volts for firmware upgrade. voltage: %d", voltage);
				SIM800L_SMS(admins[0], sendStr, 1);
				D(printf("Battery low for firmware upgrade\n"));
				SIM800L_Delete_Message(index);
				continue;
			}

			HAL_NVIC_DisableIRQ(RTC_IRQn);
			HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);
			HAL_NVIC_DisableIRQ(USART1_IRQn);

			D(printf("Downloading new firmware..."));
			ret = Download_New_Firmware();
			if(ret == OK){
				char sendStr[100];
				sprintf(sendStr, "Firmware download success, now starting bootloader");
				SIM800L_SMS(admins[0], sendStr, 1);
				D(printf("Start bootloading...\n"));
				write_bootloder_enable(1, 1);
				bootloader_restart = 1;
				HAL_Delay(1000);
			}else{
				char sendStr[100];
				sprintf(sendStr, "Firmware download errored with error No: %d", ret);
				SIM800L_SMS(admins[0], sendStr, 1);
				D(printf("Error downloading firmware\n"));
				SIM800L_Delete_Message(index);
				return;
			}

			HAL_NVIC_EnableIRQ(RTC_IRQn);
			HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
			HAL_NVIC_EnableIRQ(USART1_IRQn);
		}else if(isAdmin(sender) && (strncmp(data, "cmd:", 4) == 0)){
			general_status ret = process_admin_cmd(sender, data);
			if(ret != OK) return;
		}else if((strcmp(sender, provider[0]) == 0) || (strcmp(sender, provider[1]) == 0)){
//			HAL_Delay(1000);
			SIM800L_Forward_Message(index, admins[0]);
		}else if(answer_to_requests_enable || isAdmin(sender)){
			general_status ret = process_client_request(sender, data);
			if(ret != OK) return;
		}

		if(!SIM800L_Delete_Message(index)){
			D(printf("Message delete failed at index: %d\n", index));
			return;
		}

		if(restart_flag){
			restart_flag = 0;
			NVIC_SystemReset();
		}
	}

	goto process_messages;

}

/*
 * This function expexcts number in format +9955********
 */
uint8_t isAdmin(char* number){
	if((strcmp(number+5, admins[0]+5) == 0) || (strcmp(number+5, admins[1]+5) == 0) ||
       (strcmp(number+5, admins[2]+5) == 0) || (strcmp(number+5, admins[3]+5) == 0)){
		return 1;
	}
	return 0;
}


general_status process_admin_cmd(char* admin, char* cmd){
	if((strncmp(cmd, "cmd:write_mobile_country_code:", 30) == 0)){
		char code[5] = {};
		strncpy(code, cmd+30, 4);
		EEPROM_enable();
		write_mobile_country_code(code, 0);
		read_mobile_country_code(code, 0);
		EEPROM_disable();
		strcpy(mobile_country_code, code);
		D(printf("mobile country code set to: %s\n", code));
		char sendStr[50];
		sprintf(sendStr, "read_mobile_country_code:%s", code);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_APN:", 14) == 0)){
		char apn[32] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+14);
		strncpy(apn, cmd+14, numChars);
		EEPROM_enable();
		write_APN(apn, 0);
		read_APN(apn, 0);
		EEPROM_disable();
		D(printf("APN set to: %s\n", apn));
		char sendStr[50];
		sprintf(sendStr, "read_APN:%s", apn);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_server_address:", 25) == 0)){
		char servAddr[64] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+25);
		strncpy(servAddr, cmd+25, numChars);
		EEPROM_enable();
		write_server_adress(servAddr, 0);
		read_server_address(servAddr, 0);
		EEPROM_disable();
		D(printf("Server address set to: %s\n", servAddr));
		char sendStr[120];
		sprintf(sendStr, "read_server_address:%s", servAddr);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_username:", 19) == 0)){
		char usrnm[32] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+19);
		strncpy(usrnm, cmd+19, numChars);
		EEPROM_enable();
		write_username(usrnm, 0);
		read_username(usrnm, 0);
		EEPROM_disable();
		D(printf("Username set to: %s\n", usrnm));
		char sendStr[50];
		sprintf(sendStr, "read_username:%s", usrnm);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_password:", 19) == 0)){
		char pass[32] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+19);
		strncpy(pass, cmd+19, numChars);
		EEPROM_enable();
		write_password(pass, 0);
		read_password(pass, 0);
		EEPROM_disable();
		D(printf("Password set to: %s\n", pass));
		char sendStr[50];
		sprintf(sendStr, "read_password:%s", pass);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_file_name:", 20) == 0)){
		char flnm[64] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+20);
		strncpy(flnm, cmd+20, numChars);
		EEPROM_enable();
		write_file_name(flnm, 0);
		read_file_name(flnm, 0);
		EEPROM_disable();
		D(printf("File name set to: %s\n", flnm));
		char sendStr[120];
		sprintf(sendStr, "read_file_name:%s", flnm);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_file_path:", 20) == 0)){
		char flpth[96] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+20);
		strncpy(flpth, cmd+20, numChars);
		EEPROM_enable();
		write_file_path(flpth, 0);
		read_file_path(flpth, 0);
		EEPROM_disable();
		D(printf("File path set to: %s\n", flpth));
		char sendStr[120];
		sprintf(sendStr, "read_file_path:%s", flpth);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_local_file_name:", 26) == 0)){
		char lclflnm[64] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+26);
		strncpy(lclflnm, cmd+26, numChars);
		EEPROM_enable();
		write_local_file_name(lclflnm, 0);
		read_local_file_name(lclflnm, 0);
		EEPROM_disable();
		D(printf("Local file name set to: %s\n", lclflnm));
		char sendStr[120];
		sprintf(sendStr, "read_local_file_name:%s", lclflnm);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_admin_number:", 23) == 0)){
		char admnnm[14] = {};
		uint8_t index = 1;
		char sendStr[120];

		char* commaPos = strchr(cmd, ',');
		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = commaPos - (cmd+23);
		strncpy(admnnm, cmd+23, numChars);
		char indexChr[2] = {};
		strncpy(indexChr, cmd+23+1+numChars, terminationPos - commaPos - 1);
		index = string_to_number(indexChr);
		if((index >= MAX_ADMINS) || (index < 1)){
			sprintf(sendStr, "admin index out of range error: index=%d, number=%s", index, admnnm);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			return ADMIN_INDEX_OUT_OF_RANGE;
		}
		EEPROM_enable();
		write_admin_number(admnnm, index, 0);
		read_admin_number(admnnm, index, 0);
		EEPROM_disable();
		D(printf("Admin %d set to: %s\n", index, admnnm));
		strcpy(admins[index], admnnm);

		sprintf(sendStr, "read_admin%d_number:%s", index, admnnm);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:clear_admin_number:", 23) == 0)){
		char admnm[2] = {};
		uint8_t index = 1;
		char admnnm[14] = {};
		char sendStr[120];

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(admnm, cmd+23, numChars);
		index = string_to_number(admnm);
		if((index >= MAX_ADMINS) || (index < 1)){
			sprintf(sendStr, "admin index out of range error: index=%d", index);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			return ADMIN_INDEX_OUT_OF_RANGE;
		}
		EEPROM_enable();
		clear_admin_number(index, 0);
		read_admin_number(admnnm, index, 0);
		EEPROM_disable();
		strcpy(admins[index], admnnm);
		sprintf(sendStr, "read_admin%d_number:%s", index, admnnm);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_key_word:", 19) == 0)){
		char kwrd[33] = {};
		uint8_t index = 0;
		char sendStr[120];

		char* commaPos = strchr(cmd, ',');
		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = commaPos - (cmd+19);
		strncpy(kwrd, cmd+19, numChars);
		char indexChr[2] = {};
		strncpy(indexChr, cmd+19+1+numChars, terminationPos - commaPos - 1);
		index = string_to_number(indexChr);
		if((index < 0) || (index > 1)){
			sprintf(sendStr, "key word out of range error: index=%d, word=%s", index, kwrd);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			return KEY_WORD_INDEX_OUT_OF_RANGE;
		}
		EEPROM_enable();
		write_key_word(index, kwrd, 0);
		read_key_word(index, kwrd, 0);
		EEPROM_disable();
		D(printf("Key word %d set to: %s\n", index, kwrd));
		ToUTF16(kwrd, key_word_geo[index]);
		toLowercase(kwrd);
		strcpy(key_word_lat[index], kwrd);

		sprintf(sendStr, "read_key_word:%d,word:%s", index, kwrd);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_provider_identifier1:", 31) == 0)){
		char prvdr1[32] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+31);
		strncpy(prvdr1, cmd+31, numChars);
		EEPROM_enable();
		write_provider_identifier1(prvdr1, 0);
		read_provider_identifier1(prvdr1, 0);
		EEPROM_disable();
		D(printf("Provider identifier 1 set to: %s\n", prvdr1));
		strcpy(provider[0], prvdr1);
		char sendStr[70];
		sprintf(sendStr, "read_provider_identifier1:%s", prvdr1);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_provider_identifier2:", 31) == 0)){
		char prvdr2[32] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+31);
		strncpy(prvdr2, cmd+31, numChars);
		EEPROM_enable();
		write_provider_identifier2(prvdr2, 0);
		read_provider_identifier2(prvdr2, 0);
		EEPROM_disable();
		D(printf("Provider identifier 2 set to: %s\n", prvdr2));
		strcpy(provider[1], prvdr2);
		char sendStr[70];
		sprintf(sendStr, "read_provider_identifier2:%s", prvdr2);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_min_distance:", 23) == 0)){
		char mnds[5] = {};
		uint16_t mnds_int;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(mnds, cmd+23, numChars);
		mnds_int = (uint16_t)string_to_number(mnds);
		EEPROM_enable();
		write_min_distance(mnds_int, 0);
		min_distance = read_min_distance(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_min_distance:%d", min_distance);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_max_distance:", 23) == 0)){
		char mxds[5] = {};
		uint16_t mxds_int;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(mxds, cmd+23, numChars);
		mxds_int = (uint16_t)string_to_number(mxds);
		EEPROM_enable();
		write_max_distance(mxds_int, 0);
		max_distance = read_max_distance(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_max_distance:%d", max_distance);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_cylinder_radius:", 26) == 0)){
		char rdus[15] = {};
		float rdus_float;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+26);
		strncpy(rdus, cmd+26, numChars);
		rdus_float = strtof(rdus, NULL);
		EEPROM_enable();
		write_cylinder_radius(rdus_float, 0);
		cylinder_radius = read_cylinder_radius(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_cylinder_radius:%f", cylinder_radius);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_cylinder_height:", 26) == 0)){
		char chght[15] = {};
		float chght_float;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+26);
		strncpy(chght, cmd+26, numChars);
		chght_float = strtof(chght, NULL);
		EEPROM_enable();
		write_cylinder_height(chght_float, 0);
		cylinder_height = read_cylinder_height(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_cylinder_height:%f", cylinder_height);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_dome_height:", 22) == 0)){
		char dhght[15] = {};
		float dhght_float;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+22);
		strncpy(dhght, cmd+22, numChars);
		dhght_float = strtof(dhght, NULL);
		EEPROM_enable();
		write_dome_height(dhght_float, 0);
		dome_height = read_dome_height(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_dome_height:%f", dome_height);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_request_mode:", 23) == 0)){
		char rqmd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(rqmd, cmd+23, numChars);
		mode = string_to_number(rqmd);
		EEPROM_enable();
		write_request_mode(mode, 0);
		answer_to_requests_enable = read_request_mode(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_request_mode:%d", answer_to_requests_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_ds18b20_mode:", 23) == 0)){
		char dsmd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(dsmd, cmd+23, numChars);
		mode = string_to_number(dsmd);
		EEPROM_enable();
		write_ds18b20_mode(mode, 0);
		ds18b20_measurements_enable = read_ds18b20_mode(0);
		EEPROM_disable();
		if (ds18b20_measurements_enable) ds18_first_meas_flag = 1; // enable first measurement to be accepted without difference check
		char sendStr[50] = {};
		sprintf(sendStr, "read_ds18b20_mode:%d", ds18b20_measurements_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_rate_of_change_mode:", 30) == 0)){
		char rtchmd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+30);
		strncpy(rtchmd, cmd+30, numChars);
		mode = string_to_number(rtchmd);
		EEPROM_enable();
		write_rate_of_change_mode(mode, 0);
		rate_of_change_enable = read_rate_of_change_mode(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_rate_of_change_mode:%d", rate_of_change_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_batch_data_send_mode:", 31) == 0)){
		char btchdtsndmd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+31);
		strncpy(btchdtsndmd, cmd+31, numChars);
		mode = string_to_number(btchdtsndmd);
		EEPROM_enable();
		write_batch_data_send_mode(mode, 0);
		batch_data_send_enable = read_batch_data_send_mode(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_batch_data_send_mode:%d", batch_data_send_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_log_requested_num_mode:", 33) == 0)){
		char lgrqstdnmmd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+33);
		strncpy(lgrqstdnmmd, cmd+33, numChars);
		mode = string_to_number(lgrqstdnmmd);
		EEPROM_enable();
		write_log_requested_num_mode(mode, 0);
		log_requested_num_en = read_log_requested_num_mode(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_log_requested_num_mode:%d", log_requested_num_en);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_standby_mode_enable:", 30) == 0)){
		char stndbymd[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+30);
		strncpy(stndbymd, cmd+30, numChars);
		mode = string_to_number(stndbymd);
		EEPROM_enable();
		write_standby_mode_enable(mode, 0);
		standby_mode_enable = read_standby_mode_enable(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_standby_mode_enable:%d", standby_mode_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_subscription_enable:", 30) == 0)){
		char subs[2] = {};
		uint8_t mode;

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+30);
		strncpy(subs, cmd+30, numChars);
		mode = string_to_number(subs);
		EEPROM_enable();
		write_subscription_enable(mode, 0);
		subscription_enable = read_subscription_enable(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_subscription_enable:%d", subscription_enable);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_wakeup_time:", 22) == 0)){
		struct standby_alarm wakeup = {};
		char time[20] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+22);
		strncpy(time, cmd+22, numChars);
		sscanf(time, "%02u:%02u:%02u", (uint *)&wakeup.hour, (uint *)&wakeup.minute, (uint *)&wakeup.second);
		EEPROM_enable();
		write_wakeup_time(wakeup, 0);
		wakeup_time = read_wakeup_time(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_wakeup_time:%02d:%02d:%02d", wakeup_time.hour, wakeup_time.minute, wakeup_time.second);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:write_standby_time:", 23) == 0)){
		struct standby_alarm standby = {};
		char time[20] = {};

		char* terminationPos = strchr(cmd, '\r');
		uint8_t numChars = terminationPos - (cmd+23);
		strncpy(time, cmd+23, numChars);
		sscanf(time, "%02u:%02u:%02u", (uint *)&standby.hour, (uint *)&standby.minute, (uint *)&standby.second);
		EEPROM_enable();
		write_standby_time(standby, 0);
		standby_time = read_standby_time(0);
		EEPROM_disable();
		char sendStr[50];
		sprintf(sendStr, "read_standby_time:%02d:%02d:%02d", standby_time.hour, standby_time.minute, standby_time.second);

		RTC_AlarmTypeDef sAlarm;
		sAlarm.Alarm = RTC_ALARM_A;
		sAlarm.AlarmTime.Hours = standby_time.hour;
		sAlarm.AlarmTime.Minutes = standby_time.minute;
		sAlarm.AlarmTime.Seconds = standby_time.second;
		HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);

		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:update_user:", 16) == 0)){

		char limits[2][50] = {};
		char uppers[50] = {}, lowers[50] = {};
		char number[15] = {};
		uint8_t matched_items = 0;
		char sendStr[250] = {};
		general_status ret;
		client client_data = {};
		char phone_str[9] = {};

		matched_items = sscanf(cmd, "cmd:update_user:%[^;];%[^;];%[^;];", number, limits[0], limits[1]);

		if((matched_items < 2) || (strlen(number) == 0)){
			goto report_parse_error;
		}

		ret = phone_number_transformation(number, phone_str);
		if(ret != OK) goto report_parse_error;

		if((strncmp(limits[0], "U:", 2) == 0) &&
		   (strncmp(limits[1], "L:", 2) == 0)){
			strcpy(uppers, limits[0]);
			strcpy(lowers, limits[1]);
		}else if((strncmp(limits[1], "U:", 2) == 0) &&
				 (strncmp(limits[0], "L:", 2) == 0)){
			strcpy(uppers, limits[1]+2);
			strcpy(lowers, limits[0]+2);
		}else{
			goto report_parse_error;
		}

		goto update_user;

		report_parse_error:
		sprintf(sendStr, "Error while parsing input data\nNumber: %s\nlimits[0]: %s\nlimits[1]: %s", number, limits[0], limits[1]);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
		goto end;

		update_user:

		ret = update_user(phone_str, lowers, uppers);
		if(ret != OK){
			sprintf(sendStr, "Error updating user\nError no: %d", ret);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			goto end;
		}

		ret = get_user_data(phone_str, NULL, &client_data);
		if(ret != OK){
			sprintf(sendStr, "Error while reading user\nError no: %d", ret);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			goto end;
		}

		// if client data is updated, update it in RAM also first by removing old data
		if(find_number_page(data.number_addr, phone_str) != NULL){
			ret = remove_client_data_from_RAM(&client_data, &data);

			if(ret != OK){
				sprintf(sendStr, "Error while removing user from RAM\nError no: %d", ret);
				if(!SIM800L_SMS(admin, sendStr, 1)){
					return SIM800_ERROR_SENDING_SMS;
				}
				goto end;
			}
		}

		ret = move_client_data_to_RAM(&client_data, &data);
		if(ret != OK){
			sprintf(sendStr, "Error while updating user data in RAM\nError no: %d", ret);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			goto end;
		}

		user_data_print_repr(&client_data, sendStr);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}

		D(printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   VOLUME  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_thresholds(data.volume_thresholds);

		D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   TIME   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_thresholds(data.time_thresholds);

		D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   ADDRESSES   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_number_page(data.number_addr);

		end:;

	}else if((strncmp(cmd, "cmd:delete_user:", 16) == 0)){

		general_status ret = 0;
		char number[20] = {};
		uint8_t matched_items = 0;
		char phone_str[9] = {};
		uint16_t current_page, page_end, page_start;
		client last_client_data = {}, cli_data = {};
		char sendStr[500];

		matched_items = sscanf(cmd, "cmd:delete_user:%[^\r\n]", number);

		if(matched_items != 1){
			goto report_parse_error_2;
		}

		ret = phone_number_transformation(number, phone_str);
		if(ret != OK) goto report_parse_error_2;

		goto delete_user;

		report_parse_error_2:
		sprintf(sendStr, "Error while parsing number\nNumber: %s\nError code: %d", number, ret);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
		goto end_2;


		delete_user:

		ret = get_user_data(phone_str, NULL, &cli_data);
		if(ret != OK){
			sprintf(sendStr, "Error while retrieving data from memory\nNumber: %s\nError code: %d", phone_str, ret);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			goto end_2;
		}
		read_subscr_meta_info(&page_start, &current_page, &page_end, 0);

		ret = delete_user("", &cli_data.page_address);
		if(ret != OK){
			sprintf(sendStr, "Error while deleting user\nNumber: %s\nError code: %d", phone_str, ret);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
			goto end_2;
		}

		ret = remove_client_data_from_RAM(&cli_data, &data);
		if(ret != OK) goto report_ram_error;

		if(current_page -  1 != cli_data.page_address){
			ret = get_user_data("", &cli_data.page_address, &last_client_data);
			if(ret != OK){
				sprintf(sendStr, "Error while retrieving data from memory on page: %d\n return code: %d", cli_data.page_address, ret);
				if(!SIM800L_SMS(admin, sendStr, 1)){
					return SIM800_ERROR_SENDING_SMS;
				}
				goto end_2;
			}

			ret = remove_client_data_from_RAM(&last_client_data, &data);
			if(ret != OK) goto report_ram_error;

			ret = move_client_data_to_RAM(&last_client_data, &data);
			if(ret != OK) goto report_ram_error;
		}

		sprintf(sendStr, "Successfully removed user %s", number);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}

		D(printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   VOLUME  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_thresholds(data.volume_thresholds);

		D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   TIME   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_thresholds(data.time_thresholds);

		D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   ADDRESSES   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
		print_number_page(data.number_addr);

		goto end_2;

		report_ram_error:
		sprintf(sendStr, "Error while doing operation in RAM\nError no: %d", ret);
		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}

		end_2:;

	}else if((strncmp(cmd, "cmd:SIM800L_SMS:", 16) == 0)){
		char number[20] = {};
		char message[1024] = {};
		char sendStr[1200];

		uint8_t parsed_items = sscanf(cmd, "cmd:SIM800L_SMS:%[^,],%1024c", number, message);

		if(parsed_items == 2){
			D(printf("number: %s\n", number));
			D(printf("message: %s\n", message));

			if(!SIM800L_SMS(number, message, 0)){
				return SIM800_ERROR_SENDING_SMS;
			}

			sprintf(sendStr, "Message sended successfully\nnumber: %s\ncontent: %s", number, message);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
		}else{
			sprintf(sendStr, "Parse unsuccessful\nnumber: %s\ncontent: %s\nmatched_items: %d\n", number, message, parsed_items);
			if(!SIM800L_SMS(admin, sendStr, 1)){
				return SIM800_ERROR_SENDING_SMS;
			}
		}
	}else if((strncmp(cmd, "cmd:restart_STM", 15) == 0)){
		restart_flag = 1;
	}else if((strncmp(cmd, "cmd:read_all_conf", 17) == 0)){
		char confs[CONF_REPORT_MAX_LENGTH];
		read_all_conf(confs);
		if(!SIM800L_SMS(admin, confs, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:read_sim800", 15) == 0)){
		char sim800[100] = {};
		uint8_t rssi, ber, bcs, bcl;
		uint16_t voltage;

		general_status ret = SIM800L_CSQ(&rssi, &ber);
		if(ret != OK) return ret;
		ret = SIM800L_CBC(&bcs, &bcl, &voltage);
		if(ret != OK) return ret;

		sprintf(sim800, "+CSQ: %d,%d\n+CBC: %d,%d,%d", rssi, ber, bcs, bcl, voltage);
		if(!SIM800L_SMS(admin, sim800, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:read_STM", 12) == 0)){
		char STM[200] = {};
		struct adcMeas adc_meas;
		general_status ret;
		uint8_t solar_prec = HAL_GPIO_ReadPin(SOLAR_PRESENSE_GPIO_Port, SOLAR_PRESENSE_Pin);

		ret = getADCMeasurements(&adc_meas);
		if(ret != OK){
			return ret;
		}

		sprintf(STM, "Lopo battery: %.4f\nSolar voltage: %.4f\nSTM temp: %.4f\nA02YYUW measurement: %d\nDS18B20 temp: %.4f\n",
													adc_meas.lipo,
													adc_meas.solar,
													adc_meas.temp,
													a02yyuw_measurement,
													tank_temp);
		if(solar_prec){
			strcat(STM, "Solar power absent\n");
		}else{
			strcat(STM, "Solar power present\n");
		}

		if(ds18b20_precense_flag){
			strcat(STM, "DS18B20 present");
		}else{
			strcat(STM, "DS18B20 absent");
		}

		D(printf("%s\n", STM));
		if(!SIM800L_SMS(admin, STM, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}else if((strncmp(cmd, "cmd:read_req_nums", 17) == 0)){

		if(!SIM800L_SMS(admin, req_nums, 0)){
			return SIM800_ERROR_SENDING_SMS;
		}

	}else{
		char sendStr[120] = {};
		strcpy(sendStr, "Unknown command");
		D(printf("%s\n", sendStr));

		if(!SIM800L_SMS(admin, sendStr, 1)){
			return SIM800_ERROR_SENDING_SMS;
		}
	}

	return OK;
}


general_status process_client_request(char* client, char* message){

	char text_format[500] = "avzSi wylis mimdinare moculobaa %.0f litri";
	char text[500] = {};

	if(curr_volume > 1000){
		sprintf(text, "avzSi wylis mimdinare moculobaa %d tona da %d litri", (int)curr_volume/1000, (int)curr_volume%1000);
	}else{
		sprintf(text, text_format, curr_volume);
	}

	general_status ret;
	uint8_t valid_client_flag = 0;

	time_t remaining_time;
	struct tm remaining_time_tm;
	int8_t direction;
	predictEmptyOrFill(accum_slope, &remaining_time, &direction);
	gmtime_r(&remaining_time, &remaining_time_tm);


	if(ds18b20_measurements_enable){
		sprintf(text_format, ", temperatura: %.2f{2103}", tank_temp);
		strcat(text, text_format);
	}

	if(rate_of_change_enable){
		if(remaining_time_tm.tm_mday > 1){
			// wylis done titqmis ucvleli
			strcat(text, ". wylis done avzSi TiTqmis ucvlelia");

		}else{
			char aivs_daicl[15] = {};


			if(direction == 1) strcpy(aivs_daicl, "aivseba");
			else if(direction == -1) strcpy(aivs_daicl, "daicleba");

			sprintf(text_format, ", tempi: %.4f litri/wamSi. am tempiT avzi %s", accum_slope, aivs_daicl);
			strcat(text, text_format);

			if(remaining_time_tm.tm_hour != 0 && remaining_time_tm.tm_min != 0){
				sprintf(text_format, " %d saaTsa da %d wuTSi", remaining_time_tm.tm_hour, remaining_time_tm.tm_min);
				strcat(text, text_format);
			}else if(remaining_time_tm.tm_hour != 0){
				sprintf(text_format, " %d saaTSi", remaining_time_tm.tm_hour);
				strcat(text, text_format);
			}else if(remaining_time_tm.tm_min != 0){
				sprintf(text_format, " %d wuTSi", remaining_time_tm.tm_min);
				strcat(text, text_format);
			}else if(remaining_time_tm.tm_sec != 0){
				sprintf(text_format, "  wamebSi");
				strcat(text, text_format);
			}
		}
	}



	if((strstr(message, key_word_geo[0]) != NULL) || (strstr(message, key_word_geo[1]) != NULL)){
		valid_client_flag = 1;
		ret = SIM800L_SMS_GEO(client, text, 0);
		if(ret != OK){
			D(printf("Error sending Geo: %d", ret));
			return SIM800_ERROR_SENDING_SMS;
		}
	}else{
		toLowercase(message);

		if((strstr(message, key_word_lat[0]) != NULL) || (strstr(message, key_word_lat[1]) != NULL)){
			valid_client_flag = 1;
			memmove(text+1, text, strlen(text));
			text[0] = '[';
			strcat(text, "]");
			ret = SIM800L_SMS_GEO(client, text, 0);
			if(ret != OK){
				D(printf("Error sending Geo: %d", ret));
				return SIM800_ERROR_SENDING_SMS;
			}
		}
	}

	if(valid_client_flag && log_requested_num_en){
		// log client number and send if necessary
		if(REQ_NUM_SIZE - strlen(req_nums) > strlen(client)){
			if(strlen(req_nums) == 0){
				strcat(req_nums, client);
			}else{
				strcat(req_nums, ",");
				strcat(req_nums, client);
			}
		}else{
			if(!SIM800L_SMS(admins[0], req_nums, 0)){
				return SIM800_ERROR_SENDING_SMS;
			}
			memset(req_nums, 0, REQ_NUM_SIZE);
			strcat(req_nums, client);
		}
	}

	return OK;

}

void toLowercase(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        str[i] = tolower(str[i]);
        i++;
    }
}

