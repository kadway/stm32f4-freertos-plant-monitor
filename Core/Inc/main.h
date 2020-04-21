/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
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
#include "stm32f4xx_hal.h"
#include "spi.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "w25qxx.h"

extern osThreadId spiEspComTaskHandle;
extern osThreadId myTask02Handle;
extern osSemaphoreId spiEspSemphTXHandle;
extern osSemaphoreId spiEspSemphHandle;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/*
 * Definitions for external flash access
 */
#define FLASH_DEF_INIT         0xABCD     //Bytes for checking default initialization
#define FLASH_CONFIG_ADDR      0x00000000 //Memory block0 sector 0 - address of general configuration
#define FLASH_AREA_ADDR        0x00001001 //Memory block0 sector 1 - start address of watering areas configuration
#define FLASH_READINGS_ADDR    0x01000001 //Memory block1 - start address of stored data from ADC readings

/*
 * Definitions for default configurations
 */
#define N_AREA 2  //default numumber of watering areas
#define N_SENS 3  //default numumber of moisture sensors
#define N_PUMP 2  //default numumber of watering pumps
#define N_SOV  2  //default numumber of solenoid valves
#define WATERING_TIME 1  //default watering time (minutes)
#define MEAS_INTERVAL 10 //default interval for ADC readings (minutes)
#define MAX_N_SENS 10
#define MAX_N_SOV  5

/*
 * Definition of control bytes for SPI communication with ESP8266
 */
#define ESP_GET_CONF 0xAA //get general configuration data
#define ESP_SET_CONF 0xAB
#define ESP_GET_AREA 0xBA //get area configuration data
#define ESP_SET_AREA 0xBB
#define ESP_GET_DATA 0xCA //get measurements data
#define ESP_CLEAR_DATA 0xCB

#define ESP_STOP_CONTROL_TASK    0xDA //suspend control task
#define ESP_RESUME_CONTROL_TASK  0xDB //resume control task



void DMATransferComplete(DMA_HandleTypeDef *hdma);

/*
 * Structure with relevant configurations to be stored in external flash and loaded on startup
 * Default initialization writes FLASH_DEF_INIT to defaultInit
 * This is a criteria for checking if the memory already has the the default data written into it
 */

typedef struct generalConfig{
	uint32_t lastWrittenFlashAddr; //last address where readings from sensors was written
	uint16_t defaultInit;
	uint16_t measInt; //interval for ADC readings (minutes)
	uint8_t userInit;
	uint8_t closedLoop;
	uint8_t nArea;   //number of watering areas
	uint8_t nSens;  //number of moisture sensors
	uint8_t nPump;  //number of water pumps
	uint8_t nSov;   //number of solenoid valves
	uint8_t dummy; //for padding purposes
	uint8_t dummydummy; //for padding purposes
}gConf_t;

/*
 * Structure defining relevant parameters for each watering area
 *
 */

typedef struct wArea{
	uint8_t sensID[MAX_N_SENS]; //ids of associated sensors in the watering area
	uint8_t sovID[MAX_N_SOV];  //ids of associated solenoid valves in the watering area
	uint16_t wTime;      //watering time for this area
	uint16_t threshold;  //threshold for closed loop watering control
	uint8_t pumpID;      //id of the pump watering this particular area
}wArea_t;

/*
 * Structure defining the measurement type, contains the id of the sensor and the measured value in mV
 */

typedef struct moistMeasurement{
	uint8_t id;
	uint16_t reading;
}mMeas_t;

//general configuration structures
gConf_t monitorConf;
wArea_t waterArea;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
