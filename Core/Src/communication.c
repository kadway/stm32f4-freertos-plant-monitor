/*
 * communication.c
 *
 *  Created on: Apr 24, 2020
 *      Author: João Gonçalves
 *      	    miguel.joao.goncalves at gmail
 */

#include "communication.h"

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi){
	HAL_SPI_Abort_IT(hspi);
	HAL_SPI_MspDeInit(hspi);
	HAL_SPI_MspInit(hspi);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	//Forced reset from ESP8266
	HAL_NVIC_SystemReset();
}


/**
 * @brief  Function implementing the thread for handling spi commands from ESP8266
 * @param  argument: Not used
 * @retval None
 */

void spiEspComTask(void const * argument)
{
	uint8_t ackSlave = 0xCE;
	uint8_t command = 0x00;

	uint32_t nElm = 0;
	uint32_t replyMaster = 0;
	wArea_t dummywArea; //dummy for spi transaction
	gConf_t dummygConf; //dummy for spi transaction
	uint16_t i=0;
	uint16_t pageNum = 0;
	uint16_t offset = 0;
	mLog_t moistData;
	mLog_t dummyMoist;
	wLog_t actuationData;
	wLog_t dummyActuation;

#if (PRINTF_DEBUG_COMM == 1)
	uint32_t loop=0;
#endif
	/* need to take the semaphor the first time....*/
	osSemaphoreWait (spiEspSemphHandle, osWaitForever);

	for(;;){
#if (PRINTF_DEBUG_COMM == 1)
		loop+=1;
		printf("-> spiEspComTask loop %lu\n", loop);
#endif
		/* reset data ready pin */
		//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
		/* Send slave Ack */
		HAL_SPI_TransmitReceive_DMA(&hspi2, &ackSlave, &command, sizeof(uint8_t));
		osSemaphoreWait (spiEspSemphHandle, osWaitForever);

		switch(command){
		/* SEND CONFIGURATION DATA*/
		case ESP_GET_CONF:
			/* Send general configuration data */
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
				osDelay(1000);
			}
			else{
				readWriteFlash((void *) & gConf, sizeof(gConf_t), gConfData, READ, NULL, NULL);
				osMutexRelease(flashMutexHandle);
				HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&gConf, (uint8_t*)&dummygConf, sizeof(gConf_t));
				/*Signal data ready for transfer */
				dataIsReady();
				if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
					//HAL_SPI_Abort_IT(&hspi2);
					printf("Error or timeout getting spiEspSemphHandle in communication task (send gConf)\n");
					osDelay(1000);
				}
			}
			break;
			/* END SEND CONFIGURATION DATA*/

		case ESP_GET_AREA:
			/* Send number of wArea elements */
			nElm = gConf.nArea;
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&replyMaster, sizeof(uint32_t));
			/*Signal data ready for transfer */
			dataIsReady();

			if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
				printf("Error or timeout getting spiEspSemphHandle in communication task (GET Area, send num elements)\n");
				osDelay(1000);
				break;
			}

			if ( replyMaster == ACK_MASTER){

				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task\n");
					osDelay(1000);
					break;
				}
				/* read wArea data from external flash*/
				for (i=0; i<nElm; i++){
					readWriteFlash((void *) &aConf[i], sizeof(wArea_t), wAreaData, READ, NULL, NULL);
				}
				osMutexRelease(flashMutexHandle);

				for (i=0; i<nElm; i++){
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*) &aConf[i],(uint8_t*) &dummywArea, sizeof(wArea_t));
					/*Signal data ready for transfer */
					dataIsReady();
					if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
						printf("Error or timeout getting spiEspSemphHandle in communication task (Send wArea)\n");
						osDelay(1000);
						break;
					}

				}
			}

			break;

		case ESP_SET_CONF:
			/* Update general configuration data */
			dummygConf = gConf;
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&dummygConf, (uint8_t*)&gConf, sizeof(gConf_t));
			/*Signal data ready for transfer */
			dataIsReady();
			if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
				//HAL_SPI_Abort_IT(&hspi2);
				printf("Error or timeout getting spiEspSemphHandle, comm task (SET conf)\n");
				osDelay(1000);
				break;
			}

			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
				osDelay(1000);
				break;
			}
			/* Stop control and adc tasks */
			osThreadSuspend(controlTaskHandle);
			osThreadSuspend(adcTaskHandle);

			readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
			osMutexRelease(flashMutexHandle);

			break;

		case ESP_SET_AREA:
			/* Update area configuration data */
			for (i=0; i<gConf.nArea; i++){
				HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&dummywArea, (uint8_t*)&aConf[i], sizeof(wArea_t));
				/*Signal data ready for transfer */
				dataIsReady();
				if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
					printf("Error or timeout getting spiEspSemphHandle, comm task (SET area conf)\n");
					osDelay(1000);
					break;
				}
			}

			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
				osDelay(1000);
				break;
			}

			/* Stop control and adc tasks */
			osThreadSuspend(controlTaskHandle);
			osThreadSuspend(adcTaskHandle);

			/* Save the updated area configuration to external flash */
			W25qxx_EraseSector(FLASH_AREA_SECTOR);
			for (i=0; i<gConf.nArea; i++){
				readWriteFlash((void *)&aConf[i], sizeof(wArea_t), wAreaData, WRITE, NULL, NULL);
			}
			printf("%d areas written to flash!\n", i);
			/* Save the updated area configuration to external flash */

			for (i=0; i<gConf.nArea; i++){
				readWriteFlash((void *)&aConf[i], sizeof(wArea_t), wAreaData, READ, NULL, NULL);
			}
			osMutexRelease(flashMutexHandle);
			/* To do: clear old logged data before booting with new configuration.
			 * The actual memory offset may have changed in between reading and setting new general_configuration*/
			//clearLog();
			/* restart with new configurations */
			HAL_NVIC_SystemReset();
			break;

		case ESP_GET_DATA_ADC:
			/* Send ADC data */
			/* Send number of elements */
			nElm = getNumElements(mLogData);
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&replyMaster, sizeof(uint32_t));
			/*Signal data ready for transfer */
			dataIsReady();
			if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
				printf("Error or timeout getting spiEspSemphHandle in communication task\n");
				osDelay(1000);
				break;
			}

			if ( replyMaster == ACK_MASTER){
				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task ESP_GET_DATA_ADC \n");
					osDelay(1000);
					break;
				}

				/* send mLogData  */
				pageNum = FLASH_ADC_LOG_ADDR;
				offset = 0;
				for (i=0; i<nElm; i++){
					readWriteFlash((void *)&moistData, sizeof(mLog_t), mLogData, READ, &pageNum, &offset);
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&moistData, (uint8_t*)&dummyMoist, sizeof(mLog_t));
					/*Signal data ready for transfer */
					dataIsReady();
					if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
						printf("Error or timeout getting spiEspSemphHandle in communication task(send mMeasTime)\n");
						osDelay(1000);
						break;
					}
				}
				/* Give the mutex only after all data has been transfered or when timeout occurs*/
				osMutexRelease(flashMutexHandle);
			}

			break;
		case ESP_GET_DATA_ACT:
			/* Send Actuation data */
			/* Send number of elements */
			nElm = getNumElements(wLogData);
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&replyMaster, sizeof(uint32_t));
			/*Signal data ready for transfer */
			dataIsReady();
			if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
				printf("Error or timeout getting spiEspSemphHandle in communication task ESP_GET_DATA_ACT\n");
				osDelay(1000);
				break;
			}

			if ( replyMaster == ACK_MASTER){
				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task\n");
					osDelay(1000);
					break;
				}

				/* send wLogData data */
				pageNum = FLASH_ACT_LOG_ADDR;
				offset = 0;
				for (i=0; i<nElm; i++){
					readWriteFlash((void *)&actuationData, sizeof(wLog_t), wLogData, READ, &pageNum, &offset);
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&actuationData, (uint8_t*)&dummyActuation, sizeof(wLog_t));
					/*Signal data ready for transfer */
					dataIsReady();
					if(osSemaphoreWait (spiEspSemphHandle, 1000)!= osOK){
						printf("Error or timeout getting spiEspSemphHandle in communication task(send mMeasTime)\n");
						osDelay(1000);
						break;
					}
				}
				/* Give the mutex only after all data has been transfered */
				osMutexRelease(flashMutexHandle);

			}
			break;

		case ESP_CLEAR_DATA_ADC:
			/* Clear flash sectors where ADC data is being stored */
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
			}
			else{
				//To do: clear only adc data
				osMutexRelease(flashMutexHandle);
			}
			break;

		case ESP_CLEAR_DATA_ACT:
			/* Clear flash sectors where Actuation data is being stored */
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
			}
			else{
				//To do: clear only actuation data
				osMutexRelease(flashMutexHandle);
			}
			break;

		case ESP_CLEAR_LOG:
			clearLog();
			break;
		case ESP_CLEAR_CONF:
			/* Clear general configuration data -> fallback to default init upon restart*/
			gConf.initCode = 0;
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
				osDelay(1000);
				break;
			}
			readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
			/* restart with new configurations */
			HAL_NVIC_SystemReset();
			break;
		case ESP_STOP_CONTROL_TASK:
			osThreadSuspend(controlTaskHandle);
			osThreadSuspend(adcTaskHandle);
			break;
		case ESP_RESUME_CONTROL_TASK:
			osThreadResume(controlTaskHandle);
			osThreadResume(adcTaskHandle);
			break;
		}
	}
}

void dataIsReady(void){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
}

/* Clear all logged data sectors where ADC and Actuation data is being stored */
void clearLog(void){
	/* To check: not working!
	 int i;
		if(osMutexWait(flashMutexHandle,200)!= osOK){
		printf("Error or timeout getting Mutex in communication task\n");
	}
	else{
		osThreadSuspend(controlTaskHandle);
		osThreadSuspend(adcTaskHandle);
		gConf.pageAct = FLASH_ACT_LOG_ADDR;
		gConf.pageAdc = FLASH_ADC_LOG_ADDR;
		gConf.pageOffsetAct = 0;
		gConf.pageOffsetAdc = 0;
		readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);

		for(i=0; i<32;i++){
			W25qxx_EraseSector(FLASH_ADC_LOG_BLOCK_NUM+i);
		}
		osThreadResume(controlTaskHandle);
		osThreadResume(adcTaskHandle);
		osMutexRelease(flashMutexHandle);
	}*/
}

