/*
 * NUM PAGES 128
 * PAGE SIZE 32 bytes
 *
 *      OFFSET        0  		 1     2     3    	       4			  5		6	7	8	9	10	11	12	13	14	15	16	17	18	19	20	21	22	23	24	25	26	27	28	29	30	31
 *
 * PAGE 0:      bootloader run                     mobile country code
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
 *
 */

#include "EEPROM.h"
#include "main.h"
#include "string.h"

/*
 *  mode: if 1, bootloader will search for new firmware file in SIM800 file system after jumping to bootloder
 */
void EEPROM_enable(){
	HAL_GPIO_WritePin(EEPROM_CONTROL_GPIO_Port, EEPROM_CONTROL_Pin, 0);
	HAL_Delay(20);
}

void EEPROM_disable(){
	HAL_GPIO_WritePin(EEPROM_CONTROL_GPIO_Port, EEPROM_CONTROL_Pin, 1);
	HAL_Delay(20);
}

uint8_t write_bootloder_enable(uint8_t mode){
	EEPROM_enable();
	EEPROM_Write(0, 0, &mode, 1);
	EEPROM_disable();
	return 1;
}

uint8_t read_bootloder_enable(){
	uint8_t mode;
	EEPROM_enable();
	EEPROM_Read(0, 0, &mode, 1);
	EEPROM_disable();
	return mode;
}

uint8_t write_bootloder_wakeup(uint8_t flag){
	EEPROM_enable();
	EEPROM_Write(0, 1, &flag, 1);
	EEPROM_disable();
	return 1;
}

/*
 * mobile country code format: 9955 (without + sign)
 * mobile country code at page 0 offset 4
 * returns 1 if successful
 */
uint8_t write_mobile_country_code(char * code){
	if(strlen(code) == 4){
		EEPROM_enable();
		EEPROM_Write(0, 4, (uint8_t *) code, 4);
		EEPROM_disable();
		return 1;
	}
	return 0;
}

uint8_t read_mobile_country_code(char * code){
	memset(code, 0, 5);
	EEPROM_enable();
	EEPROM_Read(0, 4, (uint8_t *) code, 4);
	EEPROM_disable();
	return 1;
}

/*
 * read APN from EEPROM into 32byte long APN variable
 */
uint8_t read_APN(char * APN){
	memset(APN, 0, 32);
	EEPROM_enable();
	EEPROM_Read(1, 0, (uint8_t *)APN, 32);
	EEPROM_disable();
	return 1;
}

/*
 * write max 32-1 character long APN into EEPROM memory
 */
uint8_t write_APN(char * APN){
	if (strlen(APN) <= 31){
		char empty[32] = {};
		EEPROM_enable();
		EEPROM_Write(1, 0, (uint8_t *)empty, 32);
		EEPROM_Write(1, 0, (uint8_t *)APN, strlen(APN));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

/*
 * read 64-1 character long server address into variable
 */
uint8_t read_server_address(char * server_address){
	memset(server_address, 0, 64);
	EEPROM_enable();
	EEPROM_Read(2, 0, (uint8_t *)server_address, 64);
	EEPROM_disable();
	return 1;
}

/*
 * write 64-1 character long server address into EEPROM memory
 */
uint8_t write_server_adress(char * server_address){
	if(strlen(server_address) <= 63){
		char empty[64] = {};
		EEPROM_enable();
		EEPROM_Write(2, 0, (uint8_t *)empty, 64);
		EEPROM_Write(2, 0, (uint8_t *)server_address, strlen(server_address));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_username(char * username){
	memset(username, 0, 32);
	EEPROM_enable();
	EEPROM_Read(4, 0, (uint8_t *)username, 32);
	EEPROM_disable();
	return 1;
}

uint8_t write_username(char * username){
	if(strlen(username) <= 31){
		char empty[32] = {};
		EEPROM_enable();
		EEPROM_Write(4, 0, (uint8_t *)empty, 32);
		EEPROM_Write(4, 0, (uint8_t *)username, strlen(username));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_password(char * password){
	memset(password, 0, 32);
	EEPROM_enable();
	EEPROM_Read(5, 0, (uint8_t *)password, 32);
	EEPROM_disable();
	return 1;
}

uint8_t write_password(char * password){
	if(strlen(password) <= 31){
		char empty[32] = {};
		EEPROM_enable();
		EEPROM_Write(5, 0, (uint8_t *)empty, 32);
		EEPROM_Write(5, 0, (uint8_t *)password, strlen(password));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_file_name(char * file_name){
	memset(file_name, 0, 64);
	EEPROM_enable();
	EEPROM_Read(6, 0, (uint8_t *)file_name, 64);
	EEPROM_disable();
	return 1;
}

uint8_t write_file_name(char * file_name){
	if(strlen(file_name) <= 63){
		char empty[64] = {};
		EEPROM_enable();
		EEPROM_Write(6, 0, (uint8_t *)empty, 64);
		EEPROM_Write(6, 0, (uint8_t *)file_name, strlen(file_name));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_file_path(char * file_path){
	memset(file_path, 0, 96);
	EEPROM_enable();
	EEPROM_Read(8, 0, (uint8_t *)file_path, 96);
	EEPROM_disable();
	return 1;
}

uint8_t write_file_path(char * file_path){
	if(strlen(file_path) <= 95){
		char empty[96] = {};
		EEPROM_enable();
		EEPROM_Write(8, 0, (uint8_t *)empty, 96);
		EEPROM_Write(8, 0, (uint8_t *)file_path, strlen(file_path));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

uint8_t read_local_file_name(char * loc_file_name){
	memset(loc_file_name, 0, 64);
	EEPROM_enable();
	EEPROM_Read(11, 0, (uint8_t *)loc_file_name, 64);
	EEPROM_disable();
	return 1;
}

uint8_t write_local_file_name(char * loc_file_name){
	if(strlen(loc_file_name) <= 63){
		char empty[64] = {};
		EEPROM_enable();
		EEPROM_Write(11, 0, (uint8_t *)empty, 64);
		EEPROM_Write(11, 0, (uint8_t *)loc_file_name, strlen(loc_file_name));
		EEPROM_disable();
		return 1;
	}
	return -1;
}

/*
 * reads mobile number from specified memory address with 13 digit format: +9955 ** ** ** **
 * returns 0 if no record exists
 */
uint8_t read_mobile_num(char * number, uint16_t page, uint16_t offset){
	memset(number, 0, 14);
	char num[9] = {};
	char code[5] = {};
	EEPROM_enable();
	EEPROM_Read(page, offset, (uint8_t *)num, 8);
	EEPROM_disable();
	if(strlen(num) == 0) return 0;
	read_mobile_country_code(code);
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
uint8_t write_mobile_num(char * number, uint16_t page, uint16_t offset){
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
	EEPROM_enable();
	EEPROM_Write(page, offset, (uint8_t *)num, 8);
	EEPROM_disable();
	return 1;
}

/*
 * index specifies admin 0, 1, 2 or 3
 */
uint8_t read_admin_number(char * admin_number, uint8_t index){
	if (read_mobile_num(admin_number, 13, index * 8)) return 1;
	return 0;
}

/*
 * index specifies admin 0, 1, 2 or 3
 */
uint8_t write_admin_number(char * admin_number, uint8_t index){
	if (write_mobile_num(admin_number, 13, index * 8)){
		return 1;
	}
	return -1;
}

uint8_t clear_admin_number(uint8_t index){
	char empty[8] = {};
	EEPROM_enable();
	EEPROM_Write(13, index * 8, (uint8_t *)empty, 8);
	EEPROM_disable();
	return 1;
}
