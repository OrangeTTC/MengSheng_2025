#ifndef __VISION_H__
#define __VISION_H__

#include "main.h"
#include <stdint.h>

extern UART_HandleTypeDef huart6;  // UART6句柄，用于接收视觉位置误差

// ========== UART6 视觉数据协议定义 ==========
// 命令格式: E[X_Sign][XXXX][Y_Sign][YYYY]
// 示例: E1005001200 表示 X误差=+500, Y误差=+1200
//       E0050000300 表示 X误差=-500, Y误差=-300
// 
// 第1位: 'E' - 命令头 (Error)
// 第2位: X符号 ('1'=正, '0'=负)
// 第3-6位: X误差值 (0000-9999)
// 第7位: Y符号 ('1'=正, '0'=负)
// 第8-11位: Y误差值 (0000-9999)

#define VISION_FRAME_HEADER    'E'
#define VISION_FRAME_LENGTH    11       // E + X_Sign + XXXX + Y_Sign + YYYY

// 4. PID参数动态调整协议 (UART6)
// 命令格式: P[Axis][Param][IIFFF]
// - P: 命令头
// - Axis: X/Y 轴选择
// - Param: P/I/D 参数类型
// - IIFFF: 参数值 (2位整数 + 3位小数)
// 示例: PXP02025 表示 X轴Kp=2.025
//       PYD15012 表示 Y轴Kd=15.012
//       PXI00005 表示 X轴Ki=0.005
// 命令格式: P[Axis][Param][IIFFF]
// 示例: PXPII025 表示 X轴Kp=2.025
//       PYDI5012 表示 Y轴Kd=15.012
//       PXPI0005 表示 X轴Ki=0.005
// 
// 第1位: 'P' - 命令头 (Parameter)
// 第2位: 轴选择 ('X'=X轴, 'Y'=Y轴)
// 第3位: 参数类型 ('P'=Kp, 'I'=Ki, 'D'=Kd)
// 第4-5位: 整数部分 (00-99)
// 第6-8位: 小数部分 (000-999)

#define PID_PARAM_HEADER       'P'
#define PID_PARAM_LENGTH       8        // P + Axis + Param + II + FFF

#define SERVO_ID_1            0x01         // 舵机2，控制X轴
#define SERVO_ID_2            0x02         // 舵机3，控制Y轴

// 接收缓冲区
#define USART6_RX_BUF_SIZE     128
extern uint8_t usart6_rx_data[USART6_RX_BUF_SIZE];

// ========== PID控制器结构体 ==========
typedef struct {
    float Kp;           // 比例系数
    float Ki;           // 积分系数
    float Kd;           // 微分系数
    
    float error;        // 当前误差
    float last_error;   // 上次误差
    float integral;     // 误差积分
    float derivative;   // 误差微分
    
    float output;       // PID输出
    
    // 限幅参数
    float integral_max; // 积分限幅
    float output_max;   // 输出限幅
    float output_min;   // 输出下限
} PID_Controller;

// ========== 云台控制结构体 ==========
typedef struct {
    int16_t error_x;    // X轴位置误差（像素）
    int16_t error_y;    // Y轴位置误差（像素）
    
    int32_t position_x; // X轴当前位置（编码器计数）
    int32_t position_y; // Y轴当前位置（编码器计数）
    
    int32_t target_x;   // X轴目标位置
    int32_t target_y;   // Y轴目标位置
    
    uint8_t enable;     // PID使能标志
} GimbalControl;

// 全局云台控制变量
extern GimbalControl gimbal;
extern PID_Controller pid_x;
extern PID_Controller pid_y;
// 校准偏移量（像素），由上位机手动微调时累加，用于在传入PID之前消除人为偏差
extern int16_t calibration_offset_x;
extern int16_t calibration_offset_y;

// ========== 函数声明 ==========

// PID控制器函数
void PID_Init(PID_Controller *pid, float kp, float ki, float kd, float integral_max, float output_max, float output_min);
float PID_Compute(PID_Controller *pid, float error);
void PID_Reset(PID_Controller *pid);

// 云台控制函数
void Gimbal_Init(void);
void Gimbal_SetPIDParams(float kp_x, float ki_x, float kd_x, float kp_y, float ki_y, float kd_y);
void Gimbal_UpdateError(int16_t error_x, int16_t error_y);
void Gimbal_Control(void);
void Gimbal_Enable(uint8_t enable);
void Gimbal_Start(void);  // 启动云台：设置原点并使能PID

// 视觉数据处理函数
void ProcessVisionData(uint8_t *data, uint16_t size);

#endif /* __VISION_H__ */