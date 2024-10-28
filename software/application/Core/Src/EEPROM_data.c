/*
 * NUM PAGES 128
 * PAGE SIZE 32 bytes
 *
 *      OFFSET        0  		 			1     				2     3    	       4			  5		6	7	        8	             9	10	11	             12	              13	14	 15	              16	               17	18	  19	             20	                   21	22	23	          24              25  26 27				28			29	30	31
 *
 * PAGE 0:      bootloader run     wokeup from bootloader                    mobile country code                   standby_mode_enable                 answer_to_requests_enable                   enable ds18b20 measurements				            rate_of_change_enable                        batch_data_send_enable				   log_requested_num_en
 * PAGE 1:      APN (whole page)
 * PAGE 2:      server address (whole page)
 * PAGE 3:      server address (whole page)
 * PAGE 4:      username (whole page)
 * PAGE 5:      password (whole page)
 * PAGE 6:      file name (whole page)
 * PAGE 7:      file name (whole page)
 * PAGE 8:      file path (whole page)
 * PAGE 9:		file path (whole page)
 * PAGE 10:     file path (whole page)
 * PAGE 11:     local file name (whole page)
 * PAGE 12:     local file name (whole page)
 * PAGE 13:     admin number (whole page 4 numbers max)
 * PAGE 14:     provider identifier string 1 (whole page)
 * PAGE 15:     provider identifier string 2 (whole page)
 * PAGE 16:		min_distance(2 bytes)																																		max_distance (2 bytes)
 * PAGE 17:		cylinder_radius (4 bytes)		 cylinder_height (4 bytes)              dome_height (4 bytes)
 * PAGE 18:		wakeup_time (3 bytes, hour, minute, second)							standby_time (3 bytes, hour, minute, second)
 * PAGE 19:
 * PAGE 20:
 * PAGE 21: 	key_word 1 (whole page)
 * PAGE 22:		key_word 2 (whole page)
 * PAGE 23:
 * PAGE 24:     subscription_enable											page_start								current_page						   page_end
 * PAGE 25:		0-7:user_number(8 bytes), 8:enable(1byte), 9-14:lower_limits(6bytes), 15-20:upper_limits(6bytes), 21-22:send_count(2bytes), 23-30:last_reached_time(8bytes)
 * .
 * .
 * .
 * PAGE 127:	0-7:user_number(8 bytes), 8:enable(1byte), 9-14:lower_limits(6bytes), 15-20:upper_limits(6bytes), 21-22:send_count(2bytes), 23-30:last_reached_time(8bytes)
 */

#include "EEPROM.h"
#include "main.h"
#include "string.h"
#include "stdio.h"
#include "EEPROM_data.h"
#include "SIM800L.h"
#include "Process_Data.h"
#include "time_my.h"

#define PAGE_SIZE 32

char admins[MAX_ADMINS][14] = {};
char provider[2][32] = {};

char key_word_geo[2][128] = {};
char key_word_lat[2][32] = {};

extern uint8_t bootloader_restart;
extern uint8_t answer_to_requests_enable;
extern uint8_t ds18b20_measurements_enable;
extern uint8_t log_requested_num_en;
extern uint8_t subscription_enable;
extern char mobile_country_code[5];

extern uint8_t rate_of_change_enable;
extern uint8_t batch_data_send_enable;
extern volatile uint16_t min_distance;
extern volatile uint16_t max_distance;
extern volatile float cylinder_height;
extern volatile float cylinder_radius;
extern volatile float dome_height;

uint8_t standby_mode_enable = 0;
extern struct standby_alarm wakeup_time;
extern struct standby_alarm standby_time;

/*
 *  mode: if 1, bootloader will search for new firmware file in SIM800 file system after jumping to bootloder
 */
void EEPROM_enable(){
	HAL_GPIO_WritePin(EEPROM_CONTROL_GPIO_Port, EEPROM_CONTROL_Pin, 0);
	HAL_Delay(10);
}

void EEPROM_disable(){
	HAL_Delay(10);
//	HAL_GPIO_WritePin(EEPROM_CONTROL_GPIO_Port, EEPROM_CONTROL_Pin, 1);
}

uint8_t write_bootloder_enable(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 0, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_bootloder_enable(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 0, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_bootloder_wakeup(uint8_t flag, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 1, &flag, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_bootloder_wakeup(uint8_t auto_en){
	uint8_t flag;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 1, &flag, 1);
	if(auto_en) EEPROM_disable();
	return flag;
}

uint8_t write_request_mode(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 12, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_request_mode(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 12, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_ds18b20_mode(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 16, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_ds18b20_mode(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 16, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_rate_of_change_mode(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 20, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_rate_of_change_mode(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 20, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_batch_data_send_mode(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 24, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_batch_data_send_mode(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 24, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}


uint8_t write_log_requested_num_mode(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 28, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_log_requested_num_mode(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 28, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_standby_mode_enable(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(0, 8, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_standby_mode_enable(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 8, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_subscription_enable(uint8_t mode, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(24, 0, &mode, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t read_subscription_enable(uint8_t auto_en){
	uint8_t mode;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(24, 0, &mode, 1);
	if(auto_en) EEPROM_disable();
	return mode;
}

uint8_t write_current_page(uint16_t curr_page, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(24, 8, (uint8_t *)&curr_page, 2);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint16_t read_current_page(uint8_t auto_en){
	uint16_t curr_page;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(24, 8, (uint8_t *)&curr_page, 2);
	if(auto_en) EEPROM_disable();
	return curr_page;
}

uint8_t write_page_start(uint16_t page_start, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(24, 4, (uint8_t *)&page_start, 2);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint16_t read_page_start(uint8_t auto_en){
	uint16_t page_start;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(24, 4, (uint8_t *)&page_start, 2);
	if(auto_en) EEPROM_disable();
	return page_start;
}

uint8_t write_page_end(uint16_t page_end, uint8_t auto_en){
	if(auto_en) EEPROM_enable();
	EEPROM_Write(24, 12, (uint8_t *)&page_end, 2);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint16_t read_page_end(uint8_t auto_en){
	uint16_t page_end;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(24, 12, (uint8_t *)&page_end, 2);
	if(auto_en) EEPROM_disable();
	return page_end;
}

uint8_t read_subscr_meta_info(uint16_t* page_start, uint16_t* current_page, uint16_t* page_end, uint8_t auto_en){
	char page_data[PAGE_SIZE];
	if(auto_en) EEPROM_enable();
	EEPROM_Read(24, 0, (uint8_t *)page_data, PAGE_SIZE);
	*page_start = ((*(page_data+4)) | *(page_data+5) >> 8);
	*current_page = ((*(page_data+8)) | *(page_data+9) >> 8);
	*page_end = ((*(page_data+12)) | *(page_data+13) >> 8);
	if(auto_en) EEPROM_disable();
	return 1;
}

/*
 * mobile country code format: 9955 (without + sign)
 * mobile country code at page 0 offset 4
 * returns 1 if successful
 */
uint8_t write_mobile_country_code(char * code, uint8_t auto_en){
	if(strlen(code) == 4){
		if(auto_en) EEPROM_enable();
		EEPROM_Write(0, 4, (uint8_t *) code, 4);
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return 0;
}

uint8_t read_mobile_country_code(char * code, uint8_t auto_en){
	memset(code, 0, 5);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(0, 4, (uint8_t *) code, 4);
	if(auto_en) EEPROM_disable();
	return 1;
}

/*
 * read APN from EEPROM into 32byte long APN variable
 */
uint8_t read_APN(char * APN, uint8_t auto_en){
	memset(APN, 0, 32);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(1, 0, (uint8_t *)APN, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}

/*
 * write max 32-1 character long APN into EEPROM memory
 */
uint8_t write_APN(char * APN, uint8_t auto_en){
	if (strlen(APN) <= 31){
		char empty[32] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(1, 0, (uint8_t *)empty, 32);
		EEPROM_Write(1, 0, (uint8_t *)APN, strlen(APN));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

/*
 * read 64-1 character long server address into variable
 */
uint8_t read_server_address(char * server_address, uint8_t auto_en){
	memset(server_address, 0, 64);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(2, 0, (uint8_t *)server_address, 64);
	if(auto_en) EEPROM_disable();
	return 1;
}

/*
 * write 64-1 character long server address into EEPROM memory
 */
uint8_t write_server_adress(char * server_address, uint8_t auto_en){
	if(strlen(server_address) <= 63){
		char empty[64] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(2, 0, (uint8_t *)empty, 64);
		EEPROM_Write(2, 0, (uint8_t *)server_address, strlen(server_address));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_username(char * username, uint8_t auto_en){
	memset(username, 0, 32);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(4, 0, (uint8_t *)username, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t write_username(char * username, uint8_t auto_en){
	if(strlen(username) <= 31){
		char empty[32] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(4, 0, (uint8_t *)empty, 32);
		EEPROM_Write(4, 0, (uint8_t *)username, strlen(username));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_password(char * password, uint8_t auto_en){
	memset(password, 0, 32);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(5, 0, (uint8_t *)password, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t write_password(char * password, uint8_t auto_en){
	if(strlen(password) <= 31){
		char empty[32] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(5, 0, (uint8_t *)empty, 32);
		EEPROM_Write(5, 0, (uint8_t *)password, strlen(password));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_file_name(char * file_name, uint8_t auto_en){
	memset(file_name, 0, 64);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(6, 0, (uint8_t *)file_name, 64);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t write_file_name(char * file_name, uint8_t auto_en){
	if(strlen(file_name) <= 63){
		char empty[64] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(6, 0, (uint8_t *)empty, 64);
		EEPROM_Write(6, 0, (uint8_t *)file_name, strlen(file_name));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_file_path(char * file_path, uint8_t auto_en){
	memset(file_path, 0, 96);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(8, 0, (uint8_t *)file_path, 96);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t write_file_path(char * file_path, uint8_t auto_en){
	if(strlen(file_path) <= 95){
		char empty[96] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(8, 0, (uint8_t *)empty, 96);
		EEPROM_Write(8, 0, (uint8_t *)file_path, strlen(file_path));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_local_file_name(char * loc_file_name, uint8_t auto_en){
	memset(loc_file_name, 0, 64);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(11, 0, (uint8_t *)loc_file_name, 64);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint8_t write_local_file_name(char * loc_file_name, uint8_t auto_en){
	if(strlen(loc_file_name) <= 63){
		char empty[64] = {};
		if(auto_en) EEPROM_enable();
		EEPROM_Write(11, 0, (uint8_t *)empty, 64);
		EEPROM_Write(11, 0, (uint8_t *)loc_file_name, strlen(loc_file_name));
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}

/*
 * number must be at least 14 characters long variable
 * reads mobile number from specified memory address with 13 digit format: +9955 ** ** ** **
 * returns 0 if no record exists
 */
uint8_t read_mobile_num(char * number, uint16_t page, uint16_t offset, uint8_t auto_en){
	memset(number, 0, 14);
	char num[9] = {};
	char code[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Read(page, offset, (uint8_t *)num, 8);
	if(auto_en) EEPROM_disable();
	if(strlen(num) == 0) return 0;
//	read_mobile_country_code(code, 0);
	strcpy(code, mobile_country_code);
	strcat(number, "+");
	strcat(number, code);
	strcat(number, num);
	return 1;
}

/*
 * writes number into EEPROM memory
 * accepts following formats:
 * 								+995 5 ** ** ** **
 * 								 995 5 ** ** ** **
 * 								     5 ** ** ** **
 * 								       ** ** ** **
 * returns -1 if error
 */
uint8_t write_mobile_num(char * number, uint16_t page, uint16_t offset, uint8_t auto_en){
	char num[8] = {};
	if(strlen(number) == 13){
		strncpy(num, number+5, 8);
	}else if (strlen(number) == 12){
		strncpy(num, number+4, 8);
	}else if (strlen(number) == 9){
		strncpy(num, number+1, 8);
	}else if (strlen(number) == 8){
		strncpy(num, number, 8);
	}else return -1;
	if(auto_en) EEPROM_enable();
	EEPROM_Write(page, offset, (uint8_t *)num, 8);
	if(auto_en) EEPROM_disable();
	return 1;
}

/*
 * index specifies admin 0, 1, 2 or 3
 */
uint8_t read_admin_number(char * admin_number, uint8_t index, uint8_t auto_en){
	if (read_mobile_num(admin_number, 13, index * 8, auto_en)) return 1;
	return 0;
}

/*
 * index specifies admin 0, 1, 2 or 3
 */
uint8_t write_admin_number(char * admin_number, uint8_t index, uint8_t auto_en){
	if(index < 4){
		if (write_mobile_num(admin_number, 13, index * 8, auto_en)){
			return 1;
		}
	}
	return -1;
}


/*
 * index specifies admin 1, 2 or 3
 */
uint8_t clear_admin_number(uint8_t index, uint8_t auto_en){
	char empty[8] = {};
	if(index > 0 && index < 4){
		if(auto_en) EEPROM_enable();
		EEPROM_Write(13, index * 8, (uint8_t *)empty, 8);
		if(auto_en) EEPROM_disable();
		return 1;
	}
	return -1;
}


uint8_t write_provider_identifier1(char* provider, uint8_t auto_en){
	char empty[32] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(14, 0, (uint8_t *)empty, 32);
	EEPROM_Write(14, 0, (uint8_t *)provider, strlen(provider));
	if(auto_en) EEPROM_disable();
	return 1;
}


uint8_t write_provider_identifier2(char* provider, uint8_t auto_en){
	char empty[32] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(15, 0, (uint8_t *)empty, 32);
	EEPROM_Write(15, 0, (uint8_t *)provider, strlen(provider));
	if(auto_en) EEPROM_disable();
	return 1;
}


uint8_t read_provider_identifier1(char* provider, uint8_t auto_en){
	memset(provider, 0, 32);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(14, 0, (uint8_t *)provider, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}


uint8_t read_provider_identifier2(char* provider, uint8_t auto_en){
	memset(provider, 0, 32);
	if(auto_en) EEPROM_enable();
	EEPROM_Read(15, 0, (uint8_t *)provider, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}


uint8_t write_min_distance(uint16_t mndstance, uint8_t auto_en){
	char empty[3] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(16, 0, (uint8_t *)empty, 2);
	EEPROM_Write(16, 0, (uint8_t *)&mndstance, 2);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint16_t read_min_distance(uint8_t auto_en){
	uint16_t mndstance;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(16, 0, (uint8_t *)&mndstance, 2);
	if(auto_en) EEPROM_disable();
	return mndstance;
}


uint8_t write_max_distance(uint16_t mxdstance, uint8_t auto_en){
	char empty[3] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(16, 16, (uint8_t *)empty, 2);
	EEPROM_Write(16, 16, (uint8_t *)&mxdstance, 2);
	if(auto_en) EEPROM_disable();
	return 1;
}

uint16_t read_max_distance(uint8_t auto_en){
	uint16_t mxdstance;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(16, 16, (uint8_t *)&mxdstance, 2);
	if(auto_en) EEPROM_disable();
	return mxdstance;
}

uint8_t write_cylinder_radius(float radius, uint8_t auto_en){
	char empty[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(17, 0, (uint8_t *)empty, 4);
	EEPROM_Write_NUM(17, 0, radius);
	if(auto_en) EEPROM_disable();
	return 1;
}

float read_cylinder_radius(uint8_t auto_en){
	float radius;
	if(auto_en) EEPROM_enable();
	radius = EEPROM_Read_NUM(17, 0);
	if(auto_en) EEPROM_disable();
	return radius;
}


uint8_t write_cylinder_height(float height, uint8_t auto_en){
	char empty[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(17, 4, (uint8_t *)empty, 4);
	EEPROM_Write_NUM(17, 4, height);
	if(auto_en) EEPROM_disable();
	return 1;
}

float read_cylinder_height(uint8_t auto_en){
	float height;
	if(auto_en) EEPROM_enable();
	height = EEPROM_Read_NUM(17, 4);
	if(auto_en) EEPROM_disable();
	return height;
}


uint8_t write_dome_height(float height, uint8_t auto_en){
	char empty[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(17, 8, (uint8_t *)empty, 4);
	EEPROM_Write_NUM(17, 8, height);
	if(auto_en) EEPROM_disable();
	return 1;
}

float read_dome_height(uint8_t auto_en){
	float height;
	if(auto_en) EEPROM_enable();
	height = EEPROM_Read_NUM(17, 8);
	if(auto_en) EEPROM_disable();
	return height;
}

uint8_t write_wakeup_time(struct standby_alarm wake_time, uint8_t auto_en){
	char empty[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(18, 0, (uint8_t *)empty, 3);
	EEPROM_Write(18, 0, &wake_time.hour, 1);
	EEPROM_Write(18, 1, &wake_time.minute, 1);
	EEPROM_Write(18, 2, &wake_time.second, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

struct standby_alarm read_wakeup_time(uint8_t auto_en){
	struct standby_alarm wake_time;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(18, 0, &wake_time.hour, 1);
	EEPROM_Read(18, 1, &wake_time.minute, 1);
	EEPROM_Read(18, 2, &wake_time.second, 1);
	if(auto_en) EEPROM_disable();
	return wake_time;
}

uint8_t write_standby_time(struct standby_alarm standby, uint8_t auto_en){
	char empty[5] = {};
	if(auto_en) EEPROM_enable();
	EEPROM_Write(18, 8, (uint8_t *)empty, 3);
	EEPROM_Write(18, 8, &standby.hour, 1);
	EEPROM_Write(18, 9, &standby.minute, 1);
	EEPROM_Write(18, 10, &standby.second, 1);
	if(auto_en) EEPROM_disable();
	return 1;
}

struct standby_alarm read_standby_time(uint8_t auto_en){
	struct standby_alarm standby;
	if(auto_en) EEPROM_enable();
	EEPROM_Read(18, 8, &standby.hour, 1);
	EEPROM_Read(18, 9, &standby.minute, 1);
	EEPROM_Read(18, 10, &standby.second, 1);
	if(auto_en) EEPROM_disable();
	return standby;
}

general_status read_key_word(uint8_t index, char* key_word, uint8_t auto_en){
	memset(key_word, 0, 32);
	if(index < 0 || index > 1){
		return KEY_WORD_INDEX_OUT_OF_RANGE;
	}
	if(auto_en) EEPROM_enable();
	EEPROM_Read(21+index, 0, (uint8_t *)key_word, 32);
	if(auto_en) EEPROM_disable();
	return 1;
}


general_status write_key_word(uint8_t index, char* key_word, uint8_t auto_en){
	char empty[32] = {};
	if(index < 0 || index > 1){
		return KEY_WORD_INDEX_OUT_OF_RANGE;
	}
	if(auto_en) EEPROM_enable();
	EEPROM_Write(21+index, 0, (uint8_t *)empty, 32);
	EEPROM_Write(21+index, 0, (uint8_t *)key_word, strlen(key_word));
	if(auto_en) EEPROM_disable();

	return OK;
}


uint8_t read_all_conf(char* conf){
	EEPROM_enable();
	D(printf("------------------> Reading All Configuration Parameters <------------------\n"));

	memset(conf, 0, CONF_REPORT_MAX_LENGTH);
//	sprintf(conf, "");
	bootloader_restart = read_bootloder_enable(0);
	sprintf(conf + strlen(conf), "bootl:     %d\n", bootloader_restart);  // bootloader
	answer_to_requests_enable = read_request_mode(0);
	sprintf(conf + strlen(conf), "req en:    %d\n", answer_to_requests_enable);  // answer to requests enable
	ds18b20_measurements_enable = read_ds18b20_mode(0);
	sprintf(conf + strlen(conf), "ds18 en:   %d\n", ds18b20_measurements_enable);

	rate_of_change_enable = read_rate_of_change_mode(0);
	sprintf(conf + strlen(conf), "rate en:   %d\n", rate_of_change_enable);  // rate of change mode enable
	batch_data_send_enable = read_batch_data_send_mode(0);
	sprintf(conf + strlen(conf), "batch en:  %d\n", batch_data_send_enable);   // batch data send enable
	log_requested_num_en = read_log_requested_num_mode(0);
	sprintf(conf + strlen(conf), "req log:   %d\n", log_requested_num_en);   // log requested numbers enable
	standby_mode_enable = read_standby_mode_enable(0);
	sprintf(conf + strlen(conf), "standby:   %d\n", standby_mode_enable);   // standby mode enable
	subscription_enable = read_subscription_enable(0);
	sprintf(conf + strlen(conf), "subs en:   %d\n", subscription_enable);
	min_distance = read_min_distance(0);
	sprintf(conf + strlen(conf), "min d:     %d\n", min_distance);  // min distance
	max_distance = read_max_distance(0);
	sprintf(conf + strlen(conf), "max d:     %d\n", max_distance);  // max distance
	cylinder_height = read_cylinder_height(0);
	sprintf(conf + strlen(conf), "cyl H:     %f\n", cylinder_height);  // cylinder height
	cylinder_radius = read_cylinder_radius(0);
	sprintf(conf + strlen(conf), "cyl R:     %f\n", cylinder_radius);  // cylinder radius
	dome_height = read_dome_height(0);
	sprintf(conf + strlen(conf), "dome H:    %f\n", dome_height);  // done height

	read_mobile_country_code(mobile_country_code, 0);
	sprintf(conf + strlen(conf), "countryC:  %s\n", mobile_country_code);  // mobile country code
	char data[200];
	read_APN(data, 0);
	sprintf(conf + strlen(conf), "APN:       %s\n", data);
	read_server_address(data, 0);
	sprintf(conf + strlen(conf), "Serv addr: %s\n", data);   // server address
	read_username(data, 0);
	sprintf(conf + strlen(conf), "Usern:     %s\n", data);  // username
	read_password(data, 0);
	sprintf(conf + strlen(conf), "Pass:      %s\n", data);  // password
	read_file_name(data, 0);
	sprintf(conf + strlen(conf), "FileN:     %s\n", data); // filename
	read_file_path(data, 0);
	sprintf(conf + strlen(conf), "FileP:     %s\n", data);  // filepath
	read_local_file_name(data, 0);
	sprintf(conf + strlen(conf), "L FineN:   %s\n", data);   // local filename
	for(uint8_t i=0; i<4; i++){
		read_admin_number(data, i, 0);
		strcpy(admins[i], data);
		sprintf(conf + strlen(conf), "Admin %d:   %s\n", i, data);   // admin
	}
	read_provider_identifier1(data, 0);
	strcpy(provider[0], data);
	sprintf(conf + strlen(conf), "Prov 1:    %s\n", provider[0]);   // provider identifier 1
	read_provider_identifier2(data, 0);
	strcpy(provider[1], data);
	sprintf(conf + strlen(conf), "Prov 2:    %s\n", provider[1]);   // provider identifier 2

	for(uint8_t i=0; i<2; i++){
		read_key_word(i, data, 0);
		ToUTF16(data, key_word_geo[i]);
		toLowercase(data);
		strcpy(key_word_lat[i], data);
		sprintf(conf + strlen(conf), "KeyW%d:     %s\n", i, data);  // keyword
	}

	wakeup_time = read_wakeup_time(0);
	standby_time = read_standby_time(0);
	sprintf(conf + strlen(conf), "wakeup:    %02d:%02d:%02d\n", wakeup_time.hour, wakeup_time.minute, wakeup_time.second);
	sprintf(conf + strlen(conf), "stndby:    %02d:%02d:%02d\n", standby_time.hour, standby_time.minute, standby_time.second);


	D(printf("%s", conf));

	EEPROM_disable();
	return 1;
}


general_status EEPROM_set_to_default(){
	EEPROM_enable();
	write_request_mode(1, 0);
	write_ds18b20_mode(0, 0);
	write_rate_of_change_mode(1, 0);
	write_batch_data_send_mode(1, 0);
	write_log_requested_num_mode(0, 0);
	write_standby_mode_enable(1, 0);
	write_subscription_enable(1, 0);
	write_min_distance(30, 0);
	write_max_distance(1850, 0);
	write_cylinder_height(1.63, 0);
	write_cylinder_radius(0.947768, 0);
	write_dome_height(0.22, 0);
	write_mobile_country_code("9955", 0);
	write_APN("3g.ge", 0);
	write_server_adress("ftp.drivehq.com", 0);
	write_username("ShalvaButkhuzi", 0);
	write_password("password", 0);
	write_file_name("new_firmware.bin", 0);
	write_file_path("/Bootloader/", 0);
	write_local_file_name("new_firmware.bin", 0);
	write_provider_identifier1("w41474459434@4w4", 0);
	write_provider_identifier2("MAGTICOM", 0);

	struct standby_alarm wakeup, standby;
	wakeup.hour = 6;		standby.hour = 23;
	wakeup.minute = 0;		standby.minute = 59;
	wakeup.second = 0;		standby.second = 0;
	write_wakeup_time(wakeup, 0);
	write_standby_time(standby, 0);

	EEPROM_disable();

	return OK;
}


void display_mem(void* mem, int mem_size, int line_len) {
   /*
		mem 		- pointer to beggining of memory region to be printed
		mem_size 	- number of bytes mem points to
		line_len	- number of bytyes to display per line
   */

 	unsigned char* data = mem;
 	int full_lines = mem_size / line_len;
 	unsigned char* addr = mem;

 	for (int linno = 0; linno < full_lines; linno++) {
 		// Print Address
 		printf("0x%x\t", (unsigned int)addr);

 		// Print Hex
 		for (int i = 0; i < line_len; i++) {
 			printf(" %02x", data[linno*line_len + i]);
 		}
 		printf("\t");

 		// Print Ascii
 		for (int i = 0; i < line_len; i++) {
 		    char c = data[linno*line_len + i];
 		    if ( 32 < c && c < 125) {
 			    printf(" %c", c);
 		    }
 		    else {
 		        printf(" .");
 		    }
 		}
 		printf("\n");

 		addr += line_len;
 	}

 	// Print any remaining bytes that couldn't make a full line
 	int remaining = mem_size % line_len;
 	if (remaining > 0) {
 	    // Print Address
 	    printf("0x%x\t", (unsigned int)addr);

 	    // Print Hex
 	    for (int i = 0; i < remaining; i++) {
 	        printf(" %02x", data[line_len*full_lines + i]);
    	}
    	for (int i = 0; i < line_len - remaining; i++) {
    	    printf("  ");
    	}
    	printf("\t");

    	// Print Hex
 	    for (int i = 0; i < remaining; i++) {
 	        char c = data[line_len*full_lines + i];
 	        if ( 32 < c && c < 125) {
 			    printf(" %c", c);
 		    }
 		    else {
 		        printf(" .");
 		    }
    	}
    	printf("\n");
 	}
 }
