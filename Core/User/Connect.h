#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "main.h"
#include "control.h"
#include "Vision.h"
#include <stdint.h>

extern uint8_t transmit_data[256];
extern uint8_t player1[256];
extern uint8_t player2[256];
extern uint8_t player_situ;
extern int len1;
extern int len2;

extern UART_HandleTypeDef huart3;  // CubeMX 生成的 UART3 句柄，用于接收控制命令
extern UART_HandleTypeDef huart4;  // CubeMX 生成的 UART4 句柄，用于RS485通信
extern UART_HandleTypeDef huart1;  // CubeMX 生成的 UART4 句柄，用于RS485通信

// ========== 串口命令定义 ==========
// 命令格式: M[ID][±][AAAA]
// 示例: M2100500  表示电机2，移动到+500位置
// 示例: M3001000  表示电机3，移动到-1000位置
// 
// 第1位: 'M' - 命令头
// 第2位: 电机地址 ('2' 或 '3')
// 第3位: 符号 ('1'=正, '0'=负)
// 第4-7位: 位置值 (0000-9999)

#define CMD_FRAME_HEADER    'M'
#define CMD_FRAME_LENGTH    7        // M + ID + Sign + 4位数

// // 舵机ID定义
// #define SERVO_ID_1          2
// #define SERVO_ID_2          3

// 接收缓冲区
#define USART3_RX_BUF_SIZE  128
extern uint8_t usart3_rx_data[USART3_RX_BUF_SIZE];
static uint8_t usart1_rx_data[USART3_RX_BUF_SIZE];

// ========== 函数声明 ==========
void Command_Check(uint8_t* Usart_Rx, uint16_t Size);
void Light_Send_Data(uint8_t* send_data_t, int data_len);
void Light_Send_Task(void); // 新增任务函数声明

#endif /* __CONNECT_H__ */