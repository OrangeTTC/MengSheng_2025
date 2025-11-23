#include "Vision.h"
#include "Control.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

// USART6 接收缓冲区
uint8_t usart6_rx_data[USART6_RX_BUF_SIZE];

// 包序号计数器
static uint8_t vision_packet_counter = 0;

// 云台控制全局变量
GimbalControl gimbal = {0};
PID_Controller pid_x = {0};
PID_Controller pid_y = {0};
// 校准偏移量（由手动瞄准产生的像素级误差，用于在传入PID前消除）
int16_t calibration_offset_x = 0;
int16_t calibration_offset_y = 0;

/* ========== PID控制器实现 ========== */

/**
 * @brief 初始化PID控制器
 * @param pid: PID控制器结构体指针
 * @param kp: 比例系数
 * @param ki: 积分系数
 * @param kd: 微分系数
 * @param integral_max: 积分限幅值
 * @param output_max: 输出上限
 * @param output_min: 输出下限
 */
void PID_Init(PID_Controller *pid, float kp, float ki, float kd, 
              float integral_max, float output_max, float output_min)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
    
    pid->integral_max = integral_max;
    pid->output_max = output_max;
    pid->output_min = output_min;
}

/**
 * @brief PID计算函数
 * @param pid: PID控制器结构体指针
 * @param error: 当前误差
 * @return PID输出值
 */
float PID_Compute(PID_Controller *pid, float error)
{
    // 保存当前误差
    pid->error = error;
    
    // 计算积分项
    pid->integral += error;
    
    // 积分限幅（抗积分饱和）
    if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    } else if (pid->integral < -pid->integral_max) {
        pid->integral = -pid->integral_max;
    }
    
    // 计算微分项
    pid->derivative = error - pid->last_error;
    
    // 计算PID输出
    pid->output = pid->Kp * pid->error + 
                  pid->Ki * pid->integral + 
                  pid->Kd * pid->derivative;
    
    // 输出限幅
    if (pid->output > pid->output_max) {
        pid->output = pid->output_max;
    } else if (pid->output < pid->output_min) {
        pid->output = pid->output_min;
    }
    
    // 保存误差用于下次计算微分
    pid->last_error = error;
    
    return pid->output;
}

/**
 * @brief 复位PID控制器
 * @param pid: PID控制器结构体指针
 */
void PID_Reset(PID_Controller *pid)
{
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
}

/* ========== 云台控制实现 ========== */

/**
 * @brief 初始化云台控制系统
 */
void Gimbal_Init(void)
{
    // 初始化云台参数
    gimbal.error_x = 0;
    gimbal.error_y = 0;
    gimbal.position_x = 0;
    gimbal.position_y = 0;
    gimbal.target_x = 0;
    gimbal.target_y = 0;
    gimbal.enable = 0;
    
    // 初始化X轴PID控制器
    // 参数: Kp=0.001, Ki=0.001, Kd=0.05, 积分限幅=5000, 输出范围=-9999~9999
    PID_Init(&pid_x, -1.0f, 0.00f, -0.01f, 5000.0f, 9999.0f, -9999.0f);
    
    // 初始化Y轴PID控制器
    PID_Init(&pid_y, 1.0f, 0.00f, 0.02f, 5000.0f, 9999.0f, -9999.0f);
}

/**
 * @brief 设置云台PID参数
 * @param kp_x: X轴比例系数
 * @param ki_x: X轴积分系数
 * @param kd_x: X轴微分系数
 * @param kp_y: Y轴比例系数
 * @param ki_y: Y轴积分系数
 * @param kd_y: Y轴微分系数
 */
void Gimbal_SetPIDParams(float kp_x, float ki_x, float kd_x, 
                         float kp_y, float ki_y, float kd_y)
{
    pid_x.Kp = kp_x;
    pid_x.Ki = ki_x;
    pid_x.Kd = kd_x;
    
    pid_y.Kp = kp_y;
    pid_y.Ki = ki_y;
    pid_y.Kd = kd_y;
}

/**
 * @brief 更新位置误差
 * @param error_x: X轴位置误差（像素）
 * @param error_y: Y轴位置误差（像素）
 */
void Gimbal_UpdateError(int16_t error_x, int16_t error_y)
{
    gimbal.error_x = error_x;
    gimbal.error_y = error_y;
}

/**
 * @brief 云台PID控制函数
 * @note 根据位置误差计算并发送舵机控制命令
 */
void Gimbal_Control(void)
{
    if (!gimbal.enable) {
        return;  // PID未使能，不执行控制
    }
    
    // 计算X轴PID输出
    float pid_output_x = PID_Compute(&pid_x, (float)gimbal.error_x);
    
    // 计算Y轴PID输出
    float pid_output_y = PID_Compute(&pid_y, (float)gimbal.error_y);
    
    // 计算目标位置（当前位置 + PID输出）
    gimbal.target_x = gimbal.position_x + (int32_t)pid_output_x;
    gimbal.target_y = gimbal.position_y + (int32_t)pid_output_y;
    // 舵机2控制X轴，舵机3控制Y轴
    RS485_SetAbsPosition(SERVO_ID_1, gimbal.target_x, vision_packet_counter++);
    Delay_ms(5);  // 两个命令之间延时5ms，避免冲突
    RS485_SetAbsPosition(SERVO_ID_2, gimbal.target_y, vision_packet_counter++);
    
    // 更新当前位置
    gimbal.position_x = gimbal.target_x;
    gimbal.position_y = gimbal.target_y;
}

/**
 * @brief 校准云台中心偏移
 * @note 将当前位置设为新的中心点（Error=0）
 */
void Gimbal_Calibrate(void)
{
    calibration_offset_x += gimbal.error_x;
    calibration_offset_y += gimbal.error_y;
    
    // 立即清零当前误差
    gimbal.error_x = 0;
    gimbal.error_y = 0;
}

/**
 * @brief 使能/禁用云台PID控制
 * @param enable: 1=使能, 0=禁用
 */
void Gimbal_Enable(uint8_t enable)
{
    gimbal.enable = enable;
    
    if (!enable) {
        // 禁用时复位PID控制器
        PID_Reset(&pid_x);
        PID_Reset(&pid_y);
    }
}

/**
 * @brief 启动云台系统
 * @note 按下按键后调用：设置当前位置为原点，使能PID控制
 */
void Gimbal_Start(void)
{
    static uint8_t packet_no = 0;
    
    // 1. 设置两个电机的当前位置为原点（0位置）
    RS485_SetOrigin(SERVO_ID_1, packet_no++);
    Delay_ms(10);  // 软件延时，避免命令冲突
    RS485_SetOrigin(SERVO_ID_2, packet_no++);
    Delay_ms(10);
    
    // 2. 初始化云台位置为0
    gimbal.position_x = 0;
    gimbal.position_y = 0;
    gimbal.target_x = 0;
    gimbal.target_y = 0;
    
    // 3. 复位PID控制器
    PID_Reset(&pid_x);
    PID_Reset(&pid_y);

    RS485_SetAbsPosition(SERVO_ID_1, 0, packet_no++);
    Delay_ms(50);
    RS485_SetAbsPosition(SERVO_ID_2, 0, packet_no++);
    Delay_ms(50);
    
    // 4. 使能PID控制
    gimbal.enable = 1;

}

/* ========== 视觉数据处理 ========== */

/**
 * @brief 处理视觉位置误差数据
 * @param data: 接收到的数据缓冲区
 * @param size: 数据长度
 * @note 命令格式: E[X_Sign][XXXX][Y_Sign][YYYY]
 *       示例: E1005001200 - X误差=+500, Y误差=+1200
 *             E0050000300 - X误差=-500, Y误差=-300
 */
void ProcessVisionData(uint8_t *data, uint16_t size)
{
    // 检查最小数据长度
    if (size < VISION_FRAME_LENGTH) {
        return;
    }
    
    // 搜索命令帧头 'E'
    for (uint16_t i = 0; i <= size - VISION_FRAME_LENGTH; i++) {
        // 检查帧头
        if (data[i] != VISION_FRAME_HEADER) {
            continue;
        }
        
        // 提取各个字段
        char x_sign_char = data[i + 1];     // 第2位: X符号
        char x_str[5];                      // 第3-6位: X误差值
        char y_sign_char = data[i + 6];     // 第7位: Y符号
        char y_str[5];                      // 第8-11位: Y误差值
        
        // 复制X误差字符串
        x_str[0] = data[i + 2];
        x_str[1] = data[i + 3];
        x_str[2] = data[i + 4];
        x_str[3] = data[i + 5];
        x_str[4] = '\0';
        
        // 复制Y误差字符串
        y_str[0] = data[i + 7];
        y_str[1] = data[i + 8];
        y_str[2] = data[i + 9];
        y_str[3] = data[i + 10];
        y_str[4] = '\0';
        
        // 验证X符号 (必须是 '0' 或 '1')
        if (x_sign_char != '0' && x_sign_char != '1') {
            continue;
        }
        
        // 验证Y符号 (必须是 '0' 或 '1')
        if (y_sign_char != '0' && y_sign_char != '1') {
            continue;
        }
        
        // 验证X误差字符串 (必须全部是数字)
        uint8_t valid = 1;
        for (int j = 0; j < 4; j++) {
            if (x_str[j] < '0' || x_str[j] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) continue;
        
        // 验证Y误差字符串 (必须全部是数字)
        for (int j = 0; j < 4; j++) {
            if (y_str[j] < '0' || y_str[j] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) continue;
        
        // 解析X误差值
        int16_t error_x = (int16_t)atoi(x_str);
        if (x_sign_char == '0') {
            error_x = -error_x;  // 负数
        }
        
        // 解析Y误差值
        int16_t error_y = (int16_t)atoi(y_str);
        if (y_sign_char == '0') {
            error_y = -error_y;  // 负数
        }
        
        // 在传给PID之前，消除手动校准产生的偏移误差
        error_x -= calibration_offset_x;
        error_y -= calibration_offset_y;

        // 更新云台误差
        Gimbal_UpdateError(error_x, error_y);
        
        // 执行PID控制（如果已使能）
        Gimbal_Control();
    }
}
