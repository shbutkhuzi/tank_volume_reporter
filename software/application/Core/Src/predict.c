/*
 * predict.c
 *
 *  Created on: Jul 1, 2023
 *      Author: SHAKO
 */

#include "predict.h"
#include "time.h"
#include "time_my.h"
#include "a02yyuw.h"
#include "math.h"



volatile timeDist_obj accum_data = {};

volatile float cylinder_height = 1.63;
volatile float cylinder_radius = 0.947768;
volatile float dome_height = 0.22;
volatile uint16_t min_distance = 30;   					// millimeters
volatile uint16_t max_distance = 1865;					// millimeters
float accum_slope = 0; 							// liters/seconds

extern int8_t ds18b20_measurements_enable;
extern float curr_volume;
extern float tank_temp;

general_status getAndInsertMeas(){
	uint16_t distance;
	general_status ret;
	time_t currtime;
	struct tm currTime;
	char strtime[30] = {};
	float volume;
	float curr_slope;

	ret = combined_meas(&distance, &tank_temp, ds18b20_measurements_enable);
	if(ret != OK){
		return ret;
	}
//	ret = getMeasA02(&distance);
//	if(ret != OK){
//		return ret;
//	}
//	distance = accum_data.timeDist[accum_data.head].distance + 1;

	ret = getSTMdatetime(&currTime, strtime);
	if(ret != OK){
		return ret;
	}
	currtime = mktime(&currTime);


	accum_data.head++;
	if(accum_data.head >= TIMEDIST_PAIR_SIZE){
		accum_data.head = 0;
	}

	volume = getVolumeFromDistance(distance);
	accum_data.timeDist[accum_data.head].distance 	= distance;
	accum_data.timeDist[accum_data.head].time 		= currtime;
	accum_data.timeDist[accum_data.head].volume 	= volume;

	curr_volume = volume;

	uint16_t prev_head;
	if(accum_data.head == 0){
		prev_head = TIMEDIST_PAIR_SIZE - 1;
	}else{
		prev_head = accum_data.head - 1;
	}

	curr_slope = (volume - accum_data.timeDist[prev_head].volume) /
				 (currtime - accum_data.timeDist[prev_head].time);

	accum_slope = (1.5*accum_slope + 0.5*curr_slope) / 2.0;

	D(printf("Slope: %5.5f  distance: %d  volume: %5.5f\n", accum_slope, distance, curr_volume));

//	D(printf("accum_data: \n"));
//	for(uint16_t i = 0; i < TIMEDIST_PAIR_SIZE/12; i++){
//		for (uint8_t j = 0; j < 12; j++){
//			D(printf("[%3d]:%4d ", j+12*i, accum_data.timeDist[j+12*i].distance));
//		}
//		D(printf("\n"));
//	}


	return OK;
}

/*
 * slope: slope of trend in liters/seconds
 * time:  time after which tank will fill or empty.
 * direction: 1  if volume increasing
 * 			  -1 if volume decreasing
 *
 */
general_status predictEmptyOrFill(float slope, time_t *time, int8_t *direction){
	float total_volume = getVolumeFromDistance(min_distance);

	if(slope > 0){
		*direction = 1;
	}else if (slope < 0){
		*direction = -1;
	}

	if(*direction == 1){
		*time = (__int_least64_t)((total_volume - curr_volume) / fabs(slope));
	}else if(*direction == -1){
		*time = (__int_least64_t)((curr_volume) / fabs(slope));
	}else{
		*time = 9000000; // big number to indicate that slope is 0
	}

	return OK;
}


