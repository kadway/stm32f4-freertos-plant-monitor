/*
 * control.c
 *
 *  Created on: Apr 24, 2020
 *      Author: João Gonçalves
 *      		miguel.joao.goncalves at gmail
 */

/**
 * @brief  Function implementing the thread that handles the control of the actuation tasks
 * @param  argument: Not used
 * @retval None
 */

#include "control.h"
#include "main.h"

void controlTask(void const * argument){
	uint8_t areaIdx, sIdx, id;
	uint32_t elapsedTime;
	uint32_t average;
	uint16_t count;

	for(;;){
		/* Read each area configured in memory */
		for (areaIdx=0; areaIdx<gConf.nArea; areaIdx++){

			/*Check if pumpID is valid. must be a number less than or equal to defined number of used pumps */
			if( aConf[areaIdx].pumpID <= gConf.nPump){

				if(aConf[areaIdx].openLoop){
					/* In open loop just check if the watering interval has passed */
					elapsedTime=HAL_GetTick()- aConf[areaIdx].lastWateringtime;
				}
				else{
					/* In closed loop make average of measurements for the sensors in this area and compare with the threshold */
					for (sIdx=0; sIdx<sizeof(aConf[areaIdx].sensID); sIdx++){
						if(aConf[areaIdx].sensID[sIdx]>0 && aConf[areaIdx].sensID[sIdx]<sizeof(lastAdcConv.meas)){
							id=aConf[areaIdx].sensID[sIdx]-1;
							average = (uint32_t) lastAdcConv.meas[id];
							count += 1;
						}
					}
					average = average / count;
				}
				if((aConf[areaIdx].openLoop && elapsedTime>=aConf[areaIdx].wateringInterval) || (!(aConf[areaIdx].openLoop) && average >= aConf[areaIdx].threshold)){
#if (PRINTF_DEBUG_ACT == 1)
					printf("Time elapsed %lu for area %d\n", elapsedTime, aConf[areaIdx].areaID);
#endif
						/*put the data in the queue */
						if(osMessagePut(savedHandles[aConf[areaIdx].pumpID-1].queueH, (uint32_t)&aConf[areaIdx],200) != osOK){
							printf("Failed to put a message in queue\n");
							osDelay(100);
						}
#if (PRINTF_DEBUG_ACT == 1)
						else{
							printf("ControlTask, has area %d ->put in queue area with pump id %d\n", aConf[areaIdx].areaID, aConf[areaIdx].pumpID);
							osDelay(2000);
						}
#endif
					}

				/* Set to zero for next iteration */
				elapsedTime = 0;
				average = 0;
			}
		}
		osDelay(1000);
	}
}

/**
 * @brief  Function implementing the thread that handles watering for areas
 * @param  argument: Not used
 * @retval None
 */

void actuationTask(void const * argument){
	osEvent message;
	wArea_t *pAreaConf;
	actTaskQueueH_t *pHandle = (actTaskQueueH_t *)argument;
	wTime_t wateringTime;

#if (PRINTF_DEBUG_ACT == 1)
	uint32_t *id;
	osThreadId myID;
	myID = osThreadGetId();
	id = (uint32_t*) myID;
#endif
#if (PRINTF_DEBUG_ACT_FLASH == 1)
	uint16_t actualPage, offset;
#endif

	for(;;){
		message = osMessageGet(pHandle->queueH,osWaitForever);
		pAreaConf = (wArea_t *) message.value.p;
#if (PRINTF_DEBUG_ACT == 1)
		printf("Actuation task with index %lu runs, and got mail with pumpID %d\n", *id, pAreaConf->pumpID);
#endif
		/* Update the area configuration with the new time of watering*/
		pAreaConf->lastWateringtime = HAL_GetTick();

		/* Prepare data for logging in external flash memory */
		wateringTime.areaID = pAreaConf->areaID;
		wateringTime.duration = pAreaConf->wateringDuration;
		wateringTime.time = HAL_GetTick();

		osMutexWait(flashMutexHandle,osWaitForever);
		/* Save the time of watering to flash*/
		readWriteFlash((void *) &wateringTime, sizeof(wTime_t), wTimeData, WRITE, &gConf.pageAct, &gConf.pageOffsetAct);
		/* Update the current page number and offset in the flash configuration structure */
		readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
		osMutexRelease(flashMutexHandle);

		/* Activate the respective pumps and solenoid valves */
		/* to do... */

		/*Start the watering timer*/
		osTimerStart(pHandle->timerH, pAreaConf->wateringDuration);

#if (PRINTF_DEBUG_ACT == 1)
		printf("Actuation task with index %lu runs, timer for %lu mili econds started\n", *id, pAreaConf->wateringDuration);
		get_time();
#endif
		osThreadSuspend(pHandle->taskH);

		/* De-activate the respective pumps and solenoid valves */
		/* to do... */

#if (PRINTF_DEBUG_ACT_FLASH == 1)
		osMutexWait(flashMutexHandle,osWaitForever);
		printf("Actuation task with index %lu resumes after timer callback\n", *id);
		actualPage = FLASH_ACT_LOG_ADDR;
		offset =  0;
		do{
			readWriteFlash((void *) &wateringTime, sizeof(wTime_t), wTimeData, READ, &actualPage, &offset);
			printf("READ DATA->  Aread %d, Elapsed time %lu, watering duration %lu\n",wateringTime.areaID, wateringTime.time, wateringTime.duration);
		}while(actualPage < gConf.pageAct || offset < gConf.pageOffsetAct);
		osMutexRelease(flashMutexHandle);
#endif
	}
}

void pumpTimerCallback(void const * argument){
#if (PRINTF_DEBUG_ACT == 1)
	uint32_t *id;
#endif
	osTimerId myID = (osTimerId) argument;
	uint32_t i = 0;

	while(myID != savedHandles[i].timerH){
		i++;
	}
#if (PRINTF_DEBUG_ACT == 1)
	id = (uint32_t*) myID;
	printf("Timer id %lu resumes task\n", *id );
	osDelay(1000);
#endif

	osThreadResume(savedHandles[i].taskH);
}
