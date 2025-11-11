#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "main.h"
#include "control.h"
#include <stdint.h>

extern UART_HandleTypeDef huart3;  // CubeMX 生成的 UART3 句柄，用于接收控制命令
extern UART_HandleTypeDef huart4;  // CubeMX 生成的 UART4 句柄，用于RS485通信

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
#define CMD_FRAME_LENGTH    7        // M + ID + Sign + 4位数字

// // 舵机ID定义
// #define SERVO_ID_1          2
// #define SERVO_ID_2          3

// 接收缓冲区
#define USART3_RX_BUF_SIZE  128
extern uint8_t usart3_rx_data[USART3_RX_BUF_SIZE];

// ========== 函数声明 ==========
void ProcessServoCommand(uint8_t *data, uint16_t size);

#endif /* __CONNECT_H__ */