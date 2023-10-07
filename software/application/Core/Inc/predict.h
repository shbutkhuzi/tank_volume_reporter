/*
 * predict.h
 *
 *  Created on: Jul 1, 2023
 *      Author: SHAKO
 */

#ifndef INC_PREDICT_H_
#define INC_PREDICT_H_

#include "main.h"


#define TIMEDIST_PAIR_SIZE						10



typedef struct __attribute__((packed)){
	uint16_t distance;
	float volume;
	time_t time;
}timedist;


typedef struct __attribute__((packed)){
	timedist timeDist[TIMEDIST_PAIR_SIZE];
	uint16_t head;
}timeDist_obj;




general_status getAndInsertMeas();
general_status predictEmptyOrFill(float slope, time_t *time, int8_t *direction);


#endif /* INC_PREDICT_H_ */
