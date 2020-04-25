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

	W25qxx_ReadPage((uint8_t*)&generalConf, FLASH_CONFIG_ADDR, 0, sizeof(generalConf));

	//if(generalConf.initCode != FLASH_DEF_INIT_CODE){
		if(1){
		/* Init general configurations */
		generalConf.initCode=FLASH_DEF_INIT_CODE;
		generalConf.nArea = N_AREA;
		generalConf.nSens = N_SENS;
		generalConf.nPump = N_PUMP;
		generalConf.nSov = N_SOV;
		generalConf.lastFlashPageNum = FLASH_READINGS_ADDR;
		generalConf.adcConvTimeInterval = MEAS_INTERVAL;
		W25qxx_EraseChip();
		/* Save to external flash */
		W25qxx_WritePage((uint8_t*)&generalConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf_t));
#if (PRINTF_DEBUG == 1)
		printf("w25qxx init - Default general configuration initialized to flash\r\n");
#endif
		/* Init default watering areas */
		areaConf.wateringTime = WATERING_TIME;
		areaConf.wateringInterval = WATERING_INTERVAL;
		areaConf.lastWateringtime = 0;
		areaConf.threshold = 0;
		areaConf.openLoop = 1;
		//starting id zero means element is not existing and should be disconsidered
		//ids to be initialized by user
		memset(areaConf.sensID, 0, sizeof(areaConf.sensID));
		memset(areaConf.sovID, 0, sizeof(areaConf.sovID));

		for (i=0; i<N_AREA; i++){
			areaConf.areaID = i+1;
			areaConf.pumpID = i; //starting id should be 1 so that zero is considered not existing
			W25qxx_WritePage((uint8_t*)&areaConf, FLASH_AREA_ADDR, i*sizeof(wArea_t), sizeof(wArea_t));
		}
#if (PRINTF_DEBUG == 1)
			printf("w25qxx init - Default area configuration saved to flash. %d areas added.\r\n", N_AREA);
			printf("Sizes of structures: generalConf %d areaConf %d\n", sizeof(gConf_t), sizeof(wArea_t));
			osDelay(5000);
			//printf("\n ----monitor conf: -----\n");
			//W25qxx_ReadPage((uint8_t*)&generalConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf_t));
#endif
	}
	else{
#if (PRINTF_DEBUG == 1)
		printf("w25qxx init - Configuration recoverd from flash\r\n");
#endif

	}
	initActuationTasks();
}

void initActuationTasks(void){
	uint8_t pumpID, areaID;
	uint8_t queueSize = 0;
	void const * pQueueHandle = NULL;
	/* Loop over the number of existing pumps */
	for (pumpID=0; pumpID<generalConf.nPump; pumpID++){

		for(areaID=0; areaID<generalConf.nArea; areaID++){
			/* Get necessary queue size by reading how many areas use the same pump */
			W25qxx_ReadPage((uint8_t*)&areaConf, FLASH_AREA_ADDR, areaID*sizeof(wArea_t), sizeof(wArea_t));
			if(areaConf.pumpID == pumpID+1){
				queueSize += 1;
#if (PRINTF_DEBUG == 1)
				printf("Water area %d uses pump %d. Queue size is now %d \n", areaID+1,pumpID+1, queueSize);
#endif
			}
		}
		if(queueSize > 0){
			/* Create the queue(s) */
			osMailQDef(actuationQueue, queueSize, areaConf);
			savedHandles.actQueueH[pumpID] = osMailCreate(osMailQ(actuationQueue), NULL);

			/* If memory allocation succedded pass the pointer on to the new task */
			if(savedHandles.actQueueH[pumpID] != NULL){
				pQueueHandle = (void*) &savedHandles.actQueueH[pumpID];
				queueSize = 0;

				/* Creation of actuation task(s) */
				osThreadDef(actTask, actuationTask, osPriorityAboveNormal, MAX_N_PUMP, 300);
				savedHandles.actTaskH[pumpID]= osThreadCreate(osThread(actTask), pQueueHandle);
#if (PRINTF_DEBUG == 1)
				printf("Created Queue and task for pump %d\n", pumpID+1);
#endif
			}
		}
	}
}
