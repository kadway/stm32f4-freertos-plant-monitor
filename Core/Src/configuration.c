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
	W25qxx_ReadPage((uint8_t*)&gConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf));

	//if(generalConf.initCode != FLASH_DEF_INIT_CODE){
		if(1){
		/* Init general configurations */
		gConf.initCode=FLASH_DEF_INIT_CODE;
		gConf.nArea = N_AREA;
		gConf.nSens = N_SENS;
		gConf.nPump = N_PUMP;
		gConf.nSov = N_SOV;
		gConf.pageAdc = FLASH_ADC_LOG_ADDR;
		gConf.adcConvTimeInterval = MEAS_INTERVAL;
		gConf.pageAct = FLASH_ACT_LOG_ADDR;
		gConf.pageOffsetAct = 0;
		gConf.pageOffsetAdc = 0;
		W25qxx_EraseChip();
		/* Save to external flash */
		W25qxx_WritePage((uint8_t*)&gConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf_t));
#if (PRINTF_DEBUG == 1)
		printf("w25qxx init - Default general configuration initialized to flash\r\n");
#endif
		/* Init default watering areas */


		for (i=0; i<N_AREA; i++){
			aConf[i].lastWateringtime = 0;
			aConf[i].threshold = 0;
			aConf[i].openLoop = 1;
			//starting id zero means element is not existing and should be disconsidered
			//ids to be initialized by user
			memset(aConf[i].sensID, 0, sizeof(aConf[i].sensID));
			memset(aConf[i].sovID, 0, sizeof(aConf[i].sovID));

			aConf[i].areaID = i; //must start at zero for later indexing
			aConf[i].pumpID = i+1; //starting id should be 1 so that zero is considered not existing (same for sensors and sovs)
			aConf[i].wateringDuration = WATERING_TIME;
			aConf[i].wateringInterval = WATERING_INTERVAL+i*5000;
			readWriteFlash((void *) &aConf, sizeof(wArea_t), wAreaData, WRITE, NULL, NULL);
		}
#if (PRINTF_DEBUG == 1)
			printf("w25qxx init - Default area configuration saved to flash. %d areas added.\r\n", N_AREA);
			printf("Sizes of structures: generalConf %d areaConf %d\n", sizeof(gConf_t), sizeof(wArea_t));
			osDelay(5000);
			//printf("\n ----monitor conf: -----\n");
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
	void const * pHandle = NULL;
	/* Loop over the number of existing pumps */

	for (pumpID=0; pumpID<gConf.nPump; pumpID++){

		for(areaID=0; areaID<gConf.nArea; areaID++){
			/* Get necessary queue size by reading how many areas use the same pump */
			if(aConf[areaID].pumpID == pumpID+1){
				queueSize += 1;
#if (PRINTF_DEBUG == 1)
				printf("Water area %d uses pump %d. Queue size is now %d \n", areaID+1,pumpID+1, queueSize);
#endif
			}
		}
		if(queueSize > 0){
			/* Create the queue(s) */
			osMessageQDef(actuationQueue, queueSize, uint32_t);
			//osMailQDef(actuationQueue, queueSize, aConf);
			savedHandles[pumpID].queueH = osMessageCreate(osMessageQ(actuationQueue), NULL);
			//savedHandles[pumpID].queueH = osMailCreate(osMailQ(actuationQueue), NULL);

			/* If memory allocation succedded pass the pointer on to the new task and timer */
			if(savedHandles[pumpID].queueH != NULL){
				pHandle = (void*) &savedHandles[pumpID];
				queueSize = 0;

				/* Creation of actuation task(s) */
				osThreadDef(actTask, actuationTask, osPriorityAboveNormal, MAX_N_PUMP, 300);
				savedHandles[pumpID].taskH = osThreadCreate(osThread(actTask), pHandle);
#if (PRINTF_DEBUG == 1)
				printf("Created Queue and task for pump %d\n", pumpID+1);
#endif
				osTimerDef(wateringTimer, pumpTimerCallback);
				savedHandles[pumpID].timerH = osTimerCreate(osTimer(wateringTimer), osTimerOnce, NULL);

#if (PRINTF_DEBUG == 1)
				printf("Created Timer for pump %d\n", pumpID+1);
#endif
			}
		}
	}
}

void readWriteFlash(void * data, uint8_t size, flashDataType type, flashOpType operationType, uint16_t* pPageNum, uint16_t* pOffset){
	uint16_t nBytes;
	uint16_t nPage;
	uint16_t nBytesPage;
	uint16_t offset;
	wArea_t *pWarea;

	/* Data size must be smaller than the page size! (256 bytes) */
	assert_param(size <= w25qxx.PageSize);

	/* Must take a mutex here because several parallel tasks may try to access the global confifuration structures in parallel */
	/* to do */

	switch(type){

	case(mMeasTimeData):{

		if(operationType == WRITE){
			W25qxx_WritePage((uint8_t*)data,  (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		else{
			W25qxx_ReadPage((uint8_t*)data,  (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		updateOffset(FLASH_ADC_LOG_ADDR, pPageNum, FLASH_ACT_LOG_ADDR, pOffset, size);
		break;
	}
	case(wTimeData):{
		if(operationType == WRITE){
			W25qxx_WritePage((uint8_t*)data, (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		else{
			W25qxx_ReadPage((uint8_t*)data, (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}

		updateOffset(FLASH_ACT_LOG_ADDR, pPageNum, w25qxx.PageCount, pOffset, size);
		break;
	}
	case(wAreaData):{
		pWarea = data;
		/* Get the page number and offset for the given area */
		nBytes = pWarea->areaID * sizeof(wArea_t);
		nPage =  nBytes % w25qxx.PageSize;
		nBytesPage = (w25qxx.PageSize % sizeof(wArea_t)) * sizeof(wArea_t);
		offset = nBytes - nPage*nBytesPage;

		if(operationType == WRITE){
			if(pWarea->areaID == 0){
				/* If writting the first area then erase the sector before.*/
				W25qxx_EraseSector(FLASH_AREA_SECTOR);
			}
			W25qxx_WritePage((uint8_t*)data, (uint32_t)nPage + FLASH_AREA_ADDR, (uint32_t)offset, (uint32_t)size);

		}else{
			W25qxx_ReadPage((uint8_t*)data, (uint32_t)nPage + FLASH_AREA_ADDR, (uint32_t)offset, (uint32_t)size);
		}
		break;
	}
	case(gConfData):{
		/* Must erase previous data before writing new data on top*/
		W25qxx_EraseSector(FLASH_CONFIG_SECTOR);
		W25qxx_WritePage((uint8_t*)&data, FLASH_CONFIG_ADDR, 0, sizeof(gConf_t));
		break;
	}

	default:
		printf("Invalid data type when saving to flash!\n");

	}
}

void updateOffset(uint16_t startPage, uint16_t* actualPage, uint16_t endPage, uint16_t* offset, uint8_t size){
	/* Update the page offset and page number for the next write*/
	if(*offset + size >= w25qxx.PageSize){
		*actualPage += 1;
		// check if reached end of reserved memory, if so start writing from the start
		if(*actualPage >= endPage){
			*actualPage = startPage;
		}
		*offset = 0;
	}
	else{
		*offset += size;
	}
}

uint32_t getNumElements(flashDataType type){
	uint32_t nElem = 0;
	uint16_t nPage = 0;
	uint16_t nElemPage = 0;

	switch(type){
	case(mMeasTimeData):{
		nPage = gConf.pageAdc - FLASH_ADC_LOG_ADDR;
		nElemPage = w25qxx.PageSize % sizeof(mMeasTime_t);
		nElem = (nPage * nElemPage) + (gConf.pageOffsetAdc % sizeof(mMeasTime_t));
		break;
	}
	case(wTimeData):{
		nPage = gConf.pageAct - FLASH_ACT_LOG_ADDR;
		nElemPage = w25qxx.PageSize % sizeof(wTime_t);
		nElem = (nPage * nElemPage) + (gConf.pageOffsetAct % sizeof(wTime_t));
		break;
	}
	default:
		break;
	}
	return nElem;
}

