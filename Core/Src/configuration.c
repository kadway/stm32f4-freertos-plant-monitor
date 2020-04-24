/*
 * configuration.h
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */


#include "configuration.h"
#include "w25qxx.h"

void configInit(void){
	uint16_t i=0;
	uint16_t dataSize = 0;

	W25qxx_ReadPage((uint8_t*)&monitorConf, FLASH_CONFIG_ADDR, 0, sizeof(monitorConf));

	if(monitorConf.defaultInit != FLASH_DEF_INIT){
		//if(1){
		// init general configurations
		monitorConf.defaultInit=FLASH_DEF_INIT;
		monitorConf.userInit = 0;
		monitorConf.closedLoop = 0;
		monitorConf.nArea = N_AREA;
		monitorConf.nSens = N_SENS;
		monitorConf.nPump = N_PUMP;
		monitorConf.nSov = N_SOV;
		monitorConf.lastFlashPageNum = FLASH_READINGS_ADDR;
		monitorConf.adcConvTimeInterval = MEAS_INTERVAL;
		monitorConf.dummy = 0xeb;
		monitorConf.dummy2 = 0xeb;
		//save to external flash
		W25qxx_EraseChip();
		//W25qxx_EraseBlock(FLASH_CONFIG_ADDR);
		W25qxx_WritePage((uint8_t*)&monitorConf, FLASH_CONFIG_ADDR, 0, sizeof(monitorConf));

#if (PRINTF_DEBUG == 1)
		printf("w25qxx init - Default general configuration initialized to flash\r\n");
#endif

		//init default watering areas
		dataSize = sizeof(waterArea)/sizeof(uint8_t);
		waterArea.wateringTime = WATERING_TIME;
		waterArea.wateringInterval = WATERING_INTERVAL;
		waterArea.threshold = 0;

		memset(waterArea.sensID, 0, sizeof(waterArea.sensID));
		memset(waterArea.sovID, 0, sizeof(waterArea.sovID));

		for (i=0; i<N_AREA; i++){
			waterArea.pumpID = i+1;
			//save to external flash
			W25qxx_WritePage((uint8_t*)&waterArea, FLASH_AREA_ADDR, i*(dataSize), dataSize);
		}
#if (PRINTF_DEBUG == 1)
			printf("w25qxx init - Default area configuration saved to flash. %d areas added.\r\n", N_AREA);
			printf("\n ----monitor conf: -----\n");
			W25qxx_ReadPage((uint8_t*)&monitorConf, FLASH_CONFIG_ADDR, 0, sizeof(monitorConf));
#endif
	}
	else{
#if (PRINTF_DEBUG == 1)
		printf("w25qxx init - Configuration recoverd from flash\r\n");
#endif
	}

}

void initActuationTasks(void){

}
