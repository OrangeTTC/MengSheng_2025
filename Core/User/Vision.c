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
    PID_Init(&pid_x, -0.5f, 0.00f, -0.01f, 5000.0f, 9999.0f, -9999.0f);
    
    // 初始化Y轴PID控制器
    PID_Init(&pid_y, 0.5f, 0.00f, 0.01f, 5000.0f, 9999.0f, -9999.0f);
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
    
    // 发送位置命令到舵机
    uint8_t sent_data[100];
    sprintf((char *)sent_data, "Gimbal Control: Target X=%ld, Target Y=%ld\r\n", 
            gimbal.target_x, gimbal.target_y);
    HAL_UART_Transmit_DMA(&huart3, sent_data, sizeof(sent_data));
    Delay_ms(5);
    // 舵机2控制X轴，舵机3控制Y轴
    RS485_SetAbsPosition(SERVO_ID_1, gimbal.target_x, vision_packet_counter++);
    Delay_ms(5);  // 两个命令之间延时5ms，避免冲突
    RS485_SetAbsPosition(SERVO_ID_2, gimbal.target_y, vision_packet_counter++);
    
    // 更新当前位置
    gimbal.position_x = gimbal.target_x;
    gimbal.position_y = gimbal.target_y;
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

    // 4. 测试移动到初始位置
    RS485_SetAbsPosition(SERVO_ID_1, 100, packet_no++);
    Delay_ms(10);
    RS485_SetAbsPosition(SERVO_ID_2, 100, packet_no++);
    Delay_ms(10);

    Delay_ms(500);

    RS485_SetAbsPosition(SERVO_ID_1, 0, packet_no++);
    Delay_ms(10);
    RS485_SetAbsPosition(SERVO_ID_2, 0, packet_no++);
    Delay_ms(10);

    uint8_t start[] = "Gimbal Set Finish\r\n";
    HAL_UART_Transmit_DMA(&huart3, start, sizeof(start));
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
        
        // 更新云台误差
        Gimbal_UpdateError(error_x, error_y);
        
        // 执行PID控制（如果已使能）
        Gimbal_Control();
    }
}

/* ========== PID参数动态调整 ========== */

/**
 * @brief 处理PID参数调整命令
 * @param data: 接收到的数据缓冲区
 * @param size: 数据长度
 * @note 命令格式: P[Axis][Param][IIFFF]
 *       示例: PXP02025 - X轴Kp=2.025
 *             PYD15012 - Y轴Kd=15.012
 *             PXI00005 - X轴Ki=0.005
 */
void ProcessPIDParamCommand(uint8_t *data, uint16_t size)
{
    // 检查最小数据长度
    if (size < PID_PARAM_LENGTH) {
        return;
    }
    
    // 搜索命令帧头 'P'
    for (uint16_t i = 0; i <= size - PID_PARAM_LENGTH; i++) {
        // 检查帧头
        if (data[i] != PID_PARAM_HEADER) {
            continue;
        }
        
        // 提取各个字段
        char axis_char = data[i + 1];       // 第2位: 轴选择 (X或Y)
        char param_char = data[i + 2];      // 第3位: 参数类型 (P/I/D)
        char int_str[3];                    // 第4-5位: 整数部分
        char frac_str[4];                   // 第6-8位: 小数部分
        
        // 复制整数部分
        int_str[0] = data[i + 3];
        int_str[1] = data[i + 4];
        int_str[2] = '\0';
        
        // 复制小数部分
        frac_str[0] = data[i + 5];
        frac_str[1] = data[i + 6];
        frac_str[2] = data[i + 7];
        frac_str[3] = '\0';
        
        // 验证轴选择 (必须是 'X' 或 'Y')
        if (axis_char != 'X' && axis_char != 'Y') {
            continue;  // 无效的轴选择
        }
        
        // 验证参数类型 (必须是 'P', 'I', 或 'D')
        if (param_char != 'P' && param_char != 'I' && param_char != 'D') {
            continue;  // 无效的参数类型
        }
        
        // 验证整数部分 (必须全部是数字)
        uint8_t valid = 1;
        for (int j = 0; j < 2; j++) {
            if (int_str[j] < '0' || int_str[j] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) continue;
        
        // 验证小数部分 (必须全部是数字)
        for (int j = 0; j < 3; j++) {
            if (frac_str[j] < '0' || frac_str[j] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) continue;
        
        // 解析整数和小数部分
        int int_part = atoi(int_str);      // 整数部分 0-99
        int frac_part = atoi(frac_str);    // 小数部分 0-999
        
        // 组合成浮点数: 整数部分 + 小数部分/1000
        float param_value = (float)int_part + (float)frac_part / 1000.0f;
        
        // 选择要更新的PID控制器
        PID_Controller *pid = (axis_char == 'X') ? &pid_x : &pid_y;
        
        // 根据参数类型更新相应的PID参数
        switch (param_char) {
            case 'P':
                pid->Kp = param_value;
                break;
                
            case 'I':
                pid->Ki = param_value;
                break;
                
            case 'D':
                pid->Kd = param_value;
                break;
        }
        
        // 复位PID控制器状态（清除积分项等）
        PID_Reset(pid);
    }
}

