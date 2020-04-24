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
	uint32_t time = 1;
	uint8_t offset = 0;
	//  uint8_t unlocked = 0;
	/*Must take semaphore the first time... */
	osSemaphoreWait (adcSemphHandle, osWaitForever);

	for(;;)
	{
		//set output to give supply to sensors
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adcData, sizeof(adcData)/sizeof(uint16_t));
		osSemaphoreWait (adcSemphHandle, osWaitForever);
		//set reset supply to sensors
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_ADC_Stop_DMA(&hadc1);

		//#if (PRINTF_DEBUG == 1)
		//for(i=0; i<N_ADC; i++){
		//  printf("Adc %d data: %d\n", i, (uint16_t) (( (uint32_t)data[i] * 3300) / 4096));
		//  }
		//#endif
		//to do: get time from RTC
		lastAdcConv.time = time;

		//fill in conversion values in data structure and store it in external flash
		//save up only until the number of used sensors
		for(i=0; i<monitorConf.nSens; i++){
			//id not really necessary!?
			lastAdcConv.meas[i].id=i;
			lastAdcConv.meas[i].reading = adcData[i];
		}

		//to do:delete after adding RTC time
		time += 1;

		assert_param(sizeof(lastAdcConv) <= w25qxx.PageSize);
		W25qxx_WritePage((uint8_t*)&lastAdcConv, monitorConf.lastFlashPageNum, offset, sizeof(lastAdcConv));

		if(offset + sizeof(mMeasTime_t) >= w25qxx.PageSize){
			monitorConf.lastFlashPageNum += 1;
			// check if reached end of memory, if so start writing from FLASH_READINGS_ADDR
			if(monitorConf.lastFlashPageNum >= w25qxx.PageCount){
				monitorConf.lastFlashPageNum = FLASH_READINGS_ADDR;
				//nlocked = 1;
			}
			offset = 0;
		}
		else{
			offset += sizeof(lastAdcConv);
		}

		osDelay((uint32_t)monitorConf.adcConvTimeInterval);

		//	#if (PRINTF_DEBUG == 1)
		//		  if(unlocked){
		//			  // test read
		//			  monitorConf.lastFlashPageNum = FLASH_READINGS_ADDR;
		//			  offset = 0;
		//			  do{
		//				  W25qxx_ReadPage((uint8_t*)&lastAdcConv, monitorConf.lastFlashPageNum, offset, sizeof(lastAdcConv));
		//				  printf("Page to READ: %d, Offset: %X,Time %lu:\n", monitorConf.lastFlashPageNum, offset,lastAdcConv.time);
		//				  osDelay(100);
		//				  if(offset + sizeof(mMeasTime_t) >= w25qxx.PageSize){
		//					  monitorConf.lastFlashPageNum += 1;
		//					  offset = 0;
		//				  }
		//				  else
		//				  {
		//					  offset += sizeof(lastAdcConv);
		//				  }
		//			  }while(monitorConf.lastFlashPageNum < lastMemAddr);
		//			  monitorConf.lastFlashPageNum = FLASH_READINGS_ADDR;
		//			  unlocked = 0;
		//		  }
		//		  osDelay(1);
		//	#endif
	}
	/* USER CODE END adcConvTask */
}
