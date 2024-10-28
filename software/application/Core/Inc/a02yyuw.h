/*
 * a02yyuw.h
 *
 *  Created on: Jun 26, 2023
 *      Author: SHAKO
 */

#ifndef INC_A02YYUW_H_
#define INC_A02YYUW_H_

#include "main.h"
#include "usart.h"


#define A02YYUW_RX_SIZE						4
#define A02YYUW_MAX_MEAS_TRIALS				10


general_status getMeasA02(uint16_t *distance);
general_status getWaterVolume();
float getVolumeFromDistance(uint16_t distance);
general_status combined_meas(uint16_t *distance, float *temp, uint8_t ds18b20_en);


#endif /* INC_A02YYUW_H_ */
