/*
 * control.c
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */

/**
 * @brief  Function implementing the thread that handles the control of the actuation tasks
 * @param  argument: Not used
 * @retval None
 */

#include "control.h"
#include "main.h"



void controlTask(void const * argument){
	uint8_t i, id;
	void * pointerToMail = NULL;
	osStatus queueErr;
	uint32_t elapsedTime;
	uint32_t average;
	uint16_t count;

	for(;;){
		/* Read each area configured in memory */
		for (i=0; i<gConf.nArea; i++){

			W25qxx_ReadPage((uint8_t*)&aConf, FLASH_AREA_ADDR, i*sizeof(aConf), sizeof(aConf));

			/*Check if pumpID is valid. must be a number less than or equal to defined number of used pumps */
			if( aConf.pumpID <= gConf.nPump){

				if(aConf.openLoop){
					/* In open loop just check if the watering interval has passed */
					elapsedTime=HAL_GetTick()- aConf.lastWateringtime;
				}
				else{
					/* In closed loop make average of measurements for the sensors in this area and compare with the threshold */
					for (i=0;i<sizeof(aConf.sensID);i++){
						if(aConf.sensID[i]>0 && aConf.sensID[i]<sizeof(lastAdcConv.meas)){
							id=aConf.sensID[i]-1;
							average = (uint32_t) lastAdcConv.meas[id];
							count += 1;
						}
					}
					average = average / count;
				}
				if((aConf.openLoop && elapsedTime>=aConf.wateringInterval) || (!(aConf.openLoop) && average >= aConf.threshold)){
#if (PRINTF_DEBUG == 1)
					printf("Time elapsed for area %d\n", aConf.areaID);
#endif
					/* Get a pointer to the a memory block previously alocated in configInit */
					pointerToMail = osMailAlloc(savedHandles[aConf.pumpID-1].queueH, 100);

					if(pointerToMail!=NULL){
						/* If allocation is successful copy the area data into the new allocated memory */
						memcpy(pointerToMail, &aConf, sizeof(wArea_t));
						/*put the data in the queue */
						queueErr = osMailPut(savedHandles[aConf.pumpID-1].queueH, pointerToMail);

						if(queueErr == osErrorParameter || queueErr == osErrorOS ){
							/*if some error occured release the previous memory */
							osMailFree(savedHandles[i].queueH, pointerToMail);
#if (PRINTF_DEBUG == 1)
							printf("Failed to put a message in queue\n");
							osDelay(2000);
#endif
						}
#if (PRINTF_DEBUG == 1)
						else{
							printf("ControlTask, has area %d ->put in queue area with pump id %d queue status: %lu\n", i+1, aConf.pumpID, (uint32_t) queueErr);
							osDelay(2000);
						}
#endif
					}

#if (PRINTF_DEBUG == 1)
					else{
						printf("NULL pointer when trying to get memory block previously alocated in configInit\n");
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
	osEvent mail;
	osThreadId myID;
	uint32_t *id;
	wArea_t *pointerToMail;
	actTaskQueueH_t *pHandle = (actTaskQueueH_t *)argument;
	wTime_t wateringTime;
	uint16_t actualPage, offset;
	myID = osThreadGetId();
	id = (uint32_t*) myID;

	for(;;){
		mail = osMailGet(pHandle->queueH, osWaitForever);
		pointerToMail = (wArea_t *) mail.value.p;

#if (PRINTF_DEBUG == 1)
		printf("Actuation task with index %lu runs, and got mail with pumpID %d\n", *id, pointerToMail->pumpID);
#endif
		wateringTime.areaID = pointerToMail->areaID;
		wateringTime.duration = pointerToMail->wateringDuration;
		//temporary save elapsed time instead of actual time
		wateringTime.time = HAL_GetTick()- pointerToMail->lastWateringtime;
		/* Update time of watering */
		pointerToMail->lastWateringtime = HAL_GetTick();
		/* Update the area configuration in flash with the new time of watering*/
		readWriteFlash((void *) pointerToMail, sizeof(wArea_t), wAreaData, WRITE, NULL, NULL);

		/* Save the time of watering to flash */
		readWriteFlash((void *) &wateringTime, sizeof(wTime_t), wTimeData, WRITE, &gConf.pageAct, &gConf.pageOffsetAct);

		/* Activate the respective pumps and solenoid valves */
		/* to do... */

		/*Start the watering timer*/
		osTimerStart(pHandle->timerH, pointerToMail->wateringDuration);

#if (PRINTF_DEBUG == 1)
		printf("Actuation task with index %lu runs, timer for %lu seconds started\n", *id, pointerToMail->wateringDuration);
#endif
		osThreadSuspend(pHandle->taskH);

		/* De-activate the respective pumps and solenoid valves */
		/* to do... */

#if (PRINTF_DEBUG == 1)
		printf("Actuation task with index %lu resumes after timer callback\n", *id);
		actualPage = FLASH_ACT_LOG_ADDR;
		offset =  0;
		do{
			readWriteFlash((void *) &wateringTime, sizeof(wTime_t), wTimeData, READ, &actualPage, &offset);
			printf("READ DATA->  Aread %d, Elapsed time %lu, watering duration %lu\n",wateringTime.areaID, wateringTime.time, wateringTime.duration);
		}while(actualPage <= gConf.pageAct && offset< gConf.pageOffsetAct);

#endif
		osMailFree(pHandle->queueH, mail.value.p);
		osDelay(1000);
	}
}

void pumpTimerCallback(void const * argument){
	uint32_t *id;
	osTimerId myID = (osTimerId) argument;
	uint32_t i = 0;

	while(myID != savedHandles[i].timerH){
		i++;
	}

	id = (uint32_t*) myID;

#if (PRINTF_DEBUG == 1)
		printf("Timer id %lu resumes task\n", *id );
		osDelay(1000);
#endif

	osThreadResume(savedHandles[i].taskH);
}
