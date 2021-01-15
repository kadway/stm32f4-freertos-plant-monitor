/*
 * configuration.h
 *
 *  Created on: Apr 24, 2020
 *      Author: João Gonçalves
 *      		miguel.joao.goncalves at gmail
 */


#include "configuration.h"
#include "w25qxx.h"
osMessageQId actuationTaskQueueHandle;
osThreadId actuationTaskHandle;
osThreadId spiEspComTaskHandle;
osSemaphoreId spiEspSemphHandle;

void configInit(void){
	uint16_t i=0, j= 0;
	W25qxx_ReadPage((uint8_t*)&gConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf));

	if(gConf.initCode != FLASH_DEF_INIT_CODE){
		/* Init general configurations */
		//if(1){
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
		printf("w25qxx init - Default configuration initialized to flash\r\n");
#endif
		/* Init default watering areas */

		for (i=0; i<N_AREA; i++){
			aConf[i].lastWateringtime = 0;
			aConf[i].threshold = 0;
			aConf[i].openLoop = 1;
			//starting id zero means element is not existing and should be disconsidered
			//ids to be initialized by user
			for(j=0;j<sizeof(aConf[i].sensID); j++){
				aConf[i].sensID[j]=i+10;
			}
			for(j=0;j<sizeof(aConf[i].sovID); j++){
				aConf[i].sovID[j]=i+20;
			}
			//memset(aConf[i].sensID, 0, sizeof(aConf[i].sensID));
			//memset(aConf[i].sovID, 0, sizeof(aConf[i].sovID));
			aConf[i].areaID = i; //must start at zero for later indexing
			aConf[i].pumpID = i+1; //starting id should be 1 so that zero is considered not existing (same for sensors and sovs)
			aConf[i].wateringDuration = WATERING_TIME;
			aConf[i].wateringInterval = WATERING_INTERVAL+i*5000;
			readWriteFlash((void *) &aConf[i], sizeof(wArea_t), wAreaData, WRITE, NULL, NULL);
		}
#if (PRINTF_DEBUG_CONF == 1)
		printf("w25qxx init - Default area configuration saved to flash. %d areas added.\r\n", N_AREA);
		printf("Sizes of structures: generalConf %d areaConf %d ADC data %d Actuation data %d\n", sizeof(gConf_t), sizeof(wArea_t), sizeof(mLog_t), sizeof(wLog_t));
		osDelay(5000);
#endif
	}
	else{
		for (i=0; i<gConf.nArea; i++){
			aConf[i].areaID = i;
			readWriteFlash((void *) &aConf[i], sizeof(wArea_t), wAreaData, READ, NULL, NULL);
#if (PRINTF_DEBUG_CONF == 1)
			printf("Got areaID %d with pump %d\n", aConf[i].areaID, aConf[i].pumpID);
			osDelay(5000);
#endif
		}
#if (PRINTF_DEBUG_CONF == 1)
		printf("Configuration recoverd\r\n");
		printf("generalConf %d areaConf %d adcdata %d actdata %d\n", sizeof(gConf_t), sizeof(wArea_t), sizeof(mLog_t), sizeof(wLog_t));
#endif
	}
	initActuationTasks();
}

void initActuationTasks(void){
	uint8_t pumpID, areaID;
	uint8_t queueSize = 0;
	void const * pHandle = NULL;

	/* definition for the tasks */
	osThreadDef(actTask, actuationTask, osPriorityNormal, gConf.nPump, 300);

	/* Loop over the number of existing pumps */

	for (pumpID=0; pumpID<gConf.nPump; pumpID++){

		for(areaID=0; areaID<gConf.nArea; areaID++){
			/* Get necessary queue size by reading how many areas use the same pump */
			if(aConf[areaID].pumpID == pumpID+1){
				queueSize += 1;
#if (PRINTF_DEBUG == 1)
				printf("Water area %d uses pump %d. Queue size is now %d \n", areaID+1,pumpID+1, queueSize);
				osDelay(5000);
#endif
			}
		}
		if(queueSize > 0){
			/* Create the queue(s) */
			osMessageQDef(pumpID, queueSize, uint32_t);
			//osMailQDef(actuationQueue, queueSize, aConf);
			savedHandles[pumpID].queueH = osMessageCreate(osMessageQ(pumpID), NULL);
			//savedHandles[pumpID].queueH = osMailCreate(osMailQ(actuationQueue), NULL);

			/* If memory allocation succedded pass the pointer on to the new task and timer */
			if(savedHandles[pumpID].queueH != NULL){
				pHandle = (void*) &savedHandles[pumpID];
				queueSize = 0;

				/* Creation of actuation task(s) */

				savedHandles[pumpID].taskH = osThreadCreate(osThread(actTask), pHandle);
#if (PRINTF_DEBUG == 1)
				printf("Created Queue and task for pump %d\n", pumpID+1);
				osDelay(5000);
#endif
				osTimerDef(wateringTimer, pumpTimerCallback);
				savedHandles[pumpID].timerH = osTimerCreate(osTimer(wateringTimer), osTimerOnce, NULL);

#if (PRINTF_DEBUG == 1)
				printf("Created Timer for pump %d\n", pumpID+1);
				osDelay(5000);
#endif
			}
		}
	}
}


void initSpiEspTask(void){
	/* definition and creation of spiEspSemph */
	osSemaphoreDef(spiEspSemph);
	spiEspSemphHandle = osSemaphoreCreate(osSemaphore(spiEspSemph), 1);
	/* definition and creation of spiEspComT */
	osThreadDef(spiEspComT, spiEspComTask, osPriorityRealtime, 0, 600);
	spiEspComTaskHandle = osThreadCreate(osThread(spiEspComT), NULL);
}


//To do: need to erase the sectors in case memory is full and we want to start overwriting previous data in a circular way
void readWriteFlash(void * data, uint8_t size, flashDataType type, flashOpType operationType, uint16_t* pPageNum, uint16_t* pOffset){
	uint16_t maxAreas;
	uint16_t nPage;
	uint16_t offset;
	wArea_t *pWarea;
	gConf_t *pConf;
	wLog_t *pTimeData;
	mLog_t *pAdcData;

	/* Data size must be smaller than the page size! (256 bytes) */
	assert_param(size <= w25qxx.PageSize);

	switch(type){

	case(mLogData):{
		pAdcData = (mLog_t *) data;
		if(operationType == WRITE){
			W25qxx_WritePage((uint8_t*)pAdcData,  (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		else{
			W25qxx_ReadPage((uint8_t*)pAdcData,  (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		updateOffset(FLASH_ADC_LOG_ADDR, pPageNum, FLASH_ACT_LOG_ADDR, pOffset, size);
		break;
	}
	case(wLogData):{
		pTimeData = (wLog_t *) data;
		if(operationType == WRITE){
			W25qxx_WritePage((uint8_t*)pTimeData, (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}
		else{
			W25qxx_ReadPage((uint8_t*)pTimeData, (uint32_t)*pPageNum, (uint32_t)*pOffset, (uint32_t)size);
		}

		updateOffset(FLASH_ACT_LOG_ADDR, pPageNum, w25qxx.PageCount, pOffset, size);
		break;
	}
	case(wAreaData):{
		pWarea = (wArea_t *) data;
		/* Get the page number and offset for the given area */
		maxAreas = w25qxx.PageSize / sizeof(wArea_t);
		nPage = pWarea->areaID / maxAreas;
		offset = (pWarea->areaID * sizeof(wArea_t)) - ((maxAreas -1) * sizeof(wArea_t) * nPage) - (sizeof(wArea_t)*nPage);

		if(operationType == WRITE){
			W25qxx_WritePage((uint8_t*)pWarea, (uint32_t)nPage + FLASH_AREA_ADDR, (uint32_t)offset, (uint32_t)size);

		}else{
			W25qxx_ReadPage((uint8_t*)pWarea, (uint32_t)nPage + FLASH_AREA_ADDR, (uint32_t)offset, (uint32_t)size);
		}
		break;
	}
	case(gConfData):{
		pConf = (gConf_t *) data;
		/* Must erase previous data before writing new data on top*/
		W25qxx_EraseSector(FLASH_CONFIG_SECTOR);
		W25qxx_WritePage((uint8_t*)pConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf_t));
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
	case(mLogData):{
		nPage = gConf.pageAdc - FLASH_ADC_LOG_ADDR;
		nElemPage = w25qxx.PageSize / sizeof(mLog_t);
		nElem = (nPage * nElemPage) + (gConf.pageOffsetAdc / sizeof(mLog_t));
		break;
	}
	case(wLogData):{
		nPage = gConf.pageAct - FLASH_ACT_LOG_ADDR;
		nElemPage = w25qxx.PageSize / sizeof(wLog_t);
		nElem = (nPage * nElemPage) + (gConf.pageOffsetAct / sizeof(wLog_t));
		break;
	}
	default:
		break;
	}
	return nElem;
}

void get_time(rtcTime_t *timenow)
{
	RTC_TimeTypeDef rtcTime;
	RTC_DateTypeDef rtcDate;

	/* Get the RTC current Time */
	HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN);
	/* Get the RTC current Date */
	HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN);

	timenow->Day=rtcDate.Date;
	timenow->Month=rtcDate.Month;
	timenow->Year=rtcDate.Year;
	timenow->Hours=rtcTime.Hours;
	timenow->Minutes=rtcTime.Minutes;
	timenow->Seconds=rtcTime.Seconds;
	timenow->TimeFormat=rtcTime.TimeFormat;

#if (PRINTF_DEBUG_TIME == 1)
	/* Display time Format: hh:mm:ss */
	printf("Time: %02d:%02d:%02d\n",gTime->Hours, gTime->Minutes, gTime->Seconds);
	/* Display date Format: dd-mm-yy */
	printf("Date: %02d-%02d-%2d\n",gDate->Date, gDate->Month, 2000 + gDate->Year);
#endif
}

void set_time (rtcTime_t *timenow)
{

  RTC_TimeTypeDef rtcTime;
  RTC_DateTypeDef rtcDate;

  /* Get the RTC current Time */
  HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN);

  rtcTime.Hours = timenow->Hours;
  rtcTime.Minutes = timenow->Minutes;
  rtcTime.Seconds = timenow->Seconds;
  rtcTime.TimeFormat = timenow->TimeFormat;
  rtcDate.Date = timenow->Day;
  rtcDate.Month = timenow->Month;
  rtcDate.Year = timenow->Year;

  if (HAL_RTC_SetTime(&hrtc, &rtcTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_RTC_SetDate(&hrtc, &rtcDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x32F2); // backup register
}

