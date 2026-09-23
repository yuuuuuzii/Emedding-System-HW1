/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
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
/* Definitions for defaultTask */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

/* Private variables ---------------------------------------------------------*/
osThreadId_t Periodical_Task_Handle;
const osThreadAttr_t Periodical_Task_attributes = {
  .name = "Periodical_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t Press_Task_Handle;
const osThreadAttr_t Press_Task_attributes = {
  .name = "Press_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh7,
};

osThreadId_t IRQ_Task_Handle;
const osThreadAttr_t IRQ_Task_attributes = {
  .name = "IRQ_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh7,
};

osTimerId_t Periodic_timerHandle;
const osTimerAttr_t Periodic_timer_attributes = {
  .name = "Periodic_timer"
};

osSemaphoreId_t IRQ_Sema;
const osSemaphoreAttr_t IRQSema_attributes = {
  .name = "IRQ_Sema"
};

osSemaphoreId_t Periodical_Sema;
const osSemaphoreAttr_t PeriodicalSema_attributes = {
  .name = "Periodical_Sema"
};

osMessageQueueId_t Queue_press;
const osMessageQueueAttr_t Queue_press_attributes = {
  .name = "Queue_press"
};

osSemaphoreId_t Critical_section_Sema;
const osSemaphoreAttr_t Critical_section_Sema_attributes = {
  .name = "Critical_section_Sema"
};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void Periodical_Task(void *argument);
void Press_task(void *argument);
void periodic_led(void *argument);
void Timer_GPIO_PIN_13_Callback(void *argument);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* Init scheduler */
  osKernelInitialize();

  /* Create the semaphores(s) */
  Periodical_Sema = osSemaphoreNew(1, 1, &PeriodicalSema_attributes);
  Queue_press = osMessageQueueNew(1, sizeof(uint32_t), &Queue_press_attributes);
  Critical_section_Sema = osSemaphoreNew(1, 1, &Critical_section_Sema_attributes);
  IRQ_Sema = osSemaphoreNew(1, 0, &IRQSema_attributes);

  /* Create the timer(s) */
  Periodic_timerHandle = osTimerNew(periodic_led, osTimerPeriodic, NULL, &Periodic_timer_attributes);
  osTimerStart(Periodic_timerHandle, 10000U);

  /* Create the thread(s) */
  Periodical_Task_Handle = osThreadNew(Periodical_Task, NULL, &Periodical_Task_attributes);
  Press_Task_Handle = osThreadNew(Press_task, NULL, &Press_Task_attributes);
  IRQ_Task_Handle = osThreadNew(Debounce_task, NULL, &IRQ_Task_attributes);

  /* Start scheduler */
  osKernelStart();

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = User_button_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(User_button_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case GPIO_PIN_13:
    	osSemaphoreRelease(IRQ_Sema);
      break;
    default:
      break;
  }
}

void Periodical_Task(void *argument)
{
  for(;;)
  {
    osSemaphoreAcquire(Periodical_Sema, osWaitForever);
    osSemaphoreAcquire(Critical_section_Sema, osWaitForever);
    for(int i = 0; i < 40; ++i)
    {
      HAL_GPIO_TogglePin(GPIOB, LED2_Pin);
      osDelay(50);
    }
    HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
    osSemaphoreRelease(Critical_section_Sema);
  }
}

void Debounce_task(void *argument)
{
	static int btnPressTime = 0;
	static int stable_level = 1; // 0 for falling edge, 1 for rising edge

	for(;;){
		osSemaphoreAcquire(IRQ_Sema, osWaitForever);
		osDelay(30U);

		if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13))
		{ // falling edge
			if(stable_level)
			{
			  btnPressTime = HAL_GetTick();
			  stable_level = 0;
			}
		}
		else
		{ // rising edge
			if(stable_level == 0){
				stable_level = 1;
				int elapsed_time = HAL_GetTick() - btnPressTime;

				uint32_t press_state = 0;

				if(elapsed_time >= 1000U)
				{
					  press_state = 1;
				}
				else
				{
					  press_state = 0;
				}

				osMessageQueuePut(Queue_press, &press_state, 0U, 0U);
			}
		}
	}
}

void Press_task(void *argument)
{
  int msg;
  for(;;)
  {
    osMessageQueueGet(Queue_press, &msg, NULL, osWaitForever);
    osSemaphoreAcquire(Critical_section_Sema, osWaitForever);
    if(msg)
    {
      for(int i = 0; i < 100; ++i)
      {
        HAL_GPIO_TogglePin(GPIOB, LED2_Pin);
        osDelay(50);
      }
    }
    else
    {
      for(int i = 0; i < 10; ++i)
      {
        HAL_GPIO_TogglePin(GPIOB, LED2_Pin);
        osDelay(500);
      }
    }

    HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
    osSemaphoreRelease(Critical_section_Sema);
  }
}

void periodic_led(void *argument)
{
  osSemaphoreRelease(Periodical_Sema);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
