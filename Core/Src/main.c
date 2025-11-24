/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body（大号字体直接显示接收数据+灯控）
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"  // OLED驱动头文件（确保已包含正确的函数声明）
#include "usart.h"    // 仅保留串口1相关
#include <string.h>   // 字符串操作
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_BUF_SIZE 32  // 串口1数据缓冲区大小
#define FRAME_HEAD 0x1A   // 串口1接收帧头
#define FRAME_TAIL 0x0A   // 串口1接收帧尾
#define OLED_PAGE_COUNT 8 // OLED总页数（128x64屏幕默认8页，每页8像素）
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
uint8_t uart1_rx_byte;                 // 串口1单次接收1字节（原始数据）
uint8_t uart1_data_buf[UART_BUF_SIZE];// 串口1原始数据缓冲区
uint8_t frame_state = 0;               // 串口1帧状态：0=空闲，1=接收中
uint8_t data_len = 0;                  // 串口1接收数据长度
uint8_t candidate_buf[UART_BUF_SIZE];  // 候选数据缓冲，用于稳定性检测
uint8_t same_count = 0;                // 连续相同计数
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 声明全屏清屏函数（用已有函数实现，避免未定义错误）
void oled_full_clear(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  全屏清屏（用 ssd1306_clear_page 循环实现，兼容现有驱动）
  */
void oled_full_clear(void)
{
  for(uint8_t i = 0; i < OLED_PAGE_COUNT; i++)
  {
    ssd1306_clear_page(i, 0); // 逐页清屏，覆盖整个OLED
  }
}

/**
  * @brief  串口1中断回调函数（大号字体直接显示接收数据+灯控）
  * @param  huart: 串口句柄
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    switch(frame_state)
    {
      case 0: // 等待帧头
        if (uart1_rx_byte == FRAME_HEAD)
        {
          frame_state = 1;
          data_len = 0;
          memset(uart1_data_buf, 0, UART_BUF_SIZE); // 清空缓冲区
          
          // 接收到帧头：指示灯立即亮起
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
        }
        break;
      
      case 1: // 接收原始数据，等待帧尾
        if (uart1_rx_byte == FRAME_TAIL)
        {
          frame_state = 0;
          uart1_data_buf[data_len] = '\0'; // 字符串结束符
          
          // 接收到帧尾：指示灯熄灭
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
          
          // 稳定性检测：连续3次收到相同数据才显示
          if (strcmp((const char *)uart1_data_buf, (const char *)candidate_buf) == 0)
          {
            same_count++;
          }
          else
          {
            strcpy((char *)candidate_buf, (const char *)uart1_data_buf);
            same_count = 1;
          }

          if (same_count == 3)
          {
            // 核心：全屏清屏（无残留）+ 大号字体显示数据
            oled_full_clear(); 
            ssd1306_draw_string_12_24(0, 8, (const char *)candidate_buf, 1, 1); // 大号字体居中
            ssd1306_refresh();
            ssd1306_refresh();
          }
        }
        else if (data_len < UART_BUF_SIZE - 1)
        {
          uart1_data_buf[data_len++] = uart1_rx_byte; // 存储原始数据
        }
        else
        {
          frame_state = 0; // 溢出重置
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET); // 异常时灯灭
        }
        break;
    }
    
    HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1); // 串口1继续接收
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();  // 仅保留串口1初始化
  /* USER CODE BEGIN 2 */
  // 上电后A2指示灯闪烁两下（启动反馈）
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

  // OLED初始化（清屏，无任何初始显示）
  ssd1306_init();
  oled_full_clear(); // 用自定义全屏清屏，避免未定义函数
  ssd1306_refresh();

  // 仅开启串口1中断接收
  HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief  System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
    // 错误时指示灯快速闪烁（方便排查）
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    HAL_Delay(100);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
