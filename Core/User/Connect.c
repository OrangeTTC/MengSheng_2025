#include "Connect.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// USART3 接收缓冲区
uint8_t usart3_rx_data[USART3_RX_BUF_SIZE];


// 包序号计数器
static uint8_t packet_counter = 0;

// 全局变量定义（之前在头文件中错误地定义为static）
uint8_t transmit_data[256];
uint8_t player1[256];
uint8_t player2[256];
uint8_t player_situ = 0;
int len1 = 0;
int len2 = 0;

// 发送模式控制变量
static uint8_t send_mode = 0; // 0:停止, 1:单次发送(循环), 2:轮流发送
static uint32_t last_send_tick = 0;
static uint8_t alt_state = 0; // 0:发送player1, 1:发送player2

void Command_Check(uint8_t* Usart_Rx, uint16_t Size){
    switch(Usart_Rx[0]) {
        case 0xFF:
            if(Usart_Rx[1] == 'U' && Usart_Rx[4] == 'S'){
                // 上升
                RS485_SetSpeed(0x02, -10, 3);
            }
            else if(Usart_Rx[1] == 'D' && Usart_Rx[6] == 'S'){
                // 下降
                RS485_SetSpeed(0x02, 10, 3);
            }
            else if(Usart_Rx[1] == 'L' && Usart_Rx[6] == 'S'){
                // 左转
                RS485_SetSpeed(0x01, -10, 3);
            }
            else if(Usart_Rx[1] == 'R' && Usart_Rx[7] == 'S'){
                // 右转
                RS485_SetSpeed(0x01, 10, 3);
            }
            else{
                RS485_SetSpeed(0x01, 0, 3);
                Delay_ms(10);
                RS485_SetSpeed(0x02, 0, 3);
            }
            break;

        case 0xF1:
            // 单次发送模式：[0xF1][Data...]
            // 提取 player1 数据
            len1 = Size - 1;
            if(len1 > 0 && len1 < 256) {
                memcpy(player1, &Usart_Rx[1], len1);
                send_mode = 1; // 进入模式1
                last_send_tick = HAL_GetTick();
                // 立即发送一次，或者等待Task执行
            }
            break;

        case 0xF2:
            // 轮流发送模式：[0xF2][Data1...][0x01][Data2...]
            {
                int sep_index = -1;
                // 查找分隔符 0x01
                for(int i = 1; i < Size; i++) {
                    if(Usart_Rx[i] == 0x01) {
                        sep_index = i;
                        break;
                    }
                }
                
                if(sep_index != -1) {
                    // 提取 player1 数据
                    len1 = sep_index - 1;
                    if(len1 > 0 && len1 < 256) {
                        memcpy(player1, &Usart_Rx[1], len1);
                    }
                    
                    // 提取 player2 数据
                    len2 = Size - (sep_index + 1);
                    if(len2 > 0 && len2 < 256) {
                        memcpy(player2, &Usart_Rx[sep_index + 1], len2);
                    }
                    
                    send_mode = 2; // 进入模式2
                    alt_state = 0; // 从player1开始
                    last_send_tick = HAL_GetTick();
                }
                
            }
            break;

        case 0x78:
            RS485_DisableMotor(0x01, 0x10);
            Delay_ms(10);  // 两个命令之间延时，避免冲突
            RS485_DisableMotor(0x02, 0x11);
            Delay_ms(10);
            break;

        case 0x91:
            RS485_SetAbsPosition(SERVO_ID_1, 0, 1);
            Delay_ms(10);
            RS485_SetAbsPosition(SERVO_ID_2, 0, 0);
            Delay_ms(10);
            break;

        case 0x93:
            // 保留空逻辑
            break;

        case 0x51: {
            //x P
            int current = Usart_Rx[1];
            pid_x.Kp = - 0.01 * current;
            break;
        }
        case 0x52: {
            //x I
            int current = Usart_Rx[1];
            pid_x.Ki = - 0.01 * current;
            break;
        }
        case 0x53: {
            //x D
            int current = Usart_Rx[1];
            pid_x.Kd = - 0.01 * current;
            break;
        }
        case 0x15: {
            //y P
            int current = Usart_Rx[1];
            pid_y.Kp = 0.01 * current;
            break;
        }
        case 0x25: {
            //y I
            int current = Usart_Rx[1];
            pid_y.Ki = 0.01 * current;
            break;
        }
        case 0x35: {
            //y D
            int current = Usart_Rx[1];
            pid_y.Kd = 0.01 * current;
            break;
        }
        case 0x96:
            Gimbal_Enable(0);
            break;
        
        case 0x69:
            Gimbal_Calibrate();
            break;

        case 0x79:
            Gimbal_Enable(1);
            break;

        default:
            break;
    }
}

/**
 * @brief UART接收完成回调函数（不定长数据接收）
 * @param huart: UART句柄
 * @param Size: 接收到的数据长度
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
        Command_Check(usart3_rx_data, Size);

        // 重新启动接收，使用Ex函数，接收不定长数据
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, usart3_rx_data, sizeof(usart3_rx_data));
        // 关闭DMA传输过半中断（HAL库默认开启，但我们只需要接收完成中断）
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }
    else if (huart->Instance == USART6)
    {
        // 处理接收到的视觉位置误差数据
        ProcessVisionData(usart6_rx_data, Size);

        //HAL_UART_Transmit_IT(&huart3, usart6_rx_data, Size);
        // 重新启动接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, usart6_rx_data, sizeof(usart6_rx_data));
        __HAL_DMA_DISABLE_IT(huart6.hdmarx, DMA_IT_HT);
    }
}

/**
 * @brief 通过激光发射信号
 * @param send_data_len 要发射的数据
 * @param data_len: 发射的数据长度
 */
void Light_Send_Data(uint8_t* send_data_t, int data_len){
		for(int i = 1; i <= data_len; i++){
				transmit_data[i] = send_data_t[i-1];
		}
		transmit_data[0] = 0x1a;
		transmit_data[data_len + 1] = 0x0a;
		HAL_UART_Transmit_DMA(&huart1, transmit_data, data_len + 2);
}

/**
 * @brief 激光发送任务函数，需在主循环中调用
 */
void Light_Send_Task(void) {
    if (send_mode == 0) return;
    
    uint32_t now = HAL_GetTick();
    static uint32_t last_transmit_tick = 0;
    
    // 1000ms 周期切换逻辑
    if (now - last_send_tick >= 1000) {
        last_send_tick = now;
        if (send_mode == 2) {
            alt_state = !alt_state;
        }
    }

    // 50ms 间隔发送逻辑
    if (now - last_transmit_tick >= 50) {
        last_transmit_tick = now;

        if (send_mode == 1) {
            if(len1 > 0) Light_Send_Data(player1, len1);
        }
        else if (send_mode == 2) {
            if (alt_state == 0) {
                if(len1 > 0) Light_Send_Data(player1, len1);
            } else {
                if(len2 > 0) Light_Send_Data(player2, len2);
            }
        }
    }
}

