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
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "retarget.h"
#include "stdio.h"
#include "string.h"
#include "EEPROM.h"
#include "EEPROM_data.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern I2C_HandleTypeDef hi2c1;

typedef void (application_t)(void);

typedef struct vector {
	uint32_t stack_addr;     // intvec[0] is initial Stack Pointer
	application_t *func_p;        // intvec[1] is initial Program Counter
} vector_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_RETRIEVE_NUM_SIZE        (20)           // bytes
#define MAX_RECEIVE_BUF_SIZE         (0x800)        // 2k byte
#define FLASH_PAGE_SIZE_USER         (0x400)        // 1k byte
#define FLASH_BANK_SIZE_USER         (0X19000)      // 100k byte
#define APP_START_ADDRESS            (0x08007000)

#define MAX_WAIT_FOR_BIN_RESPONSE    10000
#define MAX_ALLOWED_TRIALS			 10


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */



/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t buffer[MAX_RECEIVE_BUF_SIZE];
char send_cmd[100];
char file_name[64] = "led_blink_test.bin";
uint32_t file_size;
uint32_t position;
uint32_t new_firmware_size = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void jump_to(const uint32_t addr);
uint32_t readWord(uint32_t address);
uint8_t SIM800L_Send(char *cmd, char *expect_before, char *expect_after, char *output, uint32_t response_time);
uint32_t string_to_number(char *string);
HAL_StatusTypeDef flashWord(uint32_t dataToFlash, uint32_t address);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */

  RetargetInit(&huart3);
  HAL_UART_Receive_DMA(&huart1, buffer, MAX_RECEIVE_BUF_SIZE);

  printf("\n\n\n\n");
  printf("------------------>         Welcome to Bootloader        <------------------\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//  for(uint8_t i = 0; i < 16; i++){
//	  printf("Read word at %x: %x\n", (unsigned int)(0x08005000 + 4*i),(unsigned int)readWord(0x08005000 + 4*i));
//  }
	uint8_t goodToGo = 0;
	uint8_t en = 0;
	uint8_t trialAfterError = 0;
	en = read_bootloder_enable();

	if(en){
		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 0);
		printf("Turning SIM800L off\n");
		HAL_Delay(5000);
		HAL_GPIO_WritePin(SIM_800_POWER_GPIO_Port, SIM_800_POWER_Pin, 1);
		printf("Turning SIM800L on\n");
		HAL_Delay(20000);

		read_local_file_name(file_name);
		// disable calls and messages during flashing
			char response[MAX_RETRIEVE_NUM_SIZE] = { };
			if (SIM800L_Send("AT+CFUN?\r", "\r\n+CFUN: ", "\r\n\r\nOK", response, 1000)) {
				if (strcmp(response, "0") != 0) {
					memset(response, 0, strlen(response));

					__HAL_DMA_DISABLE(huart1.hdmarx);
					huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
					__HAL_DMA_ENABLE(huart1.hdmarx);
					USART_SendText(huart1.Instance, "AT+CFUN=0\r");
					HAL_Delay(15000);

					if (*(buffer) == '\r') {
						goodToGo = 1;
					}
				}else{
					goodToGo = 1;
				}

		//		if (SIM800L_Send("AT+CFUN=0\r", "\0\r\n+CPIN: ", "\r\n\r\nOK", response, 10000) == 1 && goodToGo != 1) {
		//			if (strcmp(response, "NOT READY") == 0) {
		//				goodToGo = 1;
		//			}
		//		}
			}

			if (goodToGo) {
				error:
				if(trialAfterError > MAX_ALLOWED_TRIALS){
					printf("\n\n");
					printf("!!!!!!!!!! Trial number reached max allowed limit !!!!!!!!!!!\n");
					printf("!!!!!!!!!!            MCU STATE CRITICAL          !!!!!!!!!!!\n");
					printf("\n\n");
					printf("Don't know what to do here\n");
					printf("Halting here ...\n");
					while(1){
						printf("------------- HELP -------------\n");
						HAL_Delay(1000);
					}
				}
				else if(trialAfterError > 0){
					printf("Retrying to flash new firmware, trial No: %d\n", trialAfterError);
				}else{
					printf("-----------------------> Ready to flash new firmware <-----------------------\n\n");
				}
				HAL_Delay(1000);

				printf("file name: %s\n", file_name);

				// determine new firmware size
				new_firmware_size = FLASH_BANK_SIZE_USER + 1;
				char output[MAX_RETRIEVE_NUM_SIZE] = {};
				sprintf(send_cmd, "AT+FSFLSIZE=C:\\User\\FTP\\%s\r", file_name);
				uint8_t ret = SIM800L_Send(send_cmd, "\r\n+FSFLSIZE: ", "\r\n\r\nOK", output, 2000);
				printf("FSFLSIZE returned: %d\n", ret);
				printf("output: %s\n", output);

				if (ret) {
					new_firmware_size = string_to_number(output);
				}
				printf("New firmware size: %lu\n", new_firmware_size);

				if (new_firmware_size > FLASH_BANK_SIZE_USER) {
					// error implement program will not fit or error reading file size
					printf("\n\n!!!! New firmware appears to be larger than allowed space !!!!\n\n");

				} else {

					printf("Unlocking the flash\n");
					/* Unock the Flash to enable the flash control register access *************/
					uint8_t counter_f = 0;
					while (HAL_FLASH_Unlock() != HAL_OK && counter_f < 10){
						HAL_FLASH_Lock();
						counter_f++;
					}
					printf("counter_f: %d\n", counter_f);

					printf("Allowing access to option bytes sector\n");
					/* Allow Access to option bytes sector */
					uint8_t counter_ob = 0;
					while (HAL_FLASH_OB_Unlock() != HAL_OK && counter_ob < 10){
						HAL_FLASH_OB_Lock();
						counter_ob++;
					}
					printf("counter_ob: %d\n", counter_ob);

					printf("Erasing flash\n");
					//erase flash
					FLASH_EraseInitTypeDef EraseInitStruct;
					EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
					EraseInitStruct.PageAddress = APP_START_ADDRESS;
					EraseInitStruct.NbPages = FLASH_BANK_SIZE_USER / FLASH_PAGE_SIZE_USER;
					uint32_t PageError;

					volatile HAL_StatusTypeDef status_erase;
					status_erase = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
					/*------------------------------------------------------------------------*/
					printf("HAL_FLASHEx_Erase returned :%d\n", status_erase);

					if (counter_f >= 10 || counter_ob >= 10 || status_erase != HAL_OK) {
						// error of unlocking flash and erasing
						printf("Error while unlocking and erasing flash\n");
						trialAfterError++;
						goto error;
					} else {
						printf("Flash erased successfully\n");

						uint8_t num_of_reads = new_firmware_size / FLASH_PAGE_SIZE_USER;
						if (new_firmware_size % FLASH_PAGE_SIZE_USER > 0)
							num_of_reads++;

						position = 0;

						printf("Starting flashing software\n");
						for (uint8_t i = 0; i < num_of_reads; i++) {

							if (new_firmware_size < FLASH_PAGE_SIZE_USER) {
								file_size = new_firmware_size;
							} else {
								file_size = FLASH_PAGE_SIZE_USER;
							}

							sprintf(send_cmd, "AT+FSREAD=C:\\User\\FTP\\%s,1,%u,%u\r", file_name, (unsigned int) file_size, (unsigned int) position);


							__HAL_DMA_DISABLE(huart1.hdmarx);
							huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
							__HAL_DMA_ENABLE(huart1.hdmarx);

							memset(buffer, 0, MAX_RECEIVE_BUF_SIZE);
							USART_SendText(huart1.Instance, send_cmd);
							HAL_Delay(200);

							uint32_t start = HAL_GetTick();
							while(strncmp("\r\n", (char*) buffer, 2) != 0){
								if(HAL_GetTick() - start > MAX_WAIT_FOR_BIN_RESPONSE){
									printf("Timeout while waiting binary file chunk response from SIM800\n");
									trialAfterError++;
									goto error;
								}
							}

							char *flash_buffp = NULL;
							if (strncmp("\r\n", (char*) buffer, 2) == 0) {
								flash_buffp = (char*) (buffer + 2);
							} else {
								// error message error read
								printf("Error while reading binary from SIM800\n");
								trialAfterError++;
								goto error;
							}

							uint32_t num_of_words_to_flash = file_size / 4;
							uint32_t offset = 0;
							for (uint32_t j = 0; j < num_of_words_to_flash; j++) {
								uint32_t word_to_flash = 0;
								word_to_flash = (*(flash_buffp + offset + 3) << 24) | (*(flash_buffp + offset + 2) << 16) | (*(flash_buffp + offset + 1) << 8)
										| (*(flash_buffp + offset));

								flashWord(word_to_flash,
								APP_START_ADDRESS + position + offset);
								//printf("Flashing at %x with: %x\n", (unsigned int)(APP_START_ADDRESS + position + offset), (unsigned int)word_to_flash);


								offset += 4;
							}
							printf("Flashing\n");
							new_firmware_size -= file_size;
							position += file_size;
						}
					}

					printf("-----> After flashing <------\n");
					/* Lock the Flash to enable the flash control register access *************/
					counter_f = 0;
					while (HAL_FLASH_Lock() != HAL_OK && counter_f < 10){
						HAL_FLASH_Unlock();
						counter_f++;
					}
					printf("counter_f: %d\n", counter_f);

					/* Lock Access to option bytes sector */
					counter_ob = 0;
					while (HAL_FLASH_OB_Lock() != HAL_OK && counter_ob < 10){
						HAL_FLASH_OB_Unlock();
						counter_ob++;
					}
					printf("counter_ob: %d\n", counter_ob);

					write_bootloder_enable(0);
					write_bootloder_wakeup(1);
				}


			}
	}
	printf("jumping to main program address: %x\n", APP_START_ADDRESS);
	HAL_Delay(2000);

	jump_to(APP_START_ADDRESS);

  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void jump_to(const uint32_t addr) {
	const vector_t *vector_p = (vector_t*) addr;

	// TODO peripheral deinit

	HAL_I2C_DeInit(&hi2c1);
	HAL_UART_DeInit(&huart3);
	HAL_UART_DeInit(&huart1);
	HAL_DMA_DeInit(huart1.hdmarx);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_All);
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_All);
	HAL_GPIO_DeInit(GPIOC, GPIO_PIN_All);
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOB_CLK_DISABLE();
	__HAL_RCC_GPIOC_CLK_DISABLE();

	HAL_RCC_DeInit();
	HAL_DeInit();
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL = 0;

	/* Disable interrupt */
	NVIC->ICER[0] = 0xFFFFFFFF;
	NVIC->ICPR[0] = 0xFFFFFFFF;
#if defined(__NRF_NVIC_ISER_COUNT) && __NRF_NVIC_ISER_COUNT == 2
	NVIC->ICER[1]=0xFFFFFFFF;
	NVIC->ICPR[1]=0xFFFFFFFF;
#endif

	/* Set new vector table */
	SCB->VTOR = (uint32_t) addr;

	/* Jump, used asm to avoid stack optimization */
	asm("\n\
        msr msp, %0; \n\
        bx %1;" : : "r"(vector_p->stack_addr), "r"(vector_p->func_p));
}

uint32_t readWord(uint32_t address) {
	uint32_t read_data;
	read_data = *(uint32_t*) (address);
	return read_data;
}

uint8_t SIM800L_Send(char *cmd, char *expect_before, char *expect_after, char *output, uint32_t response_time) {
	printf("\nCMD: %s\n", cmd);

	__HAL_DMA_DISABLE(huart1.hdmarx);
	huart1.hdmarx->Instance->CNDTR = MAX_RECEIVE_BUF_SIZE;
	__HAL_DMA_ENABLE(huart1.hdmarx);
	USART_SendText(huart1.Instance, cmd);
	HAL_Delay(response_time);

	char *buffp = (char*) buffer;

	if (strncmp(expect_before, (char*) buffer, strlen((char*) expect_before)) == 0) {
		buffp += strlen((char*) expect_before);
		if (strstr((char*) buffp, expect_after) != NULL) {
			char *tempp = strstr((char*) buffp, expect_after);
			if ((char*) tempp >= (char*) buffer) {
				strncpy(output, (char*) buffp, tempp - (char*) buffp);
				return 1;
			} else {
				printf("Excpt 1\n");
				return 0;
			}
		}
		printf("Excpt 2\n");
		return 0;
	}
	printf("Excpt 3\n");
	return 0;

}

uint32_t string_to_number(char *string) {
	uint32_t number = 0;

	printf("Converting to number: %s\n", string);

	while (*string != '\0') {
		uint8_t chr = *string;
		if (chr >= 48 && chr <= 57) {
			number = number * 10 + (chr - 48);
		}
		string++;
	}
	return number;
}

HAL_StatusTypeDef flashWord(uint32_t dataToFlash, uint32_t address) {

	volatile HAL_StatusTypeDef status;
	uint8_t flash_attempt = 0;
	do {
		status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, dataToFlash);
		flash_attempt++;
	} while (status != HAL_OK && flash_attempt < 10 && dataToFlash != readWord(address));

	return status;
}

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
