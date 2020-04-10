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
uint8_t spiready = 1;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId myTask02Handle;
osSemaphoreId myBinarySem01Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask02(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

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
  osSemaphoreDef(myBinarySem01);
  myBinarySem01Handle = osSemaphoreCreate(osSemaphore(myBinarySem01), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of myTask02 */
  osThreadDef(myTask02, StartTask02, osPriorityHigh, 0, 128);
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
  uint8_t log1 []= "Default task runs...    \n";
  uint8_t loopcount = 0;
  for(;;)
  {		/* debug UART */
		sprintf((char*)&log1, "Default task runs:%d", loopcount);
		loopcount += 1;
		HAL_UART_Transmit(&huart1, log1, sizeof(log1)/sizeof(uint8_t),200);
		/* End debug UART */
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
	//osSemaphoreRelease(myBinarySem01Handle);
	//  __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
	//  __HAL_SPI_CLEAR_FREFLAG(&hspi1);

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
	//uint8_t ackMaster = 0xE3;
	//uint8_t printData [] = "Teste   \n";
	//uint8_t dummyData [] = "Teste   \n";
	//uint8_t dataSize = sizeof(printData)/sizeof(uint8_t);
	uint8_t log1 []= "SPI Loop...      \n";
	uint8_t log3 []= "Sent 0x%X to Master        \n";
	uint8_t loopcount = 0;

	osSemaphoreWait(myBinarySem01Handle, portMAX_DELAY);
	/* Infinite loop */
	for(;;){
		/* debug UART */
		sprintf((char*)&log1, "SPI Loop... %d", loopcount);
		HAL_UART_Transmit(&huart1, log1, sizeof(log1)/sizeof(uint8_t),200);
		/* End debug UART */

		//send Ack
		//spiready = 0;
		HAL_SPI_TransmitReceive_IT(&hspi1, &ackSlave, &command, sizeof(uint8_t));

		/* Debug UART */
		sprintf((char*)&log3, "Sent 0x%X to Master", ackSlave);
		HAL_UART_Transmit(&huart1, log3, sizeof(log3)/sizeof(uint8_t),200);
		/* End debug UART */

		// wait for spi transfer complete
		//while(!spiready){
		//	osDelay(10);

		//}
		osSemaphoreWait (myBinarySem01Handle, portMAX_DELAY);

		sprintf((char*)&log3, "Got 0x%X from Master", command);
		HAL_UART_Transmit(&huart1, log3, sizeof(log3)/sizeof(uint8_t),200);
//
//		/* Send data size */
//		//spiready = 0;
//		HAL_SPI_TransmitReceive_IT(&hspi1, &dataSize, &command, sizeof(uint8_t));
//
//		/* Debug UART */
//		sprintf((char*)&log3, "Sent %d to Master", dataSize);
//		HAL_UART_Transmit(&huart1, &dataSize, sizeof(dataSize)/sizeof(uint8_t),200);
//		/* End debug UART */
//
//		// wait for spi transfer complete
//		osSemaphoreAcquire (myBinarySem01Handle, 200);
//		//while(!spiready){
//		//	osDelay(10);
//
//	//	}
//
//		sprintf((char*)&log3, "Got 0x%X from Master ", command);
//		HAL_UART_Transmit(&huart1, log3, sizeof(log3)/sizeof(uint8_t),200);
//
//		/* send data */
//		//spiready = 0;
//		sprintf((char*)&printData, "Teste %d\n", loopcount);
//		HAL_SPI_TransmitReceive_IT(&hspi1, printData, dummyData, sizeof(printData)/sizeof(uint8_t));
//
//		// wait for spi transfer complete
//		osSemaphoreAcquire (myBinarySem01Handle, 200);
//		//while(!spiready){
//		//	osDelay(10);
//		//}

		loopcount += 1;
	}


	osDelay(10);

	/* USER CODE END StartTask02 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
