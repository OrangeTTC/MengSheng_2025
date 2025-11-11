# PID动态调参快速参考

## 命令格式
```
P[Axis][Param][IIFFF]
```
- **P**: 命令头
- **Axis**: X 或 Y
- **Param**: P, I, 或 D
- **IIFFF**: 2位整数 + 3位小数 (00000-99999)

## 快速命令表

### 默认配置
```
PXP01000  # X轴 Kp=1.000
PXI00010  # X轴 Ki=0.010
PXD00500  # X轴 Kd=0.500
PYP01000  # Y轴 Kp=1.000
PYI00010  # Y轴 Ki=0.010
PYD00500  # Y轴 Kd=0.500
```

### 快速响应配置
```
PXP02000  # X轴 Kp=2.000
PXI00020  # X轴 Ki=0.020
PXD00800  # X轴 Kd=0.800
PYP02000  # Y轴 Kp=2.000
PYI00020  # Y轴 Ki=0.020
PYD00800  # Y轴 Kd=0.800
```

### 平稳控制配置
```
PXP00800  # X轴 Kp=0.800
PXI00005  # X轴 Ki=0.005
PXD00300  # X轴 Kd=0.300
PYP00800  # Y轴 Kp=0.800
PYI00005  # Y轴 Ki=0.005
PYD00300  # Y轴 Kd=0.300
```

## 常用数值转换

| 参数值 | 命令格式 | 示例 |
|--------|----------|------|
| 0.001 | 00001 | PXI00001 |
| 0.01 | 00010 | PXI00010 |
| 0.1 | 00100 | PXD00100 |
| 0.5 | 00500 | PXD00500 |
| 1.0 | 01000 | PXP01000 |
| 1.5 | 01500 | PXP01500 |
| 2.0 | 02000 | PXP02000 |
| 2.5 | 02500 | PXP02500 |

## Python快速使用

```python
import serial

ser = serial.Serial('COM6', 115200)

def set_pid(axis, param, value):
    ii = int(value)
    fff = int((value - ii) * 1000)
    cmd = f"P{axis}{param}{ii:02d}{fff:03d}"
    ser.write(cmd.encode())
    print(cmd)

# 设置X轴Kp=2.0
set_pid('X', 'P', 2.0)

# 设置Y轴Ki=0.02
set_pid('Y', 'I', 0.02)

ser.close()
```

## 问题快速解决

| 问题 | 调整建议 | 命令示例 |
|------|----------|----------|
| 响应慢 | 增大Kp | PXP02000 |
| 震荡 | 减小Kp，增大Kd | PXP00800, PXD01000 |
| 超调 | 增大Kd | PXD01000 |
| 稳态误差 | 增大Ki | PXI00020 |

## 测试工具
```bash
python test_pid_tuning.py
```

## 串口配置
- 端口: COM6
- 波特率: 115200
- 模式: ASCII文本
