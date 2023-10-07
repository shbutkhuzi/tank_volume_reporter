/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdlib.h"
#include "stdio.h"
#include "new_firmware.h"
#include "SIM800L.h"
#include "retarget.h"
#include "Process_Data.h"
#include "EEPROM_data.h"
#include "adc_util.h"
#include "ds18b20.h"
#include "a02yyuw.h"
#include "predict.h"
#include "EEPROM.h"
#include "critical.h"
#include "client_subscription.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

extern uint8_t buffer[MAX_RECEIVE_BUF_SIZE];
extern uint8_t bootloader_restart;

extern float tank_temp;
extern float curr_volume;
extern volatile timeDist_obj accum_data;

extern uint8_t ds18b20_measurements_enable;

//extern uint8_t low_battery_shutdown_flag;

extern char admins[MAX_ADMINS][14];

uint8_t standby_flag = 0;
extern uint8_t standby_mode_enable;

extern struct standby_alarm wakeup_time;
extern struct standby_alarm standby_time;

uint8_t measurement_error_flag = 0;
general_status measurement_error_no;

time_t measError_time = 0;

extern uint8_t subscription_enable;
extern data_addr data;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc){

	if(standby_mode_enable) {
		standby_flag = 1;
		D(printf("Marking stanby_flag = 1\n"));
	}

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
//	if(low_battery_shutdown_flag) return;

//	D(printf("UART receive callback\n"));
	HAL_ResumeTick();

	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0x0);
}



void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc){

	HAL_NVIC_DisableIRQ(RTC_IRQn);
	HAL_NVIC_DisableIRQ(RTC_Alarm_IRQn);

	HAL_NVIC_DisableIRQ(USART1_IRQn);

//	SystemClock_Config();
	HAL_ResumeTick();

	HAL_RTCEx_BKUPWrite(hrtc, RTC_BKP_DR1, HAL_RTCEx_BKUPRead(hrtc, RTC_BKP_DR1)+1);
	HAL_RTCEx_BKUPWrite(hrtc, RTC_BKP_DR2, HAL_RTCEx_BKUPRead(hrtc, RTC_BKP_DR2)+1);
//	D(printf("1 second interrupt callback\n"));

	if(HAL_RTCEx_BKUPRead(hrtc, RTC_BKP_DR1) >= 10){
		D(printf("10 second interrupt callback\n"));
		// reset backup register value
		HAL_RTCEx_BKUPWrite(hrtc, RTC_BKP_DR1, 0x0);

		// disable auto enrty into low power in order to
		// process data and respond to requests after taking
		// sensor measurements
//		HAL_PWR_DisableSleepOnExit();


		// take sensor measurements
		volatile general_status ret;
//		if(ds18b20_measurements_enable){
//			ret = getTempDS18B20(&tank_temp);
//			if(ret != OK) D(printf("DS18B20 Error: %d\n", ret));
//		}

		ret = getAndInsertMeas();
		if(ret != OK){
			D(printf("Measurement error: %d\n", ret));
			measurement_error_flag = 1;
			measurement_error_no = ret;
		}
		measurement_error_flag = 0;

	}

	if(HAL_RTCEx_BKUPRead(hrtc, RTC_BKP_DR2) >= 30){
		D(printf("30 second interrupt callback\n"));
		HAL_RTCEx_BKUPWrite(hrtc, RTC_BKP_DR2, 0x0);
	}


	HAL_NVIC_EnableIRQ(RTC_IRQn);
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

	HAL_NVIC_EnableIRQ(USART1_IRQn);

}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

//  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);

  // Timer for DS18B20, for microsecond delay
  HAL_TIM_Base_Start(&htim4);

  RetargetInit(&huart3);
  HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);

  // write initial value into first backup register to indicate initial state
  // of measurements period setup
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xFFFF);
  // initial value for answering requests periodically
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, 0xFFFF);



  general_status rett;
//
//  rett = delete_user("995598933675");
//  printf("delete_user returned: %d\n", rett);
//
//  rett = delete_user("995598933845");
//  printf("delete_user returned: %d\n", rett);
//
//  rett = delete_user("995598933846");
//  printf("delete_user returned: %d\n", rett);
////
//  rett = add_user("+995598933845", "2000(V)", "60(T)");
//  printf("add_user returned: %d\n", rett);
//
//  rett = add_user("+995598933846", "3000(V)", "60(T)");
//  printf("add_user returned: %d\n", rett);
//
//  rett = add_user("+995598933675", "240(T)", "2500(V)");
//  printf("add_user returned: %d\n", rett);
//
//  rett = read_user("+995598933845");
//  printf("read_user returned: %d\n", rett);
//
//  rett = read_user("+995598933675");
//  printf("read_user returned: %d\n", rett);
//
//  rett = read_user("+995598933846");
//  printf("read_user returned: %d\n", rett);
//
//  rett = write_user_enable("+995598933675", 0);
//  printf("write_user_enable returned: %d\n", rett);
//
//  rett = write_user_enable("+995598933845", 0);
//  printf("write_user_enable returned: %d\n", rett);
//
//  rett = read_user("+995598933845");
//  printf("read_user returned: %d\n", rett);
//
//  rett = write_user_enable("+995598933845", 1);
//  printf("write_user_enable returned: %d\n", rett);
//
//  rett = update_user("+995598933675", "7020(V),2400(V),200(T)", "100(T),15000(V)");
//  printf("update_user returned: %d\n", rett);
//
//  rett = update_user("+995598933845", "100(T),1800(V),120(T)", "10(T),6000(V),15000(V)");
//  printf("update_user returned: %d\n", rett);
//
//  rett = read_user("+995598933845");
//  printf("read_user returned: %d\n", rett);
//
//  rett = read_user("+995598933675");
//  printf("read_user returned: %d\n", rett);
//
//  rett = read_user("+995598933846");
//  printf("read_user returned: %d\n", rett);

  subscription_enable = 1;

  rett = init_read_users(&data);
  D(printf("init_read_users returned :%d\n", rett));

  D(printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   VOLUME  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
  print_thresholds(data.volume_thresholds);

  D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   TIME   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
  print_thresholds(data.time_thresholds);

  D(printf("\n\n\n\n\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   ADDRESSES   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n"));
  print_number_page(data.number_addr);



//  while(1){}




//  uint32_t tickstart;
//  	uint8_t buffer_a02[A02YYUW_RX_SIZE];
////  	uint16_t a02yyuw_measurement = 0;
//  	uint8_t trials = 0;
//  	uint16_t distance;
//
//	HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 0);
//
//  while(1){
//	  tickstart = HAL_GetTick();
//	while(1){
//		if(HAL_UART_Receive(&huart2, buffer_a02, 1, 300) != HAL_OK){
//			D(printf("Error in reception of data\n"));
//			if(trials >= A02YYUW_MAX_MEAS_TRIALS){
//				return A02YYUW_ERROR_READING_MEASUREMENT;
//			}
//			trials += 1;
//			continue;
//		}
//		if(buffer_a02[0] == 0xFF){
//			HAL_UART_Receive(&huart2, buffer_a02+1, 1, 300);
//			HAL_UART_Receive(&huart2, buffer_a02+2, 1, 300);
//			HAL_UART_Receive(&huart2, buffer_a02+3, 1, 300);
//
//			if((uint8_t)(buffer_a02[0] + buffer_a02[1] + buffer_a02[2]) == buffer_a02[3]){
//				distance = (buffer_a02[1] << 8) | buffer_a02[2];
////				a02yyuw_measurement = distance;
//  				D(printf("Distance: %d mm\n", distance));
//				break;
//			}
//		}
//		if(HAL_GetTick() - tickstart > 1000){
//			HAL_GPIO_WritePin(A02_CONTROL_GPIO_Port, A02_CONTROL_Pin, 1);
//			return A02YYUW_TIMEOUT;
//		}
//	}
//
//  }



  general_status ret;
  ret = critical_check();
  if(ret != OK){
	  D(printf("error during critical check. error no: %d", ret));
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  char * confStr;
  confStr = (char *)malloc(sizeof(char) * CONF_REPORT_MAX_LENGTH);
  read_all_conf(confStr);
  free(confStr);


  SIM800L_init();

  if(read_bootloder_wakeup(1)){
	  write_bootloder_wakeup(0, 1);

	  char message[100] = {};
	  sprintf(message, "Module has woken up from successful bootloader start");
	  SIM800L_SMS(admins[0], message, 1);
  }

  // set alarm for entering standby mode
  RTC_AlarmTypeDef sAlarm;
  sAlarm.Alarm = RTC_ALARM_A;
  sAlarm.AlarmTime.Hours = standby_time.hour;
  sAlarm.AlarmTime.Minutes = standby_time.minute;
  sAlarm.AlarmTime.Seconds = standby_time.second;
  HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);


  HAL_RTCEx_SetSecond_IT(&hrtc);

  while (1)
  {

	  // wake sim800 from sleep
	  HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 0);
	  HAL_Delay(500);

	  SIM800L_RECONNECT();

	  if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET){
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);  // clear the flag

		D(printf("woke up from standby\n"));
		char message[200] = {};
		sprintf(message, "Module has woken up from standby mode");
		SIM800L_SMS(admins[0], message, 1);
	  }

	  // skip request handling for the first time device has powered up
	  if(tank_temp == 0 && curr_volume == 0){
		  D(printf("skip due to recent power up\n"));
	  }else{
		  check_requests();
	  }

	  struct tm curr_tm;
	  char strtime[30] = {};
	  time_t curr_time;
	  getSTMdatetime(&curr_tm, strtime);
	  curr_time = mktime(&curr_tm);

	  if(measurement_error_flag && ((curr_time - measError_time) > 60 * 15)){
		  measurement_error_flag = 0;

		  measError_time = curr_time;

		  char message[50] = {};
		  sprintf(message, "Measurement error: %d", measurement_error_no);
		  SIM800L_SMS(admins[0], message, 1);
	  }

	  if(standby_flag){
		// set alarm for waking up
		RTC_AlarmTypeDef sAlarm;
		sAlarm.Alarm = RTC_ALARM_A;
		sAlarm.AlarmTime.Hours = wakeup_time.hour;
		sAlarm.AlarmTime.Minutes = wakeup_time.minute;
		sAlarm.AlarmTime.Seconds = wakeup_time.second;
		HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);

		char message[200] = {};
		sprintf(message, "Module is going to enter standby mode");
		SIM800L_SMS(admins[0], message, 1);

		SIM800L_SHUTDOWN();
		// in case shutdown command did not succeed, cut power
		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 0);


		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		/* clear the RTC Wake UP (WU) flag */
		//  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
		__HAL_RTC_CLEAR_FLAG();

		HAL_PWR_EnterSTANDBYMode();
	  }

	  // enable sim800 module to go to sleep
	  HAL_GPIO_WritePin(SIM_800_DTR_GPIO_Port, SIM_800_DTR_Pin, 1);

	  // check critical events
	  ret = critical_check();
	  if(ret != OK){
		  D(printf("error during critical check. error no: %d", ret));
	  }

	  if(bootloader_restart){
		  SIM800L_SHUTDOWN();
		  jump_to(BOOTLOADER_ADDRESS);
	  }


//	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

//	  HAL_Delay(5000);


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

//	  D(printf("Outer loop check \n"));

	  HAL_UART_DMAStop(&huart1);
	  HAL_UART_Receive_DMA(&huart1, buffer, 1);

	  while(1){
	//	  HAL_PWR_EnableSleepOnExit();
//		  __HAL_RCC_PWR_CLK_ENABLE();
//		  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFE);


		  HAL_SuspendTick();
		  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		  HAL_ResumeTick();

		  if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2) == 0){
			  HAL_UART_DMAStop(&huart1);
			  HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);
			  break;
		  }
	  }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  D(printf("Into Error_Handler\nRestarting..."));
	  NVIC_SystemReset();
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
