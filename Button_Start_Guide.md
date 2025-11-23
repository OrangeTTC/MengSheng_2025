# 按键启动云台系统说明

## 功能概述

按下连接到**PD2引脚**的按钮后，云台系统将：
1. 设置两个电机当前位置为**原点(0位置)**
2. 初始化云台位置为0
3. 复位PID控制器
4. **启动PID控制**，开始自动追踪

## 硬件连接

### 按键连接
```
按钮 ──┬── PD2 (STM32H750)
       └── GND
```

- **PD2**: 已配置为下降沿触发中断，内部上拉
- **按键类型**: 常开按钮（按下时连接GND）
- **无需外部上拉电阻**（内部已配置上拉）

## 工作流程

### 1. 系统上电
```c
// main.c 初始化部分
RS485_DisableMotor(0x01, 0x10);  // 关闭电机1
RS485_DisableMotor(0x02, 0x11);  // 关闭电机2
Gimbal_Init();                   // 初始化云台PID系统
// 注意：此时PID未启用，等待按键触发
```

### 2. 手动调整云台到合适位置
- 此时电机处于**关闭状态**，可以手动调整
- 将云台调整到合适的初始位置
- 准备好后，按下PD2按钮

### 3. 按下PD2按钮触发启动
```c
// GPIO中断回调
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_2)  // PD2按键
  {
    HAL_Delay(50);  // 消抖50ms
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2) == GPIO_PIN_RESET)
    {
      Gimbal_Start();  // 启动云台
    }
  }
}
```

### 4. Gimbal_Start() 执行流程
```c
void Gimbal_Start(void)
{
  // 步骤1: 设置电机原点
  RS485_SetOrigin(SERVO_ID_1, packet_no++);  // X轴电机
  HAL_Delay(10);
  RS485_SetOrigin(SERVO_ID_2, packet_no++);  // Y轴电机
  HAL_Delay(10);
  
  // 步骤2: 初始化云台位置
  gimbal.position_x = 0;
  gimbal.position_y = 0;
  gimbal.target_x = 0;
  gimbal.target_y = 0;
  
  // 步骤3: 复位PID控制器
  PID_Reset(&pid_x);
  PID_Reset(&pid_y);
  
  // 步骤4: 使能PID控制
  gimbal.enable = 1;
}
```

### 5. PID控制开始工作
- OpenMV发送误差数据 `E[X_Sign][XXXX][Y_Sign][YYYY]`
- STM32接收误差，PID计算输出
- 发送位置命令到电机
- 云台自动追踪红外目标

## 代码修改说明

### 修改的文件

#### 1. Vision.h
```c
// 新增函数声明
void Gimbal_Start(void);  // 启动云台：设置原点并使能PID
```

#### 2. Vision.c
```c
// 新增函数实现
void Gimbal_Start(void)
{
    // 设置原点 + 初始化位置 + 复位PID + 使能控制
}
```

#### 3. main.c
```c
// 修改初始化流程（不自动启动）
Gimbal_Init();
// Gimbal_Enable(0);  // 默认禁用，等待按键启动

// 新增GPIO中断回调
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_2)  // PD2按键
    {
        Gimbal_Start();
    }
}
```

## 使用步骤

### 准备阶段
1. ✅ 上电，系统初始化
2. ✅ 电机处于关闭状态
3. ✅ 手动调整云台到初始位置
4. ✅ 确保OpenMV已连接并运行
5. ✅ 确认红外光源准备好

### 启动阶段
1. **按下PD2按钮**
2. 系统自动设置当前位置为原点
3. PID控制启动
4. 云台开始自动追踪

### 运行阶段
- 云台自动追踪红外光源
- 可通过UART6动态调整PID参数
- 误差数据持续接收和处理

## 调试技巧

### 验证按键功能
```c
// 在HAL_GPIO_EXTI_Callback中添加调试代码
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_2)
  {
    HAL_Delay(50);
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2) == GPIO_PIN_RESET)
    {
      // 调试：点亮LED或发送串口信息
      // HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
      
      Gimbal_Start();
    }
  }
}
```

### 检查PID是否启用
```c
// 在主循环中检查
if (gimbal.enable) {
    // PID已启用，正常运行
} else {
    // PID未启用，等待按键
}
```

### 监控启动流程
- 检查电机是否响应原点设置命令
- 验证PID控制器是否开始输出
- 观察云台是否开始移动

## 注意事项

1. **消抖处理**: 代码中已加入50ms消抖延时
2. **中断优先级**: EXTI2优先级为0（最高）
3. **阻塞延时**: `HAL_Delay()`在中断中使用，保证命令执行完成
4. **重复按下**: 可以多次按下按钮重新设置原点
5. **安全性**: 按下按钮前确保云台位置安全

## 引脚配置总结

| 引脚 | 功能 | 模式 | 上拉/下拉 |
|------|------|------|-----------|
| PD2 | 启动按键 | 外部中断(下降沿) | 上拉 |
| PD3-7 | 预留按键 | 外部中断(下降沿) | 上拉 |

## 扩展功能

### 添加停止按钮（例如PD3）
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_2)  // 启动
  {
    HAL_Delay(50);
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2) == GPIO_PIN_RESET)
    {
      Gimbal_Start();
    }
  }
  else if (GPIO_Pin == GPIO_PIN_3)  // 停止
  {
    HAL_Delay(50);
    if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3) == GPIO_PIN_RESET)
    {
      Gimbal_Enable(0);  // 禁用PID
      RS485_DisableMotor(SERVO_ID_1, 0x01);
      RS485_DisableMotor(SERVO_ID_2, 0x02);
    }
  }
}
```

### 添加LED指示
```c
void Gimbal_Start(void)
{
    // ... 原有代码 ...
    
    // 启动成功，点亮LED
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}
```

## 故障排查

| 问题 | 可能原因 | 解决方法 |
|------|----------|----------|
| 按键无响应 | 中断未启用 | 检查NVIC配置 |
| 电机不动 | 原点设置失败 | 检查RS485通信 |
| PID不工作 | 未使能 | 检查`gimbal.enable` |
| 误触发 | 抖动 | 增加消抖时间 |
| 重复启动 | 长按按钮 | 添加标志位防止重复 |

## 系统状态流程图

```
上电
  ↓
系统初始化
  ↓
电机关闭状态
  ↓
等待按键... ←─────────┐
  ↓ (按下PD2)          │
消抖检测              │
  ↓                   │
设置原点(位置=0)       │
  ↓                   │
初始化云台位置         │
  ↓                   │
复位PID控制器         │
  ↓                   │
使能PID控制           │
  ↓                   │
开始自动追踪          │
  ↓                   │
运行中... ────────────┘
  (可再次按PD2重新初始化)
```
