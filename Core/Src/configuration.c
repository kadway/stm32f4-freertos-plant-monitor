/*
 * configuration.h
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */


#include "configuration.h"
#include "w25qxx.h"
osMessageQId actuationTaskQueueHandle;
osThreadId actuationTaskHandle;
void configInit(void){
	uint16_t i=0;

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

		waterArea.wateringTime = WATERING_TIME;
		waterArea.wateringInterval = WATERING_INTERVAL;
		waterArea.threshold = 0;

		//starting id zero means element is not existing and should be disconsidered
		//ids to be initialized by user
		memset(waterArea.sensID, 0, sizeof(waterArea.sensID));
		memset(waterArea.sovID, 0, sizeof(waterArea.sovID));

		for (i=0; i<N_AREA; i++){
			waterArea.pumpID = i+1; //starting id should be 1 so that zero is considered not existing
			//save to external flash
			W25qxx_WritePage((uint8_t*)&waterArea, FLASH_AREA_ADDR, i*sizeof(wArea_t), sizeof(wArea_t));
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
		initActuationTasks();
	}

}

void initActuationTasks(void){
	uint8_t pumpID, areaID;
	uint8_t queueSize = 0;
	void const * pQueueHandle = NULL;
	/*loop over the number of existing pumps*/
	for (pumpID=0; pumpID<monitorConf.nPump; pumpID++){

		for(areaID=0; areaID<monitorConf.nArea; areaID++){
			/*get necessary queue size by reading how many areas use the same pump*/
			W25qxx_ReadPage((uint8_t*)&waterArea, FLASH_AREA_ADDR, areaID*sizeof(waterArea), sizeof(waterArea));
			if(waterArea.pumpID == pumpID+1){
				queueSize += 1;
#if (PRINTF_DEBUG == 1)
				printf("Water area %d uses pump %d. Queue size is now %d \n", areaID+1,pumpID+1, queueSize);
#endif
			}
		}
		if(queueSize > 0){
			/* Create the queue(s) */
			osMailQDef(actuationQueue, queueSize, waterArea);
			savedHandles.actQueueH[pumpID] = osMailCreate(osMailQ(actuationQueue), NULL);

			/*if memory allocation succedded pass the pointer on to the new task*/
			if(savedHandles.actQueueH[pumpID] != NULL){
				pQueueHandle = (void*) &savedHandles.actQueueH[pumpID];
				queueSize = 0;

				/* Creation of actuation task(s) */
				osThreadDef(actTask, actuationTask, osPriorityAboveNormal, MAX_N_PUMP, 300);
				savedHandles.actTaskH[pumpID]= osThreadCreate(osThread(actTask), pQueueHandle);
#if (PRINTF_DEBUG == 1)
				printf("Created Queue and task for pump %d\n", pumpID+1);
				printf("monitorConf %d wArea %d\n", sizeof(gConf_t), sizeof(wArea_t));
#endif
			}
		}
	}
}
