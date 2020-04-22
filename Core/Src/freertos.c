/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


/* USER CODE END Variables */
osThreadId spiEspComTaskHandle;
osThreadId myTask02Handle;
osMessageQId spiEspQueueHandle;
osSemaphoreId spiEspSemphTXHandle;
osSemaphoreId spiEspSemphHandle;
osSemaphoreId adcSemphHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void spiEspComTask(void const * argument);
void StartTask02(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
	printf("WARNING: Stack overflow!\nTask name: %s\nTask handle: %lu\n", pcTaskName, (uint32_t) xTask);
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
	printf("WARNING: vApplicationMallocFailedHook got called!\n");
}
/* USER CODE END 5 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of myBinarySem01 */
  osSemaphoreDef(spiEspSemph);
  spiEspSemphHandle = osSemaphoreCreate(osSemaphore(spiEspSemph), 1);
  /* USER CODE BEGIN RTOS_SEMAPHORES */
  osSemaphoreDef(adcSemph);
  adcSemphHandle = osSemaphoreCreate(osSemaphore(adcSemph), 1);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of SpiQueueEsp */
  osMessageQDef(SpiEspQueue, 256, uint8_t);
  spiEspQueueHandle = osMessageCreate(osMessageQ(SpiEspQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */

  osThreadDef(spiEspComT, spiEspComTask, osPriorityNormal, 0, 500);
  spiEspComTaskHandle = osThreadCreate(osThread(spiEspComT), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityNormal, 0, 500);
  myTask02Handle = osThreadCreate(osThread(myTask02), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}
//void DMATransferComplete(DMA_HandleTypeDef *hdma){
//	printf("txrx complete\n");
//	osSemaphoreRelease (spiEspSemphHandle);
//
//}
//void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
//
//}


/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
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
			W25qxx_ReadPage((uint8_t*)&monitorConf, FLASH_CONFIG_ADDR, 0, sizeof(monitorConf)/sizeof(uint8_t));
			HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&monitorConf, (uint8_t*)&dummygConf, sizeof(monitorConf)/sizeof(uint8_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);
#if (PRINTF_DEBUG == 1)
				printf("Sent Conf data to ESP\n");
#endif
			break;

		case ESP_GET_AREA:
			/* Send number of wArea elements */
			nElm = monitorConf.nArea;
			HAL_SPI_TransmitReceive_DMA(&hspi2, &nElm, &command, sizeof(uint8_t));
			osSemaphoreWait (spiEspSemphHandle, osWaitForever);

			if ( command == ackMaster){
				/* send wArea data */
				for (i=0; i<nElm; i++){
					W25qxx_ReadSector((uint8_t*)&waterArea, FLASH_AREA_ADDR, i*sizeof(waterArea), sizeof(waterArea)/sizeof(uint8_t));
					HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*) &waterArea,(uint8_t*) &dummywArea, sizeof(waterArea)/sizeof(uint8_t));
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

//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc){
//#if (PRINTF_DEBUG == 1)
//	printf("Adc conversion half complete callback.\n");
//#endif
//}
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
#if (PRINTF_DEBUG == 1)
	printf("Adc conversion complete callback.\n");
#endif

	osSemaphoreRelease (adcSemphHandle);
}
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc){
#if (PRINTF_DEBUG == 1)
	printf("Adc error %d.\n", (int)hadc->ErrorCode);
#endif
}
/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void const * argument)
{
  /* USER CODE BEGIN StartTask02 */
  uint16_t data [15];
  HAL_StatusTypeDef result;
  uint8_t i;
  osSemaphoreWait (adcSemphHandle, osWaitForever);
	  for(;;)
	  {
		  result = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&data, sizeof(data)/sizeof(uint16_t));
		  osSemaphoreWait (adcSemphHandle, osWaitForever);
		  HAL_ADC_Stop_DMA(&hadc1);
#if (PRINTF_DEBUG == 1)
		  //printf("Adc conversion finished.\n Result: %d \n", result);
		  for(i=0; i<sizeof(data)/sizeof(uint16_t); i++){
			  printf("Adc %d data: %d\n", i, (uint16_t) (( (uint32_t)data[i] * 3300) / 4096));
		  }
#endif

		  osDelay(1000);
	  }
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
