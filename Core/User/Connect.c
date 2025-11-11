#include "Connect.h"
#include "Vision.h"
#include <string.h>
#include <stdlib.h>

// USART3 接收缓冲区
uint8_t usart3_rx_data[USART3_RX_BUF_SIZE];

// 包序号计数器
static uint8_t packet_counter = 0;

/**
 * @brief 处理舵机控制命令
 * @param data: 接收到的数据缓冲区
 * @param size: 数据长度
 * @note 命令格式: M[ID][±][AAAA]
 *       示例: M2100500 - 电机2移动到+500
 *             M3001000 - 电机3移动到-1000
 */
void ProcessServoCommand(uint8_t *data, uint16_t size)
{
    // 检查最小数据长度
    if (size < CMD_FRAME_LENGTH) {
        return;
    }

    // 搜索命令帧头 'M'
    for (uint16_t i = 0; i <= size - CMD_FRAME_LENGTH; i++) {
        // 检查帧头
        if (data[i] != CMD_FRAME_HEADER) {
            continue;
        }

        // 提取各个字段
        char motor_id_char = data[i + 1];      // 第2位: 电机地址
        char sign_char = data[i + 2];          // 第3位: 符号
        char pos_str[5];                        // 第4-7位: 位置值
        
        // 复制位置字符串
        pos_str[0] = data[i + 3];
        pos_str[1] = data[i + 4];
        pos_str[2] = data[i + 5];
        pos_str[3] = data[i + 6];
        pos_str[4] = '\0';

        // 验证电机ID (必须是 '2' 或 '3')
        if (motor_id_char != '2' && motor_id_char != '3') {
            continue;  // 无效的电机ID
        }

        // 验证符号 (必须是 '0' 或 '1')
        if (sign_char != '0' && sign_char != '1') {
            continue;  // 无效的符号
        }

        // 验证位置字符串 (必须全部是数字)
        uint8_t valid = 1;
        for (int j = 0; j < 4; j++) {
            if (pos_str[j] < '0' || pos_str[j] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) {
            continue;  // 位置包含非数字字符
        }

        // 解析电机地址
        uint8_t motor_id = motor_id_char - '0';  // '2' -> 2, '3' -> 3

        // 解析位置值
        int32_t position = atoi(pos_str);  // 转换为整数 0-9999

        // 根据符号调整位置
        if (sign_char == '0') {
            position = -position;  // 负数
        }
        // sign_char == '1' 时保持正数

        // 发送绝对位置命令到舵机
        RS485_SetAbsPosition(motor_id, position, packet_counter++);
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
        // 处理接收到的舵机控制命令
        ProcessServoCommand(usart3_rx_data, Size);

        if(usart3_rx_data[0] != 'S')
        {
            if(usart3_rx_data[1] == 'E'){
                RS485_SetOrigin(0x01, 0x01);
                Delay_ms(10);  // 两个命令之间延时
                RS485_SetOrigin(0x02, 0x02);
            }
            else if(usart3_rx_data[1] == 'T'){
                RS485_DisableMotor(0x01, 0x10);
                Delay_ms(10);  // 两个命令之间延时
                RS485_DisableMotor(0x02, 0x11);
            }
        }
        
        // 处理PID参数调整命令
        ProcessPIDParamCommand(usart3_rx_data, Size);

        // 重新启动接收，使用Ex函数，接收不定长数据
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, usart3_rx_data, sizeof(usart3_rx_data));
        // 关闭DMA传输过半中断（HAL库默认开启，但我们只需要接收完成中断）
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }
    else if (huart->Instance == USART6)
    {
        // 处理接收到的视觉位置误差数据
        ProcessVisionData(usart6_rx_data, Size);

        HAL_UART_Transmit_IT(&huart3, usart6_rx_data, Size);
        
        // 重新启动接收
        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, usart6_rx_data, sizeof(usart6_rx_data));
        __HAL_DMA_DISABLE_IT(huart6.hdmarx, DMA_IT_HT);
    }
}