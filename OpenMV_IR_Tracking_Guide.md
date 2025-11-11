# OpenMV 红外光追踪配置说明

## 硬件连接

### OpenMV与STM32连接
```
OpenMV P4 (UART3 TX) --> STM32 PC7  (UART6 RX)
OpenMV P5 (UART3 RX) --> STM32 PC6  (UART6 TX) (可选，用于接收反馈)
OpenMV GND          --> STM32 GND
```

### 摄像头设置
- **偏光片**: 已安装，过滤可见光，增强红外光识别
- **镜头**: 标准镜头
- **环境**: 避免强光直射

## 软件配置

### 1. 关键参数调整

#### 红外阈值 (根据实际环境调整)
```python
IR_THRESHOLD = (200, 255)  # 灰度值范围
```
- **亮度过高**: 降低下限，如 `(180, 255)`
- **检测不到**: 降低下限，如 `(150, 255)`
- **误检过多**: 提高下限，如 `(220, 255)`

#### 目标面积范围
```python
MIN_BLOB_AREA = 10      # 最小面积（像素）
MAX_BLOB_AREA = 5000    # 最大面积（像素）
```
- **目标太小检测不到**: 减小 `MIN_BLOB_AREA`
- **背景噪点太多**: 增大 `MIN_BLOB_AREA`
- **近距离目标被过滤**: 增大 `MAX_BLOB_AREA`

#### 曝光时间
```python
sensor.set_auto_exposure(False, 5000)  # 5000微秒 = 5ms
```
- **图像太亮**: 减小曝光，如 `3000`
- **图像太暗**: 增大曝光，如 `8000`

### 2. 坐标系说明

```
OpenMV图像坐标系:
(0,0) ┌─────────────────┐ (320,0)
      │                 │
      │    (160,120)    │ ← 屏幕中心
      │        ●        │
      │                 │
(0,240)└─────────────────┘ (320,240)

误差计算:
error_x = target_x - 160  (范围: -160 ~ +160)
error_y = target_y - 120  (范围: -120 ~ +120)

误差含义:
- error_x > 0: 目标在中心右侧
- error_x < 0: 目标在中心左侧
- error_y > 0: 目标在中心下方
- error_y < 0: 目标在中心上方
```

### 3. 数据帧格式

```
E[X_Sign][XXXX][Y_Sign][YYYY]
│   │     │      │      │
│   │     │      │      └─ Y轴误差值 (4位数字)
│   │     │      └──────── Y轴符号 (1=正, 0=负)
│   │     └─────────────── X轴误差值 (4位数字)
│   └───────────────────── X轴符号 (1=正, 0=负)
└───────────────────────── 命令头
```

**示例:**
- `E10050011200` - X=+50, Y=+120 (目标在中心右下方)
- `E00030010080` - X=-30, Y=+80 (目标在中心左下方)
- `E10000010000` - X=+0, Y=+0 (目标在中心)

## 使用步骤

### 1. 硬件准备
1. 连接OpenMV与STM32 UART6
2. 确认偏光片已正确安装
3. 准备红外光源（LED或激光）

### 2. OpenMV IDE操作
1. 打开 `openmv_ir_tracking.py`
2. 连接OpenMV到电脑
3. 点击"连接"按钮
4. 点击"运行"按钮（绿色三角）

### 3. 参数调整流程

#### 步骤1: 检查图像
- 观察OpenMV IDE的图像窗口
- 红外光源应显示为**白色亮点**
- 如果看不到红外光，调整曝光时间

#### 步骤2: 调整阈值
```python
# 在OpenMV IDE中打开"工具" -> "机器视觉" -> "阈值编辑器"
# 调整滑块找到合适的灰度范围
# 将结果更新到代码中
IR_THRESHOLD = (调整后的min, 调整后的max)
```

#### 步骤3: 验证检测
- 红外光源出现时，红色LED应亮起
- 图像上应显示：
  - 白色矩形框（目标边界）
  - 白色十字（目标中心）
  - 灰色十字（屏幕中心）
  - 连线（误差向量）

#### 步骤4: 验证串口输出
- 打开OpenMV IDE的"串口终端"
- 应看到类似输出：
  ```
  目标: (180,140) | 误差: X= +20, Y= +20 | E10020010020 | 30.5fps
  ```

### 4. STM32端验证
- 在STM32上应收到误差数据帧
- PID控制器应开始响应
- 云台应自动调整到目标位置

## 调试技巧

### 问题1: 检测不到红外光
**可能原因:**
- 阈值设置过高
- 曝光时间过短
- 偏光片方向错误
- 红外光源功率不足

**解决方法:**
```python
# 降低阈值下限
IR_THRESHOLD = (150, 255)

# 增加曝光时间
sensor.set_auto_exposure(False, 10000)

# 临时启用自动曝光测试
sensor.set_auto_exposure(True)
```

### 问题2: 误检测（检测到背景）
**可能原因:**
- 阈值设置过低
- 环境光干扰
- 反光物体

**解决方法:**
```python
# 提高阈值下限
IR_THRESHOLD = (220, 255)

# 增大最小面积
MIN_BLOB_AREA = 20

# 减小最大面积，过滤大片高亮区域
MAX_BLOB_AREA = 2000
```

### 问题3: 检测不稳定（闪烁）
**可能原因:**
- 红外光源不稳定
- 边缘检测不准确
- 反光导致面积变化

**解决方法:**
```python
# 增加斑点合并
blobs = img.find_blobs(
    [IR_THRESHOLD],
    pixels_threshold=MIN_BLOB_AREA,
    area_threshold=MIN_BLOB_AREA,
    merge=True,
    margin=10  # 添加合并边距
)
```

### 问题4: 没有串口数据
**检查项:**
1. UART连接是否正确
2. 波特率是否匹配（115200）
3. OpenMV IDE串口终端是否看到发送数据
4. STM32 UART6是否正确初始化

## 性能优化

### 提高帧率
```python
# 降低分辨率
sensor.set_framesize(sensor.QQVGA)  # 160x120，更快

# 减少不必要的绘图
# 注释掉部分 img.draw_xxx() 调用
```

### 减少误差抖动
```python
# 添加简单滤波（在主循环外定义）
error_x_filtered = 0
error_y_filtered = 0
alpha = 0.3  # 滤波系数 (0-1)

# 在主循环中使用
error_x_filtered = int(alpha * error_x + (1 - alpha) * error_x_filtered)
error_y_filtered = int(alpha * error_y + (1 - alpha) * error_y_filtered)
send_error_data(error_x_filtered, error_y_filtered)
```

## 高级功能扩展

### 1. 多目标追踪
```python
# 追踪前N个最大目标
top_blobs = sorted(blobs, key=lambda b: b.pixels(), reverse=True)[:3]
```

### 2. 目标丢失处理
```python
# 记录上次有效位置
last_valid_x = CENTER_X
last_valid_y = CENTER_Y
lost_frames = 0

if blobs:
    last_valid_x = target_x
    last_valid_y = target_y
    lost_frames = 0
else:
    lost_frames += 1
    if lost_frames > 30:  # 丢失超过30帧
        # 发送告警或停止发送
        pass
```

### 3. 距离估计
```python
# 根据斑点面积估算距离
# 需要实际标定
KNOWN_WIDTH = 10  # 红外光源实际宽度(mm)
FOCAL_LENGTH = 2.8  # 相机焦距(mm)

def estimate_distance(blob_width):
    # 简化的距离公式
    return (KNOWN_WIDTH * FOCAL_LENGTH) / blob_width
```

## 参数快速参考

| 参数 | 默认值 | 调整建议 |
|------|--------|----------|
| `IR_THRESHOLD` | `(200, 255)` | 室内: 180-220, 室外: 200-240 |
| `MIN_BLOB_AREA` | `10` | 远距离: 5, 近距离: 20 |
| `MAX_BLOB_AREA` | `5000` | 根据目标大小调整 |
| `曝光时间` | `5000μs` | 暗环境: 8000-15000 |
| `分辨率` | `QVGA` | 高速: QQVGA, 精度: VGA |

## 常用OpenMV命令

```python
# 保存当前图像
img.save("snapshot.jpg")

# 查看FPS
print(clock.fps())

# 统计信息
print(largest_blob.area())      # 面积
print(largest_blob.pixels())    # 像素数
print(largest_blob.density())   # 密度
print(largest_blob.w(), largest_blob.h())  # 宽高
```
