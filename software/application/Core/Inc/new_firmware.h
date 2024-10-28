/*
 * new_firmware.h
 *
 *  Created on: Oct 9, 2022
 *      Author: SHAKO
 */

#ifndef INC_NEW_FIRMWARE_H_
#define INC_NEW_FIRMWARE_H_

#include "main.h"

#define BOOTLOADER_ADDRESS   (0x8000000)

typedef void (application_t)(void);

typedef struct vector {
	uint32_t stack_addr;     // intvec[0] is initial Stack Pointer
	application_t *func_p;        // intvec[1] is initial Program Counter
} vector_t;


general_status Download_New_Firmware();
void jump_to(const uint32_t addr);

#endif /* INC_NEW_FIRMWARE_H_ */
