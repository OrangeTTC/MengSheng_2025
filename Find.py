"""
OpenMV 摄像头示例：
 - 识别红外光线（IR）并进行检测，
 - 显示灰度图像（黑白），更适合红外检测，
 - 标注检测到的红外光斑区域（矩形、十字与文字），
 - 所有注释均为中文，便于在 OpenMV IDE 中直接使用和调试。
 - 内存优化版本：避免使用 img.copy() 防止 MemoryError

使用说明：
 1. 把此脚本复制到 OpenMV 板上（例如保存为 main.py 或其他文件名）。
 2. 确保你的 OpenMV 板已移除红外滤光片（IR-Cut Filter），或使用红外摄像头。
 3. 启动 OpenMV IDE，打开串口/摄像头预览，加载并运行脚本。
 4. 对准红外 LED 光源（850nm-940nm），观察是否检测到白色亮点。
 5. 如果检测不到，降低 binary_threshold 第一个值（如改为 100 或 80）。
 6. 如果噪点太多，提高 binary_threshold 第一个值（如改为 180 或 200）。

注意：
 - 需要硬件支持：移除 IR-Cut 滤光片或使用 NoIR（无红外滤光片）摄像头模块。
 - 关闭自动增益和自动曝光，以便更准确地检测红外光源。
 - 红外 LED（850nm-940nm）在灰度图像中会显示为高亮白点。
 - 建议在暗环境下使用，避免可见光干扰。
 - 显示灰度图像，红外光斑用白色标注。
 - 已降低检测阈值（150-255）和面积阈值（20）以提高灵敏度。
"""

import sensor
import image
import time
import pyb

# ------------------------- 配置区（可调节） -------------------------
# 曝光控制（微秒，1000-50000 范围）
# 数值越小图像越暗，越大越亮；根据环境光调整
EXPOSURE_TIME_US = 10000  # 10ms 曝光时间，可调节

# 红外光线检测阈值（0-255）
# 像素灰度值在此范围内会被识别为红外光
# 降低阈值以提高检测灵敏度，如果检测不到红外光，尝试降低最小值
binary_threshold = (250, 255)  # (min, max)，已降低阈值提高灵敏度

# 光强判断阈值（0-255）
# 只有光斑的平均亮度或最大亮度超过此值，才会被识别为有效红外光源
MIN_LIGHT_INTENSITY = 255 # 最小光强阈值，数值越高要求越严格

# 用于 find_blobs 的像素与面积门槛，过滤小噪声
# 降低这些值以检测更小的红外光斑
PIXELS_THRESHOLD = 20   # 最小像素数（已降低）
AREA_THRESHOLD = 20     # 最小面积（已降低）

# 是否启用 UART 输出（如果需要把检测结果发给上位机或单片机，设置为 True）
ENABLE_UART_OUTPUT = False
UART_BUS = 3  # 串口号，根据板子更改（OpenMV v3/v4 常用 3）
UART_BAUD = 115200
# ----------------------------------------------------------------------


def init_sensor():
    """初始化摄像头参数（红外检测模式）"""
    sensor.reset()
    sensor.set_pixformat(sensor.GRAYSCALE)  # 使用灰度模式（更适合红外检测）
    sensor.set_framesize(sensor.QVGA)       # 320x240，可改为 QQVGA (160x120) 提速

    # 关闭自动增益和自动曝光，提高红外检测稳定性
    sensor.set_auto_gain(False)      # 关闭自动增益
    sensor.set_auto_exposure(False)  # 关闭自动曝光
    sensor.set_auto_whitebal(False)  # 关闭自动白平衡

    # 手动设置曝光时间，防止过曝
    # 如果你的 OpenMV 支持 set_exposure_us()，则使用此方法
    try:
        sensor.set_exposure_us(EXPOSURE_TIME_US)  # 设置曝光时间（微秒）
        print("曝光时间已设置为: %d us" % EXPOSURE_TIME_US)
    except:
        # 如果不支持，尝试使用寄存器设置（适用于 OV7725 等传感器）
        print("不支持 set_exposure_us，使用默认曝光")

    sensor.skip_frames(time=2000)    # 等待摄像头稳定


def main():
    init_sensor()
    clock = time.clock()

    # 可选：初始化 UART（注释/取消注释来开启）
    uart = None
    if ENABLE_UART_OUTPUT:
        uart = pyb.UART(UART_BUS, UART_BAUD, timeout_char=1000)

    while True:
        clock.tick()
        img = sensor.snapshot()  # 获取灰度图像

        # 1) 直接在灰度图上查找高亮区域（红外光斑）
        # 使用 binary_threshold 作为灰度阈值
        ir_blobs = img.find_blobs([binary_threshold], pixels_threshold=PIXELS_THRESHOLD, area_threshold=AREA_THRESHOLD, merge=True)

        # 2) 对检测到的光斑进行光强判断，只标注强度足够的光源
        valid_blobs = []  # 存储有效的光斑
        for b in ir_blobs:
            # 获取光斑区域的统计信息
            stats = img.get_statistics(roi=b.rect())
            # stats.l_mean() 是该区域的平均亮度
            # stats.l_max() 是该区域的最大亮度
            avg_intensity = stats.l_mean()
            max_intensity = stats.l_max()

            # 判断：平均亮度或最大亮度超过阈值才认为是有效红外光源
            if max_intensity >= MIN_LIGHT_INTENSITY:
                valid_blobs.append((b, avg_intensity, max_intensity))

        # 3) 绘制通过光强判断的红外光斑标注（用白色绘制在灰度图上）
        for b, avg_int, max_int in valid_blobs:
            img.draw_rectangle(b.rect(), color=255)  # 白色
            img.draw_cross(b.cx(), b.cy(), color=255)
            # 构造标签文字：中心坐标、面积和光强
            label = 'IR cx=%d cy=%d I=%d' % (b.cx(), b.cy(), max_int)
            img.draw_string(b.x(), max(0, b.y()-10), label, color=255, mono_space=False)


        # 4) 如果需要，通过 UART 发送检测信息（格式简单示例）
        if ENABLE_UART_OUTPUT and uart is not None:
            for b, avg_int, max_int in valid_blobs:
                # 例如发送："IR,x,y,w,h,intensity\n"
                try:
                    out = 'IR,%d,%d,%d,%d,%d\n' % (b.cx(), b.cy(), b.w(), b.h(), max_int)
                    uart.write(out)
                except Exception as e:
                    # 如果串口发送失败，继续循环（不要崩溃）
                    pass

        # 可选：在图像上显示当前帧率
        img.draw_string(2, 2, 'FPS:%.1f' % clock.fps(), color=(255, 255, 255))


if __name__ == '__main__':
    main()
