# PID参数动态调整协议文档

## 概述

通过UART6实时调整云台PID控制器的参数，无需重新编译和下载程序。

## 命令格式

### 帧结构（8个ASCII字符）

```
P[Axis][Param][II][FFF]
```

### 字段说明

| 位置 | 字段 | 说明 | 取值范围 |
|------|------|------|----------|
| 1 | P | 命令头（固定） | 'P' (Parameter) |
| 2 | Axis | 轴选择 | 'X' = X轴, 'Y' = Y轴 |
| 3 | Param | 参数类型 | 'P' = Kp, 'I' = Ki, 'D' = Kd |
| 4-5 | II | 整数部分（2位） | 00-99 |
| 6-8 | FFF | 小数部分（3位） | 000-999 |

### 参数值计算

```
参数值 = 整数部分 + 小数部分/1000
```

例如：`02050` → 2 + 50/1000 = 2.050

## 命令示例

### X轴参数调整

| 命令 | 说明 | 参数值 |
|------|------|--------|
| `PXP02050` | 设置X轴Kp=2.050 | 2.050 |
| `PXI00010` | 设置X轴Ki=0.010 | 0.010 |
| `PXD00500` | 设置X轴Kd=0.500 | 0.500 |
| `PXP01000` | 设置X轴Kp=1.000 | 1.000 |
| `PXI00005` | 设置X轴Ki=0.005 | 0.005 |
| `PXD00800` | 设置X轴Kd=0.800 | 0.800 |

### Y轴参数调整

| 命令 | 说明 | 参数值 |
|------|------|--------|
| `PYP02050` | 设置Y轴Kp=2.050 | 2.050 |
| `PYI00010` | 设置Y轴Ki=0.010 | 0.010 |
| `PYD00500` | 设置Y轴Kd=0.500 | 0.500 |
| `PYP01500` | 设置Y轴Kp=1.500 | 1.500 |
| `PYI00020` | 设置Y轴Ki=0.020 | 0.020 |
| `PYD01000` | 设置Y轴Kd=1.000 | 1.000 |

### 常用参数组合示例

#### 快速响应配置
```
PXP02000  # X轴 Kp=2.000 (提高响应速度)
PXI00020  # X轴 Ki=0.020 (快速消除误差)
PXD00800  # X轴 Kd=0.800 (增强阻尼)

PYP02000  # Y轴 Kp=2.000
PYI00020  # Y轴 Ki=0.020
PYD00800  # Y轴 Kd=0.800
```

#### 平稳控制配置
```
PXP00800  # X轴 Kp=0.800 (降低响应速度)
PXI00005  # X轴 Ki=0.005 (缓慢消除误差)
PXD00300  # X轴 Kd=0.300 (较小阻尼)

PYP00800  # Y轴 Kp=0.800
PYI00005  # Y轴 Ki=0.005
PYD00300  # Y轴 Kd=0.300
```

#### 默认配置（恢复）
```
PXPII01000  # X轴 Kp=1.000
PXPI00010  # X轴 Ki=0.010
PXDI00500  # X轴 Kd=0.500

PYPII01000  # Y轴 Kp=1.000
PYPI00010  # Y轴 Ki=0.010
PYDI00500  # Y轴 Kd=0.500
```

## 参数范围建议

### Kp (比例系数)
- **推荐范围**: 0.5 ~ 5.0
- **作用**: 控制响应速度
- **调整建议**:
  - 增大Kp：响应更快，但可能震荡
  - 减小Kp：响应更慢，但更稳定

### Ki (积分系数)
- **推荐范围**: 0.001 ~ 0.1
- **作用**: 消除稳态误差
- **调整建议**:
  - 增大Ki：更快消除误差，但可能超调
  - 减小Ki：更稳定，但收敛慢

### Kd (微分系数)
- **推荐范围**: 0.1 ~ 2.0
- **作用**: 减少超调和震荡
- **调整建议**:
  - 增大Kd：减少超调，但对噪声敏感
  - 减小Kd：平滑但可能震荡

## 数值转换表

### 常用Kp值对照

| 参数值 | 整数部分 | 小数部分 | 命令示例 |
|--------|----------|----------|----------|
| 0.5 | 00 | 500 | PXPII00500 |
| 1.0 | 01 | 000 | PXPII01000 |
| 1.5 | 01 | 500 | PXPII01500 |
| 2.0 | 02 | 000 | PXPII02000 |
| 2.5 | 02 | 500 | PXPII02500 |
| 3.0 | 03 | 000 | PXPII03000 |

### 常用Ki值对照

| 参数值 | 整数部分 | 小数部分 | 命令示例 |
|--------|----------|----------|----------|
| 0.001 | 00 | 001 | PXPI00001 |
| 0.005 | 00 | 005 | PXPI00005 |
| 0.010 | 00 | 010 | PXPI00010 |
| 0.020 | 00 | 020 | PXPI00020 |
| 0.050 | 00 | 050 | PXPI00050 |
| 0.100 | 00 | 100 | PXPI00100 |

### 常用Kd值对照

| 参数值 | 整数部分 | 小数部分 | 命令示例 |
|--------|----------|----------|----------|
| 0.1 | 00 | 100 | PXDI00100 |
| 0.3 | 00 | 300 | PXDI00300 |
| 0.5 | 00 | 500 | PXDI00500 |
| 0.8 | 00 | 800 | PXDI00800 |
| 1.0 | 01 | 000 | PXDI01000 |
| 1.5 | 01 | 500 | PXDI01500 |

## Python测试代码

### 基本发送函数

```python
import serial

def send_pid_param(ser, axis, param_type, value):
    """
    发送PID参数调整命令
    
    参数:
        ser: 串口对象
        axis: 'X' 或 'Y'
        param_type: 'P', 'I', 或 'D'
        value: 参数值 (float, 0.000-99.999)
    """
    # 分离整数和小数部分
    int_part = int(value)
    frac_part = int((value - int_part) * 1000)
    
    # 确保范围有效
    if int_part > 99:
        int_part = 99
    if frac_part > 999:
        frac_part = 999
    
    # 构建命令: P[Axis][Param][II][FFF]
    command = f"P{axis}{param_type}{int_part:02d}{frac_part:03d}"
    
    # 发送
    ser.write(command.encode('ascii'))
    print(f"✓ 发送: {command} ({axis}轴 K{param_type.lower()}={value:.3f})")

# 使用示例
ser = serial.Serial('COM6', 115200)

# 设置X轴PID参数
send_pid_param(ser, 'X', 'P', 2.050)  # Kp=2.050
send_pid_param(ser, 'X', 'I', 0.010)  # Ki=0.010
send_pid_param(ser, 'X', 'D', 0.500)  # Kd=0.500

# 设置Y轴PID参数
send_pid_param(ser, 'Y', 'P', 1.500)  # Kp=1.500
send_pid_param(ser, 'Y', 'I', 0.020)  # Ki=0.020
send_pid_param(ser, 'Y', 'D', 0.800)  # Kd=0.800

ser.close()
```

### 完整调参工具

```python
import serial
import time

class PIDTuner:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=1)
        
    def set_param(self, axis, param_type, value):
        """设置单个参数"""
        int_part = int(value)
        frac_part = int((value - int_part) * 1000)
        command = f"P{axis}{param_type}{int_part:02d}{frac_part:03d}"
        self.ser.write(command.encode('ascii'))
        print(f"设置 {axis}轴 K{param_type}={value:.3f}")
        time.sleep(0.05)
    
    def set_axis_params(self, axis, kp, ki, kd):
        """设置某个轴的所有参数"""
        print(f"\n设置{axis}轴参数:")
        self.set_param(axis, 'P', kp)
        self.set_param(axis, 'I', ki)
        self.set_param(axis, 'D', kd)
    
    def set_all_params(self, kp_x, ki_x, kd_x, kp_y, ki_y, kd_y):
        """设置所有参数"""
        print("\n设置所有PID参数:")
        self.set_axis_params('X', kp_x, ki_x, kd_x)
        self.set_axis_params('Y', kp_y, ki_y, kd_y)
    
    def reset_to_default(self):
        """恢复默认参数"""
        print("\n恢复默认参数...")
        self.set_all_params(1.0, 0.01, 0.5, 1.0, 0.01, 0.5)
    
    def close(self):
        self.ser.close()

# 使用示例
tuner = PIDTuner('COM6')

# 方案1: 快速响应
tuner.set_all_params(2.0, 0.02, 0.8, 2.0, 0.02, 0.8)

# 等待观察效果
time.sleep(5)

# 方案2: 平稳控制
tuner.set_all_params(0.8, 0.005, 0.3, 0.8, 0.005, 0.3)

# 恢复默认
tuner.reset_to_default()

tuner.close()
```

## 串口调试助手测试

### 配置
- **端口**: COM6 (UART6)
- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: 无
- **发送模式**: ASCII文本

### 测试步骤

1. 打开串口调试助手
2. 选择"文本发送"模式
3. 输入命令（例如：`PXPII02000`）
4. 点击发送
5. 观察云台响应变化

## 调参流程建议

### 步骤1: 调整Kp
1. 将Ki和Kd设为0
   ```
   PXPI00000
   PXDI00000
   ```
2. 逐步增大Kp，观察响应
   ```
   PXPII00500  # Kp=0.5
   PXPII01000  # Kp=1.0
   PXPII01500  # Kp=1.5
   PXPII02000  # Kp=2.0
   ```
3. 找到开始震荡的Kp值，选择稍小的值

### 步骤2: 调整Kd
1. 在找到的Kp基础上，逐步增大Kd
   ```
   PXDI00100  # Kd=0.1
   PXDI00300  # Kd=0.3
   PXDI00500  # Kd=0.5
   PXDI00800  # Kd=0.8
   ```
2. 观察超调和震荡是否减小

### 步骤3: 调整Ki
1. 最后加入Ki，从很小的值开始
   ```
   PXPI00001  # Ki=0.001
   PXPI00005  # Ki=0.005
   PXPI00010  # Ki=0.010
   ```
2. 观察稳态误差是否消除

### 步骤4: 微调
1. 根据实际效果微调各参数
2. Y轴重复以上步骤

## 注意事项

1. **参数更新时机**
   - 参数更新后会立即生效
   - 同时会复位PID状态（清除积分项）

2. **参数范围限制**
   - 整数部分: 00-99
   - 小数部分: 000-999
   - 最大值: 99.999

3. **调参安全**
   - 建议小幅度调整参数
   - 每次调整后观察效果
   - 记录有效的参数组合

4. **命令格式**
   - 必须严格按照8字符格式
   - 大小写敏感（P、X、Y、I、D必须大写）
   - 数字必须补齐位数

## 常见问题

### Q: 如何快速恢复默认参数？
A: 发送以下6条命令：
```
PXPII01000
PXPI00010
PXDI00500
PYPII01000
PYPI00010
PYDI00500
```

### Q: 参数调整后效果不明显？
A: 可能原因：
1. 参数变化太小
2. 当前误差很小，看不出差异
3. 可以结合阶跃测试观察

### Q: 如何验证参数是否生效？
A: 使用阶跃测试：
1. 发送固定误差 `E1100010000` (X=1000)
2. 调整参数
3. 再次发送相同误差
4. 对比响应时间和超调

## 故障排查

| 问题 | 可能原因 | 解决方法 |
|------|---------|----------|
| 参数无变化 | 命令格式错误 | 检查8字符格式 |
| 系统不稳定 | 参数过大 | 降低Kp和Ki |
| 响应太慢 | Kp太小 | 增大Kp值 |
| 持续震荡 | Kd太小 | 增大Kd值 |
| 有稳态误差 | Ki太小 | 增大Ki值 |

## 参数保存

**注意**: 当前参数调整是临时的，掉电后会恢复到代码中的默认值。如果需要永久保存：

1. 找到最佳参数组合
2. 修改 `Vision.c` 中 `Gimbal_Init()` 函数
3. 重新编译下载

```c
void Gimbal_Init(void)
{
    // 修改这里的参数为你测试出的最佳值
    PID_Init(&pid_x, 2.0f, 0.02f, 0.8f, 5000.0f, 9999.0f, -9999.0f);
    PID_Init(&pid_y, 2.0f, 0.02f, 0.8f, 5000.0f, 9999.0f, -9999.0f);
}
```
