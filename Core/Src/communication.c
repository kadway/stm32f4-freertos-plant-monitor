/*
 * communication.c
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */

#include "communication.h"

/**
 * @brief  Function implementing the thread for handling spi commands from ESP8266
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END spiEspComTask */
void spiEspComTask(void const * argument)
{
	uint8_t ackSlave = 0xCE;
	uint8_t command = 0x00;
	uint32_t ackMaster = 0xE3E3E3E3;
	uint32_t nElm = 0;
	uint32_t dummySize = 0;
	wArea_t dummywArea; //dummy for spi transaction
	gConf_t dummygConf; //dummy for spi transaction
	uint16_t i=0;
	uint16_t pageNum = 0;
	uint16_t offset = 0;
	mMeasTime_t moistData;
	mMeasTime_t dummyMoist;
	wTime_t actuationData;
	wTime_t dummyActuation;
	/* need to take the semaphor the first time....*/
	osSemaphoreWait (spiEspSemphHandle, osWaitForever);

	for(;;){
#if (PRINTF_DEBUG_COM == 1)
		printf("Spi loop..\n");
#endif
		/* Send slave Ack */
		HAL_SPI_TransmitReceive_DMA(&hspi2, &ackSlave, &command, sizeof(uint8_t));
		osSemaphoreWait (spiEspSemphHandle, osWaitForever);

		switch(command){

		case ESP_GET_CONF:
			/* Send general configuration data */
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
			}
			else{
				readWriteFlash((void *) & gConf, sizeof(gConf_t), gConfData, READ, NULL, NULL);
				osMutexRelease(flashMutexHandle);
				HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&gConf, (uint8_t*)&dummygConf, sizeof(gConf_t));
				osSemaphoreWait (spiEspSemphHandle, osWaitForever);
#if (PRINTF_DEBUG_COM == 1)
				printf("Sent Conf data to ESP\n");
#endif
			}
			break;

		case ESP_GET_AREA:
			/* Send number of wArea elements */
			nElm = gConf.nArea;
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&dummySize, sizeof(uint32_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);

			if ( command == ackMaster){
				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task\n");
				}
				else{
					/* send wArea data */
					for (i=0; i<nElm; i++){
						readWriteFlash((void *) &aConf[i], sizeof(wArea_t), wAreaData, READ, NULL, NULL);
						HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*) &aConf[i],(uint8_t*) &dummywArea, sizeof(wArea_t));
						osSemaphoreWait (spiEspSemphHandle, osWaitForever);
					}
					/* Give the mutex only after all data has been transfered */
					osMutexRelease(flashMutexHandle);
#if (PRINTF_DEBUG == 1)
					printf("Sent Area data to ESP\n");
#endif
				}
			}
			break;

		case ESP_SET_CONF:
			/* Update general configuration data */
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&dummygConf, (uint8_t*)&gConf, sizeof(gConf_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
			}
			else{
				readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
				osMutexRelease(flashMutexHandle);
			}
			break;

		case ESP_SET_AREA:
			/* Update area configuration data */
			if(osMutexWait(flashMutexHandle,200)!= osOK){
				printf("Error or timeout getting Mutex in communication task\n");
			}
			else{
				for (i=0; i<gConf.nArea; i++){
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&dummywArea, (uint8_t*)&aConf[i], sizeof(gConf_t));
					readWriteFlash((void *)&aConf[i], sizeof(wArea_t), wAreaData, WRITE, NULL, NULL);
					osSemaphoreWait (spiEspSemphHandle, osWaitForever);
				}
				/* Give the mutex only after all data has been transfered */
				osMutexRelease(flashMutexHandle);
			}
			break;

		case ESP_GET_DATA_ADC:
			/* Send ADC data */
			/* Send number of elements */
			nElm = getNumElements(mMeasTimeData);
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&dummySize, sizeof(uint32_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);

			if ( command == ackMaster){
				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task\n");
				}
				else{
					/* send wArea data */
					pageNum = FLASH_ADC_LOG_ADDR;
					offset = 0;
					for (i=0; i<nElm; i++){
						readWriteFlash((void *)&moistData, sizeof(mMeasTime_t), mMeasTimeData, READ, &pageNum, &offset);
						HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&moistData, (uint8_t*)&dummyMoist, sizeof(mMeasTime_t));
						osSemaphoreWait (spiEspSemphHandle, osWaitForever);
					}
					/* Give the mutex only after all data has been transfered */
					osMutexRelease(flashMutexHandle);
				}
			}
			break;
		case ESP_GET_DATA_ACT:
			/* Send Actuation data */
			/* Send number of elements */
			nElm = getNumElements(wTimeData);
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&nElm, (uint8_t*)&dummySize, sizeof(uint32_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);

			if ( command == ackMaster){
				if(osMutexWait(flashMutexHandle,200)!= osOK){
					printf("Error or timeout getting Mutex in communication task\n");
				}
				else{
					/* send wArea data */
					pageNum = FLASH_ACT_LOG_ADDR;
					offset = 0;
					for (i=0; i<nElm; i++){
						readWriteFlash((void *)&actuationData, sizeof(wTime_t), wTimeData, READ, &pageNum, &offset);
						HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&actuationData, (uint8_t*)&dummyActuation, sizeof(wTime_t));
						osSemaphoreWait (spiEspSemphHandle, osWaitForever);
					}
					/* Give the mutex only after all data has been transfered */
					osMutexRelease(flashMutexHandle);
				}
			}
			break;

		default:
			break;
		}
		osDelay(10);
	}
}


