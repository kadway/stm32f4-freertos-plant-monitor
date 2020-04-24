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
		for (i=0; i<generalConf.nArea; i++){
			W25qxx_ReadPage((uint8_t*)&areaConf, FLASH_AREA_ADDR, i*sizeof(areaConf), sizeof(areaConf));

			/*Check if pumpID is valid. must be a number less than or equal to defined number of used pumps */
			if( areaConf.pumpID <= generalConf.nPump){

				if(areaConf.openLoop){
					/* In open loop just check if the watering interval has passed */
					elapsedTime=HAL_GetTick()- areaConf.lastWateringtime >= areaConf.wateringInterval;
#if (PRINTF_DEBUG == 1)
					printf("Time elapsed\n");
#endif
				}
				else{
					/* In closed loop make average of measurements for the sensors in this area and compare with the threshold */
					for (i=0;i<sizeof(areaConf.sensID);i++){
						if(areaConf.sensID[i]>0 && areaConf.sensID[i]<sizeof(lastAdcConv.meas)){
							id=areaConf.sensID[i]-1;
							average = (uint32_t) lastAdcConv.meas->reading[id];
							count += 1;
						}
					}
					average = average / count;
				}
				if((areaConf.openLoop && elapsedTime>=areaConf.wateringInterval) || average >= areaConf.threshold){

					/* Get a pointer to the a memory block previously alocated in configInit */
					pointerToMail = osMailAlloc(savedHandles.actQueueH[areaConf.pumpID-1], 100);

					if(pointerToMail!=NULL){
						/* If allocation is successful copy the area data into the new allocated memory */
						memcpy(pointerToMail, &areaConf, sizeof(wArea_t));
						/*put the data in the queue */
						queueErr = osMailPut(savedHandles.actQueueH[areaConf.pumpID-1], pointerToMail);

						if(queueErr == osErrorParameter || queueErr == osErrorOS ){
							/*if some error occured release the previous memory */
							osMailFree(savedHandles.actQueueH[i], pointerToMail);
#if (PRINTF_DEBUG == 1)
							printf("Failed to put a message in queue\n");
							osDelay(2000);
#endif
						}
#if (PRINTF_DEBUG == 1)
						else{
							printf("ControlTask, has area %d ->put in queue area with pump id %d queue status: %d\n", i+1, areaConf.pumpID, (uint32_t) queueErr);
							osDelay(2000);
						}
#endif
					}

#if (PRINTF_DEBUG == 1)
					else{
						printf("NULL pointer when trying to get memory block previously alocated in configInit\nInvalid pumpID configured?!\n");
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
	uint32_t *myID;
	wArea_t *pointerToMail;
	osMailQId *pToQueueHandle = (osMailQId *)argument;

	for(;;){
		mail = osMailGet(*pToQueueHandle, osWaitForever);
		pointerToMail = (wArea_t *) mail.value.p;
#if (PRINTF_DEBUG == 1)
		myID = (uint32_t *)osThreadGetId();
		printf("Actuation task with index %lu runs, and got mail with pumpID %d\n", *myID, pointerToMail->pumpID);
#endif

		osMailFree(*pToQueueHandle, mail.value.p);
		osDelay(1000);
	}
}
