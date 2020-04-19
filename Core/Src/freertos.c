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
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osMessageQId spiEspQueueHandle;
osSemaphoreId spiEspSemphHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
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
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
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
  /* add semaphores, ... */
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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 300);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityIdle, 0, 300);
  myTask02Handle = osThreadCreate(osThread(myTask02), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */

  uint8_t i =0;
  for(i=0;i<N_AREA;i++){
	  W25qxx_ReadSector((uint8_t*)&waterArea,FLASH_READINGS_ADDR,i*sizeof(waterArea),sizeof(waterArea));
	  printf("Found waterArea %d in flash\r\n", waterArea.pumpID);
	  osDelay(1000);
  }

  for(;;)
  {
	 // sprintf((char*)&buffer, "log2:Sent 0x%X to Master  Got 0x%X from Master\n", ackSlave, command);
	 // HAL_UART_Transmit(&huart1, &buffer[0], sizeof(init_message),800);

    osDelay(5000);
  }
  /* USER CODE END StartDefaultTask */
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
	uint8_t ackSlave = 0xCE;
	uint8_t command = 0x00;
	uint8_t ackMaster = 0xE3;
	uint8_t c_List = 0xBA;
	uint8_t printData [] = "Teste 0000 \n";
	uint8_t dummyData [] = "Teste 0000 \n";
	uint8_t dataSize = sizeof(printData)/sizeof(uint8_t);
	uint8_t log1 []= "SPI Loop...      \n";
	uint8_t log2 []= "logx:Sent 0x00 to Master  Got 0x00 from Master                       \n";
	uint8_t loopcount = 0;
	//osEvent message;
	//osSemaphoreWait(myBinarySem01Handle, portMAX_DELAY);
	/* Infinite loop */
	for(;;){
		/* debug UART */

		memset(log1, 0x00, sizeof log1);
		sprintf((char*)&log1, "SPI Loop... %d\n", loopcount);
		HAL_UART_Transmit(&huart1, log1, sizeof(log1)/sizeof(uint8_t),200);
		/* End debug UART */

		/* Send slave Ack */
		HAL_SPI_TransmitReceive_DMA(&hspi2, &ackSlave, &command, sizeof(uint8_t));
		osSemaphoreWait (spiEspSemphHandle, portMAX_DELAY);
		//message = osMessageGet(spiEspQueueHandle, command, 200);
		/* Debug UART */
		memset(log2, 0x00, sizeof log2);
		sprintf((char*)&log2, "log2:Sent 0x%X to Master  Got 0x%X from Master\n", ackSlave, command);
		HAL_UART_Transmit(&huart1, log2, sizeof(log2)/sizeof(uint8_t),200);
		/* End debug UART */

		if ( command == c_List){
			/* Send data size */
			HAL_SPI_TransmitReceive_DMA(&hspi2, &dataSize, &command, sizeof(uint8_t));
			//message = osMessageGet(spiEspQueueHandle, command, 200);
			osSemaphoreWait (spiEspSemphHandle, portMAX_DELAY);
			/* Debug UART */
			memset(log2, 0x00, sizeof log2);
			sprintf((char*)&log2, "log3:Sent 0x%X to Master  Got 0x%X from Master\n", dataSize, command);
			HAL_UART_Transmit(&huart1, log2, sizeof(log2)/sizeof(uint8_t),200);
			/* End debug UART */

			if ( command == ackMaster){
				/* send data */
				memset(printData, 0x00, sizeof printData);
				sprintf((char*)&printData, "Teste %d\n", loopcount);
				HAL_SPI_TransmitReceive_DMA(&hspi2, printData, dummyData, sizeof(printData)/sizeof(uint8_t));
				osSemaphoreWait (spiEspSemphHandle, portMAX_DELAY);
				/* Debug UART */
				memset(log2, 0x00, sizeof log2);
				sprintf((char*)&log2, "log4:Sent %s", (char*)&printData);
				HAL_UART_Transmit(&huart1, log2, sizeof(log2)/sizeof(uint8_t),200);
				/* End debug UART */
			}
		}

		loopcount += 1;
		osDelay(10);
	}
  /* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
