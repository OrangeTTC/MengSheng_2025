#include "Control.h"
#include <string.h>

/* ========== 软件延时函数（用于中断中）========== */
/**
 * @brief 微秒级软件延时
 * @param us: 延时微秒数
 * @note STM32H7系列主频480MHz，每个循环约4个指令周期
 *       1us ≈ 120个循环 (480MHz / 4)
 */
void Delay_us(uint32_t us)
{
    uint32_t count = us * 120;  // 根据主频调整
    while(count--)
    {
        __NOP();  // 空操作指令
    }
}

/**
 * @brief 毫秒级软件延时
 * @param ms: 延时毫秒数
 */
void Delay_ms(uint32_t ms)
{
    while(ms--)
    {
        Delay_us(1000);
    }
}

/* ========== CRC16 (Modbus) 算法 ========== */
/*
   多项式：0xA001
   初值：  0xFFFF
   计算范围：从协议头（0x3E）到最后一个数据字节
   结果：低字节在前（小端）
*/
uint16_t RS485_CalcCRC16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/* ========== 发送完整协议包 ========== */
HAL_StatusTypeDef RS485_SendPacket(uint8_t packet_no, uint8_t addr, uint8_t cmd, const uint8_t *data, uint8_t len)
{
    uint8_t txbuf[1 + 1 + 1 + 1 + 1 + RS485_MAX_PAYLOAD + 2];
    uint16_t idx = 0;

    // ──────────────── 构建发送帧 ────────────────
    txbuf[idx++] = RS485_HOST_HEADER;  // [0] 协议头，固定 0x3E，表示主机发送
    txbuf[idx++] = packet_no;          // [1] 包序号，用于匹配应答（可递增）
    txbuf[idx++] = addr;               // [2] 设备地址（1~32）
    txbuf[idx++] = cmd;                // [3] 命令码，对应具体控制功能
    txbuf[idx++] = len;                // [4] 数据长度

    // [5...N] 数据字段（可选）
    if (len > 0 && data != NULL) {
        memcpy(&txbuf[idx], data, len);
        idx += len;
    }

    // ──────────────── 计算 CRC16 ────────────────
    uint16_t crc = RS485_CalcCRC16(txbuf, idx);
    txbuf[idx++] = crc & 0xFF;         // [N+1] CRC低字节
    txbuf[idx++] = (crc >> 8) & 0xFF;  // [N+2] CRC高字节

    // ──────────────── 串口发送 ────────────────
    return HAL_UART_Transmit(&huart4, txbuf, idx, RS485_TX_TIMEOUT_MS);
}

/* ========== 常用控制函数 ========== */

// 0x0A 获取电机信息
HAL_StatusTypeDef RS485_GetMotorInfo(uint8_t addr, uint8_t packet_no)
{
    return RS485_SendPacket(packet_no, addr, CMD_GET_INFO, NULL, 0);
}

// 0x0B 读取实时数据（电流、转速、温度等）
HAL_StatusTypeDef RS485_ReadRealtime(uint8_t addr, uint8_t packet_no)
{
    return RS485_SendPacket(packet_no, addr, CMD_READ_REALTIME, NULL, 0);
}

// 0x53 开环控制（参数：int16_t Power）
HAL_StatusTypeDef RS485_SetOpenLoop(uint8_t addr, int16_t power, uint8_t packet_no)
{
    uint8_t data[2];
    data[0] = power & 0xFF;        // 低字节
    data[1] = (power >> 8) & 0xFF; // 高字节
    return RS485_SendPacket(packet_no, addr, CMD_OPEN_LOOP, data, 2);
}

// 0x54 速度闭环控制（单位 0.1RPM）
HAL_StatusTypeDef RS485_SetSpeed(uint8_t addr, int16_t speed_0_1rpm, uint8_t packet_no)
{
    uint8_t data[2];
    data[0] = speed_0_1rpm & 0xFF;
    data[1] = (speed_0_1rpm >> 8) & 0xFF;
    return RS485_SendPacket(packet_no, addr, CMD_SPEED_CONTROL, data, 2);
}

// 0x55 绝对位置闭环（单位：编码器计数）
HAL_StatusTypeDef RS485_SetAbsPosition(uint8_t addr, int32_t pos_counts, uint8_t packet_no)
{
    uint8_t data[4];
    data[0] = pos_counts & 0xFF;
    data[1] = (pos_counts >> 8) & 0xFF;
    data[2] = (pos_counts >> 16) & 0xFF;
    data[3] = (pos_counts >> 24) & 0xFF;
    return RS485_SendPacket(packet_no, addr, CMD_ABS_POSITION, data, 4);
}

// 0x56 相对位置闭环
HAL_StatusTypeDef RS485_SetRelPosition(uint8_t addr, int16_t rel_counts, uint8_t packet_no)
{
    uint8_t data[2];
    data[0] = rel_counts & 0xFF;
    data[1] = (rel_counts >> 8) & 0xFF;
    return RS485_SendPacket(packet_no, addr, CMD_REL_POSITION, data, 2);
}

//设置原点
// 0x21 设置电机当前为原点
HAL_StatusTypeDef RS485_SetOrigin(uint8_t addr, uint8_t packet_no)
{
    // 此命令无数据字段，因此 data=NULL, len=0
    return RS485_SendPacket(packet_no, addr, CMD_SET_ORIGIN, NULL, 0);
}

// 0x50 关闭电机（Disable Motor）
HAL_StatusTypeDef RS485_DisableMotor(uint8_t addr, uint8_t packet_no)
{
    // 此命令无数据字段
    return RS485_SendPacket(packet_no, addr, CMD_DISABLE_MOTOR, NULL, 0);
}

