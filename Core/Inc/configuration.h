/*
 * configuration.h
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */

#ifndef INC_CONFIGURATION_H_
#define INC_CONFIGURATION_H_

#include "main.h"

/*
 * Definitions for external flash memory access (w25q16)
 * Hints: Page size is 256 Bytes, Sectors have 16 Pages, Blocks have 16 Sectors and there are 32 Blocks.
 * 		  Can only write up to one page at a time indicating the page number and offset.
 * 		  The respective sector or block has to be erased before writing!
 */

#define FLASH_DEF_INIT_CODE    0xABCD     //Bytes for checking default initialization
#define FLASH_CONFIG_ADDR      0 		  //Memory block0 sector 0 for storing of general configuration
#define FLASH_CONFIG_SECTOR	   0 		  //sector 0, definition for erasing
#define FLASH_AREA_ADDR        16 		  //Memory block0 sector 1 - start address(page number) of watering areas configuration
#define FLASH_AREA_SECTOR	   1 		  //sector 1, definition for erasing
#define FLASH_ADC_LOG_ADDR     16*16      //Memory block1 - start address(page number) of stored data from ADC readings
#define FLASH_ADC_LOG_SECTOR   16         //Sector num 16 or Block1 sector 0
#define FLASH_ADC_LOG_BLOCK_NUM	   1
#define FLASH_ACT_LOG_ADDR     16*16*30   //Memory Block30 - start address(page number) of stored data from ADC readings
#define FLASH_ACT_LOG_SECTOR   16*30      //Sector 480 or Block30 sector 0
#define FLASH_ACT_LOG_BLOCK_NUM    30
/*
 * Definitions for default configurations
 */
#define N_AREA 2  //default numumber of watering areas
#define N_SENS 5  //default numumber of moisture sensors
#define N_PUMP 2  //default numumber of watering pumps
#define N_SOV  5  //default numumber of solenoid valves
#define MAX_N_PUMP 5 //maximum number of pumps, necessary for static array containing the actuation tasks and queues handles

#define WATERING_TIME 1 *1000/*ms*/  //default watering time in ticks (milisecond)
#define WATERING_INTERVAL 7*1000/*ms*/ //default interval for watering in ticks (milisecond)
#define MEAS_INTERVAL 3600 * 1000/*ms*/ //default interval for ADC readings in ticks (milisecond)
#define MAX_N_SENS 10
#define MAX_N_SOV  5
#define MAX_N_AREA 30
#define N_ADC  15

/*
 * Definition of control bytes for SPI communication with ESP8266
 */
#define ESP_GET_CONF       0xAA //get general configuration data
#define ESP_SET_CONF       0xAB
#define ESP_GET_AREA 	   0xBA //get area configuration data
#define ESP_SET_AREA	   0xBB
#define ESP_GET_DATA_ADC   0xCA //get measurements data
#define ESP_CLEAR_DATA_ADC 0xCB
#define ESP_GET_DATA_ACT   0xDA //get actuation data
#define ESP_CLEAR_DATA_ACT 0xDB
#define ESP_CLEAR_LOG	   0xEA
#define ESP_CLEAR_CONF	   0xEB

#define ESP_STOP_CONTROL_TASK    0xFA //suspend control task
#define ESP_RESUME_CONTROL_TASK  0xFB //resume control task

#define ACK_MASTER  0xE3E3E3E3   //Bytes for master Ack

typedef struct generalConfig{
	uint32_t adcConvTimeInterval;   //interval for ADC readings
	uint16_t initCode;              //Bytes for checking default initialization
	uint16_t pageAdc;   //last page number where readings from sensors was written
	uint16_t pageAct;   //last page number where actuation log was written
	uint16_t pageOffsetAdc;          //page offset in bytes for writing to external flash memory
	uint16_t pageOffsetAct;          //page offset in bytes for writing to external flash memory
	uint8_t nArea;  //number of watering areas
	uint8_t nSens;  //number of moisture sensors
	uint8_t nPump;  //number of water pumps
	uint8_t nSov;   //number of solenoid valves
}gConf_t;

/*
 * Structure defining relevant parameters for each watering area
 *
 */
typedef struct wArea{
	uint8_t sensID[MAX_N_SENS]; //ids of associated sensors in the watering area
	uint8_t sovID[MAX_N_SOV];   //ids of associated solenoid valves in the watering area
	uint32_t wateringDuration;  //watering duration
	uint32_t wateringInterval;  //watering interval for open loop
	uint32_t lastWateringtime;  //last time of watering
	uint16_t threshold;         //threshold for closed loop watering control
	uint8_t pumpID;             //id of the pump watering this particular area
	uint8_t openLoop;           //to consider the threshold or not
	uint8_t areaID;             //number for the area to be watered
}wArea_t;

/*
 * Structure defining the number of measurements, contains a static array of mMeas_t and the time of adc conversions
 */
typedef struct moistMeasTime{
	uint32_t time;        //date of measurements
	uint16_t temperature; //ambient temperature from ds3232
	uint16_t meas[N_ADC]; //N_ADC structures to alocate each convertion and adc id
}mMeasTime_t;

typedef struct wateringTime{
	uint32_t time;       //time of watering
	uint32_t duration;   //watering duration
	uint8_t  areaID;     //number for the area to be watered
}wTime_t;

typedef struct actuationTaskHandles{
	osMailQId queueH;
	osThreadId taskH;
	osTimerId timerH;
}actTaskQueueH_t;

typedef enum flashDType {
	gConfData = 0,
	wAreaData,
	mMeasTimeData,
	wTimeData
}flashDataType;

typedef enum flashOType {
	READ = 0,
	WRITE
}flashOpType;

/*general configuration structures*/
gConf_t gConf;
wArea_t aConf[MAX_N_AREA];


/*global structure holding last adc values */
mMeasTime_t lastAdcConv;

/*global structure holding the handles for allocated tasks and queues */
actTaskQueueH_t savedHandles[MAX_N_PUMP];

/*
 * Function prototypes
 */

void configInit(void);
void initActuationTasks(void);
void readWriteFlash(void * data, uint8_t size, flashDataType type, flashOpType operationType, uint16_t* pPageNum, uint16_t* pOffset);
void updateOffset(uint16_t startPage, uint16_t* actualPage, uint16_t endPage, uint16_t* offset, uint8_t size);
uint32_t getNumElements(flashDataType type);
#endif /* INC_CONFIGURATION_H_ */
