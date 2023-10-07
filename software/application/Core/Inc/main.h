/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stdio.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define DEBUG_MY

#ifdef DEBUG_MY
#  define D(x) x
#else
#  define D(x)
#endif

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define LIPO_ADC_Pin GPIO_PIN_1
#define LIPO_ADC_GPIO_Port GPIOA
#define A02YYUW_TX_Pin GPIO_PIN_2
#define A02YYUW_TX_GPIO_Port GPIOA
#define A02YYUW_RX_Pin GPIO_PIN_3
#define A02YYUW_RX_GPIO_Port GPIOA
#define SOLAR_ADC_Pin GPIO_PIN_4
#define SOLAR_ADC_GPIO_Port GPIOA
#define DS18B20_TX_Pin GPIO_PIN_5
#define DS18B20_TX_GPIO_Port GPIOA
#define SIM_800_DTR_Pin GPIO_PIN_7
#define SIM_800_DTR_GPIO_Port GPIOA
#define SOLAR_PRESENSE_Pin GPIO_PIN_0
#define SOLAR_PRESENSE_GPIO_Port GPIOB
#define A02_OUT_SEL_Pin GPIO_PIN_1
#define A02_OUT_SEL_GPIO_Port GPIOB
#define DEBUG_TX_Pin GPIO_PIN_10
#define DEBUG_TX_GPIO_Port GPIOB
#define DEBUG_RX_Pin GPIO_PIN_11
#define DEBUG_RX_GPIO_Port GPIOB
#define A02_CONTROL_Pin GPIO_PIN_12
#define A02_CONTROL_GPIO_Port GPIOB
#define EEPROM_CONTROL_Pin GPIO_PIN_13
#define EEPROM_CONTROL_GPIO_Port GPIOB
#define DS18_CONTROL_Pin GPIO_PIN_14
#define DS18_CONTROL_GPIO_Port GPIOB
#define SIM_800_RESET_Pin GPIO_PIN_15
#define SIM_800_RESET_GPIO_Port GPIOB
#define SIM_800_TX_Pin GPIO_PIN_9
#define SIM_800_TX_GPIO_Port GPIOA
#define SIM_800_RX_Pin GPIO_PIN_10
#define SIM_800_RX_GPIO_Port GPIOA
#define SIM_800_POWER_Pin GPIO_PIN_3
#define SIM_800_POWER_GPIO_Port GPIOB
#define EEPROM_SCL_Pin GPIO_PIN_6
#define EEPROM_SCL_GPIO_Port GPIOB
#define EEPROM_SDA_Pin GPIO_PIN_7
#define EEPROM_SDA_GPIO_Port GPIOB
#define LIPO_ADC_EN_Pin GPIO_PIN_8
#define LIPO_ADC_EN_GPIO_Port GPIOB
#define SOLAR_ADC_EN_Pin GPIO_PIN_9
#define SOLAR_ADC_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

typedef enum{
  OK = 0,
  SIM800_NOT_CONNECTED_TO_NETWORK,
  GPRS_CONTEXT_CONF_ERROR,
  GPRS_CONTEXT_CONF_SETTING_ERROR,
  APN_CONF_ERROR,
  APN_CONF_SETTING_ERROR,
  GPRS_CONTEXT_OPEN_FAIL,
  FTPCID_CONF_ERROR,
  FTPCID_CONF_SETTING_ERROR,
  FTPSERV_CONF_ERROR,
  FTPSERV_CONF_SETTING_ERROR,
  FTPUN_CONF_ERROR,
  FTPUN_CONF_SETTING_ERROR,
  FTPPW_CONF_ERROR,
  FTPPW_CONF_SETTING_ERROR,
  FTPGETNAME_CONF_ERROR,
  FTPGETNAME_CONF_SETTING_ERROR,
  FTPGETPATH_CONF_ERROR,
  FTPGETPATH_CONF_SETTING_ERROR,
  FTPGETTOFS_CONF_ERROR,
  FTPGETTOFS_CONF_SETTING_ERROR,
  FIRMWARE_DOWNLOAD_TIMEOUT_ERROR,
  GPRS_CONTEXT_CLOSE_TIMEOUT_ERROR,
  SMS_SEND_TIMEOUT_ERROR,
  SIM800_DOES_NOT_RESPOND,
  SIM800_ERROR_READING_CSQ,
  SIM800_ERROR_READING_CBC,
  SIM800_ERROR_SENDING_SMS,
  SIM800_ERROR_GETTING_CURRENT_TIME,
  SIM800_NORMAL_POWERDOWN_UNSUCCESSFUL,
  SIM800_CSMP_SETTING_ERROR,
  SIM800_CSMP_RESETTING_ERROR,
  SIM800_CSCS_SETTING_ERROR,
  SIM800_CSCS_RESETTING_ERROR,
  DS18B20_NO_RESPONSE,
  DS18B20_INVALID_TEMP,
  A02YYUW_ERROR_READING_MEASUREMENT,
  A02YYUW_TIMEOUT,
  ADMIN_INDEX_OUT_OF_RANGE,
  TO_UTF_CONV_ERROR_TERMINATION_CHAR_NOT_FOUND,
  TO_UTF_CONV_ERROR_MESSAGE_WILL_NOT_FIT,
  KEY_WORD_INDEX_OUT_OF_RANGE,
  SUBSCRIPTION_UNSUPPORTED_NUMBER_FORMAT,
  SUBSCRIPTION_COUNTRY_CODE_DOES_NOT_MATCH,
  SUBSCRIPTION_NO_MORE_SPACE_TO_ADD_NEW_USER,
  SUBSCRIPTION_MEMORY_ALLOCATION_ERROR,
  SUBSCRIPTION_USER_ALREADY_EXISTS,
  SUBSCRIPTION_LOWER_THRESHOLDS_NOT_RECOGNIZED,
  SUBSCRIPTION_UPPER_THRESHOLDS_NOT_RECOGNIZED,
  SUBSCRIPTION_USER_DOES_NOT_EXIST,
  SUBSCRIPTION_LOWER_LIMIT_TYPE_NOT_RECOGNIZED,
  SUBSCRIPTION_UPPER_LIMIT_TYPE_NOT_RECOGNIZED,
  SUBSCRIPTION_NOT_ENABLED,
  SUBSCRIPTION_NO_USER_REGISTERED,
  SUBSCRIPTION_PARSE_FAILED,
  SUBSCRIPTION_ADD_NUMBER_PAGE_ERROR_WHILE_MOVING_CLIENT_DATA_TO_RAM,
  SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_TIME_WHILE_MOVING_CLIENT_DATA_TO_RAM,
  SUBSCRIPTION_ADD_THRESHOLD_ERROR_FOR_VOLUME_WHILE_MOVING_CLIENT_DATA_TO_RAM
} general_status;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
