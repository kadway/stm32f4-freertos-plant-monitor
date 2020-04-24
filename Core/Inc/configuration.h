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
 */
#define FLASH_DEF_INIT         0xABCD     //Bytes for checking default initialization
#define FLASH_CONFIG_ADDR      0 //Memory block0 sector 0 for storing of general configuration
#define FLASH_AREA_ADDR        16 //Memory block0 sector 1 - start address(page number) of watering areas configuration
#define FLASH_READINGS_ADDR    16*16 //Memory block1 - start address(page number) of stored data from ADC readings

/*
 * Definitions for default configurations
 */
#define N_AREA 2  //default numumber of watering areas
#define N_SENS 3  //default numumber of moisture sensors
#define N_PUMP 2  //default numumber of watering pumps
#define N_SOV  2  //default numumber of solenoid valves
#define MAX_N_PUMP 5 //maximum number of pumps, necessary for static array containing the actuation tasks and queues handles

#define WATERING_TIME 1  //default watering time (minutes)
#define WATERING_INTERVAL 1  //default interval for watering (minutes)

#define MEAS_INTERVAL 1 /*min*/ *5/*sec*/ *1000/*ms*/ //default interval for ADC readings
#define MAX_N_SENS 10
#define MAX_N_SOV  5
#define N_ADC  15

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

/*
 * Function prototypes
 */

void configInit(void);
void initActuationTasks(void);

typedef struct generalConfig{
	uint32_t lastFlashPageNum; //last page number where readings from sensors was written
	uint16_t defaultInit;
	uint16_t adcConvTimeInterval; //interval for ADC readings (minutes)
	uint8_t userInit;
	uint8_t closedLoop;
	uint8_t nArea;   //number of watering areas
	uint8_t nSens;  //number of moisture sensors
	uint8_t nPump;  //number of water pumps
	uint8_t nSov;   //number of solenoid valves
	uint8_t dummy;  //for padding purposes
	uint8_t dummy2; //for padding purposes
}gConf_t;

/*
 * Structure defining relevant parameters for each watering area
 *
 */
typedef struct wArea{
	uint8_t sensID[MAX_N_SENS]; //ids of associated sensors in the watering area
	uint8_t sovID[MAX_N_SOV];  //ids of associated solenoid valves in the watering area
	uint32_t wateringTime;      //watering time
	uint32_t wateringInterval; //watering interval for open loop
	uint16_t threshold;  //threshold for closed loop watering control
	uint8_t pumpID;      //id of the pump watering this particular area
}wArea_t;

/*
 * Structure defining the measurement type, contains the id of the sensor and the measured value in mV
 */
typedef struct moistMeasurement{
	uint16_t id;      //id of adc
	uint16_t reading; //measured value in mV
}mMeas_t;

/*
 * Structure defining the number of measurements, contains a static array of mMeas_t and the time of adc conversions
 */
typedef struct moistMeasTime{
	uint32_t time;      //date of measurements
	//uint16_t ambient temperature from ds3232?
	mMeas_t meas[N_ADC]; //N_ADC structures to alocate each convertion and adc id
}mMeasTime_t;

typedef struct actuationTaskHandles{
	osMailQId actQueueH[MAX_N_PUMP];
	osThreadId actTaskH[MAX_N_PUMP];
}actTaskQueueH_t;

/*general configuration structures*/
gConf_t monitorConf;
wArea_t waterArea;

/*global structure holding last adc values */
mMeasTime_t lastAdcConv;

/*global structure holding the handles for dynamically allocated tasks and queues */
actTaskQueueH_t savedHandles;

#endif /* INC_CONFIGURATION_H_ */
