/*
 * communication.c
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */

#include "main.h"
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
	uint8_t ackMaster = 0xE3;
	uint8_t nElm = 0;

	wArea_t dummywArea; //dummy for spi transaction
	gConf_t dummygConf; //dummy for spi transaction
	uint16_t i=0;

	/* need to take the semaphor the first time....*/
	osSemaphoreWait (spiEspSemphHandle, osWaitForever);

	for(;;){
#if (PRINTF_DEBUG == 1)
		printf("Spi loop..\n");
#endif
		/* Send slave Ack */
		HAL_SPI_TransmitReceive_DMA(&hspi2, &ackSlave, &command, sizeof(uint8_t));
		osSemaphoreWait (spiEspSemphHandle, osWaitForever);

		switch(command){

		case ESP_GET_CONF:
			/* Send general configuration data */
			W25qxx_ReadPage((uint8_t*)&gConf, FLASH_CONFIG_ADDR, 0, sizeof(gConf)/sizeof(uint8_t));
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&gConf, (uint8_t*)&dummygConf, sizeof(gConf)/sizeof(uint8_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);
#if (PRINTF_DEBUG == 1)
			printf("Sent Conf data to ESP\n");
#endif
			break;

		case ESP_GET_AREA:
			/* Send number of wArea elements */
			nElm = gConf.nArea;
			HAL_SPI_TransmitReceive_DMA(&hspi2, &nElm, &command, sizeof(uint8_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);

			if ( command == ackMaster){
				/* send wArea data */
				for (i=0; i<nElm; i++){
					W25qxx_ReadSector((uint8_t*)&aConf, FLASH_AREA_ADDR, i*sizeof(aConf), sizeof(aConf)/sizeof(uint8_t));
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*) &aConf,(uint8_t*) &dummywArea, sizeof(aConf)/sizeof(uint8_t));
					osSemaphoreWait (spiEspSemphHandle, osWaitForever);
				}
#if (PRINTF_DEBUG == 1)
				printf("Sent Area data to ESP\n");
#endif
			}
			break;
		default:
			break;
		}
		osDelay(10);
	}
}
