#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "main.h"
#include <stdint.h>

extern UART_HandleTypeDef huart3;  // UART3句柄，接收舵机控制命令
extern UART_HandleTypeDef huart4;  // UART4句柄，RS485通信
extern UART_HandleTypeDef huart6;  // UART6句柄，接收视觉位置误差

// ========== 协议定义 ==========
#define RS485_HOST_HEADER      0x3E  // 主机发送协议头
#define RS485_SLAVE_HEADER     0x3C  // 从机应答协议头

#define RS485_MAX_PAYLOAD      60    // 最大数据区长度
#define RS485_TX_TIMEOUT_MS    100   // 串口发送超时

// 常用命令（来自协议文档）
#define CMD_GET_INFO          0x0A
#define CMD_READ_REALTIME     0x0B
#define CMD_READ_PARAMS       0x0C
#define CMD_WRITE_PARAMS      0x0D
#define CMD_SAVE_PARAMS       0x0E
#define CMD_FACTORY_RESET     0x0F
#define CMD_ENCODER_CALIB     0x20
#define CMD_SET_ORIGIN        0x21
#define CMD_READ_ENCODER      0x2F
#define CMD_READ_STATUS       0x40
#define CMD_CLEAR_FAULT       0x41
#define CMD_DISABLE_MOTOR     0x50
#define CMD_GO_HOME_ABS       0x51
#define CMD_GO_HOME_CLOSEST   0x52
#define CMD_OPEN_LOOP         0x53
#define CMD_SPEED_CONTROL     0x54
#define CMD_ABS_POSITION      0x55
#define CMD_REL_POSITION      0x56
#define CMD_POS_TARGET_SPEED  0x57

static uint8_t rx_data[256];

// ========== 函数声明 ==========
uint16_t RS485_CalcCRC16(const uint8_t *buf, uint16_t len);
HAL_StatusTypeDef RS485_SendPacket(uint8_t packet_no, uint8_t addr, uint8_t cmd, const uint8_t *data, uint8_t len);

// 命令封装函数
HAL_StatusTypeDef RS485_GetMotorInfo(uint8_t addr, uint8_t packet_no);
HAL_StatusTypeDef RS485_ReadRealtime(uint8_t addr, uint8_t packet_no);
HAL_StatusTypeDef RS485_ReadEncoder(uint8_t addr, uint8_t packet_no);
HAL_StatusTypeDef RS485_SetOpenLoop(uint8_t addr, int16_t power, uint8_t packet_no);
HAL_StatusTypeDef RS485_SetSpeed(uint8_t addr, int16_t speed_0_1rpm, uint8_t packet_no);
HAL_StatusTypeDef RS485_SetAbsPosition(uint8_t addr, int32_t pos_counts, uint8_t packet_no);
HAL_StatusTypeDef RS485_SetRelPosition(uint8_t addr, int16_t rel_counts, uint8_t packet_no);
HAL_StatusTypeDef RS485_SetOrigin(uint8_t addr, uint8_t packet_no);
HAL_StatusTypeDef RS485_DisableMotor(uint8_t addr, uint8_t packet_no);

// 软件延时函数（用于中断中）
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

#endif /* __CONTROL_H__ */