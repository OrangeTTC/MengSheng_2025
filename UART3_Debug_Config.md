# UART3 串口调试配置说明

## 问题诊断

### 问题：printf没有输出
串口打印不工作的常见原因：
1. ✅ UART3被DMA接收占用（已解决）
2. ❓ 微库(MicroLIB)未启用
3. ❓ 串口波特率配置错误
4. ❓ 硬件连接问题

## 已完成的修改

### 1. 禁用UART3的DMA接收
```c
// main.c - 已注释掉UART3的DMA接收
// HAL_UARTEx_ReceiveToIdle_DMA(&huart3, usart3_rx_data, sizeof(usart3_rx_data));
// __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
```

### 2. 注释Connect.c中的USART3处理
```c
// Connect.c - USART3接收回调已注释
// USART3现在专门用于printf调试输出
```

### 3. 添加printf重定向
```c
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
```

### 4. 添加原始串口测试
```c
// 测试代码：不依赖printf的原始发送
const char *test_msg = "UART3 Test OK\r\n";
HAL_UART_Transmit(&huart3, (uint8_t *)test_msg, strlen(test_msg), 100);
```

## Keil MDK 配置步骤

### 方法1: 启用MicroLIB（推荐）

1. 在Keil MDK中打开项目
2. 点击菜单：**Project → Options for Target**
3. 选择 **Target** 选项卡
4. 勾选 **Use MicroLIB** ✓
5. 点击 **OK**
6. 重新编译项目

```
Project Options
├─ Target
│  ├─ Use MicroLIB  ☑  ← 勾选这个
│  └─ ...
```

**优点**：简单，只需勾选一个选项
**缺点**：功能受限（但对嵌入式足够）

### 方法2: 完整实现（不用MicroLIB）

如果不想用MicroLIB，需要添加更多代码：

```c
// 在main.c的USER CODE BEGIN 4区域添加

#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};

FILE __stdout;

void _sys_exit(int x)
{
    x = x;
}

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}
```

## 测试步骤

### 1. 硬件连接
```
UART3 引脚 (STM32H750):
- TX: PB10 或 PD8 (根据你的配置)
- RX: PB11 或 PD9
- GND: GND

连接到USB转TTL模块:
STM32 TX → USB转TTL RX
STM32 RX → USB转TTL TX  (可选，只打印不需要)
STM32 GND → USB转TTL GND
```

### 2. 串口助手配置
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验**: None
- **流控**: None

### 3. 验证测试

#### 测试1: 原始发送测试
上电后应该首先看到：
```
UART3 Test OK
```
如果看到这个，说明UART3硬件正常。

#### 测试2: printf测试
如果测试1通过，应该接着看到：
```
========================================
  STM32H7 Gimbal Control System
  Version: 1.0
  Date: 2025-11-11
========================================

[Init] System initializing...
```

### 4. 故障排查

| 现象 | 可能原因 | 解决方法 |
|------|----------|----------|
| 完全无输出 | 硬件连接错误 | 检查TX/RX引脚，确认波特率 |
| 乱码 | 波特率不匹配 | 确认UART3初始化波特率为115200 |
| 只有测试OK | MicroLIB未启用 | 按上述方法启用MicroLIB |
| 只有printf输出 | 测试代码未执行 | 检查代码是否正确编译 |

## 检查UART3配置

在CubeMX或main.c中查找UART3初始化代码：

```c
static void MX_USART3_UART_Init(void)
{
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;      // ← 确认波特率
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;  // ← 确认TX模式已启用
  // ...
}
```

## 当前串口分配

| 串口 | 功能 | 波特率 | 方向 |
|------|------|--------|------|
| UART3 | **调试输出(printf)** | 115200 | 仅TX |
| UART4 | RS485电机通信 | 115200 | TX/RX |
| UART6 | 视觉误差数据+PID调参 | 115200 | RX |

## 常见错误和解决

### 错误1: undefined reference to `__use_no_semihosting`
**原因**: 未启用MicroLIB且未添加完整实现
**解决**: 按方法1启用MicroLIB

### 错误2: printf输出到ITM而非UART
**原因**: SWV配置干扰
**解决**: 确保fputc函数正确重定向到UART3

### 错误3: 程序卡死在printf
**原因**: UART3初始化失败或被占用
**解决**: 
1. 检查UART3是否正确初始化
2. 确认UART3 DMA接收已禁用
3. 检查UART时钟是否使能

## 调试技巧

### 1. 使用宏控制日志级别
```c
#define DEBUG_LEVEL 2

#if DEBUG_LEVEL >= 1
  #define LOG_INFO(fmt, ...) printf("[INFO] " fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO(fmt, ...)
#endif

#if DEBUG_LEVEL >= 2
  #define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(fmt, ...)
#endif
```

### 2. 添加时间戳
```c
printf("[%lu] System started\r\n", HAL_GetTick());
```

### 3. 减少日志输出避免影响实时性
```c
// Vision.c中已使用计数器
static uint16_t vision_log_counter = 0;
if (++vision_log_counter >= 10) {
    printf("[UART6] Vision Error -> X:%+5d, Y:%+5d\r\n", error_x, error_y);
    vision_log_counter = 0;
}
```

## 性能影响

### printf性能开销
- 每次调用 ~1-2ms (115200波特率)
- 建议：
  1. 初始化阶段可以频繁使用
  2. 主循环中减少使用
  3. 中断中避免使用printf

### 替代方案
如需高实时性，考虑：
1. 使用DMA发送（缓冲区方式）
2. 使用ITM（需要ST-Link）
3. 只在调试时启用printf

## 验证清单

- [ ] Keil中启用了MicroLIB
- [ ] UART3初始化波特率为115200
- [ ] UART3 DMA接收已禁用
- [ ] Connect.c中USART3回调已注释
- [ ] 硬件连接正确（TX-RX交叉）
- [ ] 串口助手波特率设置为115200
- [ ] 代码已重新编译并下载
- [ ] 上电后能看到"UART3 Test OK"

## 预期输出示例

```
UART3 Test OK
========================================
  STM32H7 Gimbal Control System
  Version: 1.0
  Date: 2025-11-11
========================================

[Init] System initializing...
[Init] Disabling motors...
[Init] Motors disabled
[Init] Initializing gimbal PID control system...
[Init] Gimbal PID initialized (Kp=0.001, Ki=0.001, Kd=0.050)
[Init] System ready. Waiting for button press (PD2) to start...
[Info] Press PD2 button to set origin and enable PID control

[System] Button pressed, starting gimbal system...
[System] Gimbal system started successfully!
[System] Origin set to (0, 0)
[System] PID control enabled

[UART6] Vision Error -> X: +120, Y: +80 | Pos: (150, 100)
[UART6] Vision Error -> X: +100, Y: +60 | Pos: (250, 180)
[UART6] PID Update -> X-axis Kp = 2.000
```
