/*
 * measurement.c
 *
 *  Created on: Apr 24, 2020
 *      Author: johny
 */

#include "main.h"
#include "measurement.h"

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
	osSemaphoreRelease (adcSemphHandle);
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc){
#if (PRINTF_DEBUG == 1)
	printf("Adc error %d.\n", (int)hadc->ErrorCode);
#endif
}

/**
 * @brief Function implementing the adcConvTask thread.
 * @param argument: Not used
 * @retval None
 */

void adcConvTask(void const * argument)
{
	/* USER CODE BEGIN StartTask02 */
	uint16_t adcData [N_ADC];
	uint8_t i;
	uint8_t unlocked = 0;
	mMeasTime_t data;
	uint16_t pPageNum;
	uint16_t pOffset;
	/*Must take semaphore the first time... */
	osSemaphoreWait (adcSemphHandle, osWaitForever);
	lastAdcConv.time = 0;
	for(;;)
	{
		/*Set output to give supply to sensors */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adcData, sizeof(adcData)/sizeof(uint16_t));
		osSemaphoreWait (adcSemphHandle, osWaitForever);
		/* Reset supply to sensors */
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_ADC_Stop_DMA(&hadc1);

		//#if (PRINTF_DEBUG == 1)
		//for(i=0; i<N_ADC; i++){
		//  printf("Adc %d data: %d\n", i, (uint16_t) (( (uint32_t)data[i] * 3300) / 4096));
		//  }
		//#endif
		//to do: get time from RTC
		lastAdcConv.time += 1;
		lastAdcConv.temperature = 25;
		//fill in conversion values in data structure and store it in external flash
		//save up only until the number of used sensors
		for(i=0; i<gConf.nSens; i++){
			lastAdcConv.meas[i] = adcData[i];
		}
		osMutexWait(flashMutexHandle,osWaitForever);
		/* Save the measurements to flash*/
		readWriteFlash((void*)&lastAdcConv, sizeof(lastAdcConv), mMeasTimeData, WRITE, &gConf.pageAdc, &gConf.pageOffsetAdc);
		/* Update the current page number and offset in the flash configuration structure */
		readWriteFlash((void *) &gConf, sizeof(gConf_t), gConfData, WRITE, NULL, NULL);
		osMutexRelease(flashMutexHandle);
//#if (PRINTF_DEBUG == 1)
//		if(gConf.pageAdc>= 258){
//			unlocked = 1;
//		}
//		pPageNum = FLASH_ADC_LOG_ADDR;
//		pOffset = 0;
//		if(unlocked){
//			do{
//				readWriteFlash((void*)&data, sizeof(data), mMeasTimeData, READ, &pPageNum, &pOffset);
//				printf("Time %lu temperature %lu adc_data %lu\n", data.time, data.temperature, data.meas[1]);
//				osDelay(100);
//			}while(pPageNum < gConf.pageAdc || pOffset < gConf.pageOffsetAdc);
//			unlocked = 0;
//			osDelay(100);
//		}
//#endif
		osDelay(gConf.adcConvTimeInterval);
	}
}	/* USER CODE END adcConvTask */
