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

/*uint32_t elapsedtimeSeconds (time_t dateTime){

	return dateTime->time.Hours * 60 + dateTime->time.Minutes * 60 + dateTime->time.Seconds;
}*/

void controlTask(void const * argument){
	uint8_t areaIdx, sIdx, id;
	uint32_t elapsedTime;
	uint32_t average;
	uint16_t count;
	uint32_t tickNow;
#if (PRINTF_DEBUG_CTRL == 1)
	uint32_t loop = 0;
#endif
	for(;;){
#if (PRINTF_DEBUG_CTRL == 1)
		loop += 1;
#endif
		/* Read each area configured in memory */
		for (areaIdx=0; areaIdx<gConf.nArea; areaIdx++){

			/*Check if pumpID is valid. must be a number less than or equal to defined number of used pumps */
			if( aConf[areaIdx].pumpID <= gConf.nPump && aConf[areaIdx].pumpID > 0){

				if(aConf[areaIdx].openLoop){
					/* In open loop just check if the watering interval has passed */
					tickNow = HAL_GetTick();
					elapsedTime = tickNow - aConf[areaIdx].lastWateringtime;
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
#if (PRINTF_DEBUG_CTRL == 1)
					printf("Time elapsed %lu for area %d\n", elapsedTime, aConf[areaIdx].areaID);
#endif
						/*put the data in the queue */
						if(osMessagePut(savedHandles[aConf[areaIdx].pumpID-1].queueH, (uint32_t)&aConf[areaIdx],200) != osOK){
							printf("Failed to put a message in queue\n");
							osDelay(100);
						}
#if (PRINTF_DEBUG_CTRL == 1)
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
#if (PRINTF_DEBUG_CTRL == 1)
		printf("-> ControlTask loop %lu\n", loop);
#endif
		osDelay(CONTROL_TASK_LOOP_TIME);
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
	wLog_t wateringTime;


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
		printf("Act task %lu. Area %d, pump %d\n", *id, pAreaConf->areaID, pAreaConf->pumpID);
#endif
		/* Update the area configuration with the new time of watering*/
		pAreaConf->lastWateringtime = HAL_GetTick();

		osMutexWait(flashMutexHandle,osWaitForever);
		/* Prepare data for logging in external flash memory */
		wateringTime.areaID = pAreaConf->areaID;
		wateringTime.duration = pAreaConf->wateringDuration;
		get_time(&wateringTime.dateTime);

		/* Save the time of watering to flash*/
		readWriteFlash((void *) &wateringTime, sizeof(wLog_t), wLogData, WRITE, &gConf.pageAct, &gConf.pageOffsetAct);
		/* Update the current page number and offset in the flash configuration structure */
		readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
		osMutexRelease(flashMutexHandle);

		/* Activate the respective pumps and solenoid valves */
		/* to do... */

		/*Start the watering timer*/
		osTimerStart(pHandle->timerH, pAreaConf->wateringDuration);

#if (PRINTF_DEBUG_ACT == 1)
		printf("Act task %lu start timer %lu ms\n", *id, pAreaConf->wateringDuration);
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
			readWriteFlash((void *) &wateringTime, sizeof(wLog_t), wTimeData, READ, &actualPage, &offset);
			printf("READ DATA->  Aread %d, Elapsed time %lu, watering duration %lu\n",wateringTime.areaID, wateringTime.time, wateringTime.duration);
		}while(actualPage < gConf.pageAct || offset < gConf.pageOffsetAct);
		osMutexRelease(flashMutexHandle);
#endif
	}
}

void pumpTimerCallback(void const * argument){
#if (PRINTF_DEBUG_TIMER == 1)
	uint32_t *id;
#endif
	osTimerId myID = (osTimerId) argument;
	uint32_t i = 0;

	while(myID != savedHandles[i].timerH){
		i++;
	}
#if (PRINTF_DEBUG_TIMER == 1)
	id = (uint32_t*) myID;
	printf("Timer id %lu resumes task\n", *id );
	osDelay(1000);
#endif

	osThreadResume(savedHandles[i].taskH);
}
