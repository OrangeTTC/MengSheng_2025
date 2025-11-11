# 云台PID控制系统文档

## 概述

本系统实现了基于视觉反馈的云台PID闭环控制，通过UART6接收视觉系统发送的位置误差，经过PID计算后控制两个舵机（X轴和Y轴）进行位置调整。

## 系统架构

```
视觉系统 (UART6) → STM32 (PID控制器) → RS485 (UART4) → 舵机
```

- **UART6**: 接收视觉位置误差数据
- **PID控制器**: 计算舵机目标位置
- **UART4 (RS485)**: 发送位置命令到舵机

## UART6 通信协议

### 帧格式（11个ASCII字符）

```
E[X_Sign][XXXX][Y_Sign][YYYY]
```

### 字段说明

| 位置 | 字段 | 说明 | 取值范围 |
|------|------|------|----------|
| 1 | E | 命令头（固定） | 'E' (Error) |
| 2 | X_Sign | X轴误差符号 | '1'=正, '0'=负 |
| 3-6 | XXXX | X轴误差值（4位） | 0000-9999 |
| 7 | Y_Sign | Y轴误差符号 | '1'=正, '0'=负 |
| 8-11 | YYYY | Y轴误差值（4位） | 0000-9999 |

### 命令示例

| 命令 | 说明 |
|------|------|
| `E1005001200` | X误差=+500像素, Y误差=+1200像素 |
| `E0050000300` | X误差=-500像素, Y误差=-300像素 |
| `E1000010000` | X误差=0, Y误差=0（目标居中） |
| `E1010001500` | X误差=+100像素, Y误差=+1500像素 |
| `E0030000800` | X误差=-300像素, Y误差=-800像素 |

### 坐标系定义

- **X轴正方向**: 目标在图像右侧（舵机需向右转）
- **X轴负方向**: 目标在图像左侧（舵机需向左转）
- **Y轴正方向**: 目标在图像上方（舵机需向上转）
- **Y轴负方向**: 目标在图像下方（舵机需向下转）

## PID控制器

### 控制算法

标准PID控制算法：

```
Output = Kp × Error + Ki × ∫Error·dt + Kd × dError/dt
```

- **Kp**: 比例系数，响应速度
- **Ki**: 积分系数，消除稳态误差
- **Kd**: 微分系数，减少超调

### 默认PID参数

```c
// X轴PID参数
Kp_x = 1.0
Ki_x = 0.01
Kd_x = 0.5

// Y轴PID参数
Kp_y = 1.0
Ki_y = 0.01
Kd_y = 0.5

// 限幅参数
积分限幅 = 5000
输出范围 = -9999 ~ +9999
```

### 参数调整指南

1. **比例系数 (Kp)**
   - 增大Kp：响应更快，但可能震荡
   - 减小Kp：响应更慢，但更稳定
   - 建议范围：0.5 ~ 2.0

2. **积分系数 (Ki)**
   - 增大Ki：更快消除稳态误差，但可能超调
   - 减小Ki：更稳定，但收敛慢
   - 建议范围：0.001 ~ 0.1

3. **微分系数 (Kd)**
   - 增大Kd：减少超调，但对噪声敏感
   - 减小Kd：平滑但可能震荡
   - 建议范围：0.1 ~ 1.0

## 代码结构

### 主要文件

1. **Vision.h/c** - PID控制器和视觉数据处理
2. **Connect.h/c** - UART中断处理
3. **Control.h/c** - RS485舵机通信
4. **main.c** - 系统初始化

### 关键函数

#### PID控制器函数

```c
// 初始化PID控制器
void PID_Init(PID_Controller *pid, float kp, float ki, float kd, 
              float integral_max, float output_max, float output_min);

// PID计算
float PID_Compute(PID_Controller *pid, float error);

// 复位PID
void PID_Reset(PID_Controller *pid);
```

#### 云台控制函数

```c
// 初始化云台系统
void Gimbal_Init(void);

// 设置PID参数
void Gimbal_SetPIDParams(float kp_x, float ki_x, float kd_x, 
                         float kp_y, float ki_y, float kd_y);

// 更新位置误差
void Gimbal_UpdateError(int16_t error_x, int16_t error_y);

// 执行PID控制
void Gimbal_Control(void);

// 使能/禁用控制
void Gimbal_Enable(uint8_t enable);
```

## 使用方法

### 1. 硬件连接

- UART6 连接视觉系统
- UART4 (RS485) 连接舵机
- 舵机2 (地址2) 控制X轴
- 舵机3 (地址3) 控制Y轴

### 2. 系统初始化

系统启动时自动执行以下步骤：

```c
// 1. 初始化串口DMA接收
HAL_UARTEx_ReceiveToIdle_DMA(&huart6, usart6_rx_data, sizeof(usart6_rx_data));

// 2. 初始化云台PID控制
Gimbal_Init();

// 3. 使能PID控制
Gimbal_Enable(1);
```

### 3. 运行流程

```
1. 视觉系统通过UART6发送位置误差
   ↓
2. UART6中断接收数据并解析
   ↓
3. 调用ProcessVisionData()处理数据
   ↓
4. 更新云台误差Gimbal_UpdateError()
   ↓
5. 执行PID控制Gimbal_Control()
   ↓
6. 计算目标位置并发送到舵机
```

### 4. 调整PID参数

在程序中修改PID参数：

```c
// 方法1: 修改Gimbal_Init()中的默认参数
void Gimbal_Init(void)
{
    PID_Init(&pid_x, 1.5f, 0.02f, 0.6f, 5000.0f, 9999.0f, -9999.0f);
    PID_Init(&pid_y, 1.5f, 0.02f, 0.6f, 5000.0f, 9999.0f, -9999.0f);
}

// 方法2: 运行时动态调整
Gimbal_SetPIDParams(1.5f, 0.02f, 0.6f,  // X轴参数
                    1.5f, 0.02f, 0.6f); // Y轴参数
```

### 5. 临时禁用PID控制

```c
// 禁用PID控制（例如手动控制时）
Gimbal_Enable(0);

// 重新启用
Gimbal_Enable(1);
```

## Python测试脚本

### 基本测试

```python
import serial
import time

def send_vision_error(ser, error_x, error_y):
    """
    发送视觉位置误差
    :param error_x: X轴误差 (-9999 到 9999)
    :param error_y: Y轴误差 (-9999 到 9999)
    """
    # 确定符号和绝对值
    x_sign = '1' if error_x >= 0 else '0'
    y_sign = '1' if error_y >= 0 else '0'
    x_abs = abs(error_x)
    y_abs = abs(error_y)
    
    # 构建命令: E[X_Sign][XXXX][Y_Sign][YYYY]
    command = f"E{x_sign}{x_abs:04d}{y_sign}{y_abs:04d}"
    
    # 发送
    ser.write(command.encode('ascii'))
    print(f"发送: {command} (X={error_x:+d}, Y={error_y:+d})")

# 打开串口
ser = serial.Serial('COM6', 115200, timeout=1)

# 测试1: 目标在右上方
send_vision_error(ser, 500, 1200)
time.sleep(1)

# 测试2: 目标在左下方
send_vision_error(ser, -300, -800)
time.sleep(1)

# 测试3: 目标居中
send_vision_error(ser, 0, 0)

ser.close()
```

### 模拟连续跟踪

```python
import serial
import time
import math

ser = serial.Serial('COM6', 115200, timeout=1)

# 模拟圆形运动轨迹
for angle in range(0, 360, 5):
    rad = math.radians(angle)
    error_x = int(500 * math.cos(rad))
    error_y = int(500 * math.sin(rad))
    
    send_vision_error(ser, error_x, error_y)
    time.sleep(0.05)  # 50ms更新一次

ser.close()
```

## 常用命令快速参考

### 测试命令

```
E1000010000  - 目标居中（误差为0）
E1005000000  - 目标在右侧500像素
E0005000000  - 目标在左侧500像素
E1000010500  - 目标在上方500像素
E1000000500  - 目标在下方500像素
E1050001000  - 目标在右上方（X=500, Y=1000）
E0050000800  - 目标在左下方（X=-500, Y=-800）
```

## 串口配置

### UART6 (视觉接收)
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **发送模式**: ASCII文本

## 故障排查

### 问题1: 云台不响应

**检查项**:
- UART6是否正确连接
- 波特率是否为115200
- PID是否已使能 (Gimbal_Enable(1))
- 命令格式是否正确

**解决方法**:
```c
// 在main.c中检查
Gimbal_Enable(1);  // 确保PID已使能
```

### 问题2: 云台震荡

**原因**: PID参数不合适

**解决方法**:
1. 减小Kp（降低响应速度）
2. 增大Kd（增加阻尼）
3. 减小Ki（降低积分作用）

```c
// 尝试更保守的参数
Gimbal_SetPIDParams(0.5f, 0.005f, 0.8f,
                    0.5f, 0.005f, 0.8f);
```

### 问题3: 响应太慢

**原因**: Kp太小

**解决方法**:
```c
// 增大Kp
Gimbal_SetPIDParams(2.0f, 0.01f, 0.5f,
                    2.0f, 0.01f, 0.5f);
```

### 问题4: 有稳态误差

**原因**: Ki太小

**解决方法**:
```c
// 增大Ki
Gimbal_SetPIDParams(1.0f, 0.05f, 0.5f,
                    1.0f, 0.05f, 0.5f);
```

### 问题5: 命令无效

**检查**:
- 命令是否为11个字符
- 是否以'E'开头
- 符号位是否为'0'或'1'
- 误差值是否为4位数字

## 性能优化建议

### 1. 调整控制周期

如果视觉系统发送频率较高（>50Hz），可以在接收到数据后立即执行控制，无需定时器。

### 2. 死区设置

避免在误差很小时频繁调整：

```c
void Gimbal_Control(void)
{
    if (!gimbal.enable) return;
    
    // 添加死区（误差小于10像素不调整）
    if (abs(gimbal.error_x) < 10 && abs(gimbal.error_y) < 10) {
        return;
    }
    
    // 正常PID计算...
}
```

### 3. 滤波处理

对噪声较大的误差信号进行滤波：

```c
// 简单的一阶低通滤波
static float filtered_error_x = 0;
static float filtered_error_y = 0;
float alpha = 0.7;  // 滤波系数

filtered_error_x = alpha * gimbal.error_x + (1 - alpha) * filtered_error_x;
filtered_error_y = alpha * gimbal.error_y + (1 - alpha) * filtered_error_y;
```

## 系统状态监控

### 查看PID状态

可以通过串口输出PID状态（调试时）：

```c
void Gimbal_PrintStatus(void)
{
    char msg[100];
    sprintf(msg, "X: err=%d, out=%.2f, Y: err=%d, out=%.2f\r\n",
            gimbal.error_x, pid_x.output,
            gimbal.error_y, pid_y.output);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
}
```

## 注意事项

1. **位置单位**: 误差单位为像素，需要根据实际情况调整PID参数
2. **机械限位**: 确保PID输出不会使舵机超出机械限位
3. **电源稳定**: PID控制可能导致频繁调整，需要稳定的电源
4. **通信延迟**: 考虑视觉处理和通信延迟，可能需要预测性控制
5. **初始位置**: 启动时舵机应在中间位置，避免超出范围

## 扩展功能

### 1. 多目标切换

```c
typedef enum {
    TARGET_1,
    TARGET_2,
    NO_TARGET
} TargetState;

TargetState current_target = NO_TARGET;
```

### 2. 自动/手动模式切换

```c
typedef enum {
    MODE_AUTO,   // 自动PID控制
    MODE_MANUAL  // 手动控制
} ControlMode;

ControlMode control_mode = MODE_AUTO;
```

### 3. 数据记录

记录PID运行数据用于分析：

```c
typedef struct {
    uint32_t timestamp;
    int16_t error_x;
    int16_t error_y;
    float output_x;
    float output_y;
} ControlLog;
```

## 版本历史

### v1.0 (2025-11-10)
- ✓ 实现基本PID控制功能
- ✓ UART6视觉数据接收
- ✓ 双轴独立PID控制
- ✓ 参数可调整
- ✓ 完整的错误处理

## 参考资料

- PID控制理论
- STM32 HAL库文档
- RS485舵机协议
