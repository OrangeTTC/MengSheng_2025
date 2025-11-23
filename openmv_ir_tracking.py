import sensor
import image
import time
import pyb
import math
EXPOSURE_TIME_US = 10000
binary_threshold = (200, 255)
MIN_LIGHT_INTENSITY = 255
PIXELS_THRESHOLD = 20
AREA_THRESHOLD = 20
EXPECTED_LIGHTS = 4
MAX_LIGHTS = 6
ENABLE_UART_OUTPUT = True
UART_BUS = 3
UART_BAUD = 115200
FRAME_WIDTH = 320
FRAME_HEIGHT = 240
CENTER_X = FRAME_WIDTH // 2  # 视觉显示中心
CENTER_Y = FRAME_HEIGHT // 2
TARGET_X = FRAME_WIDTH // 2 + 90  # 目标中心（用于误差计算）
TARGET_Y = FRAME_HEIGHT // 2 + 20
def init_sensor():
    sensor.reset()
    sensor.set_pixformat(sensor.GRAYSCALE)
    sensor.set_framesize(sensor.QVGA)
    sensor.set_auto_gain(False)
    sensor.set_auto_exposure(False)
    sensor.set_auto_whitebal(False)
    sensor.set_hmirror(True)
    sensor.set_vflip(True)
    try:
        sensor.set_exposure_us(EXPOSURE_TIME_US)
        print("曝光时间已设置为: %d us" % EXPOSURE_TIME_US)
    except:
        print("不支持 set_exposure_us，使用默认曝光")
    sensor.skip_frames(time=2000)
def calculate_geometric_center(blobs):
    if len(blobs) == 0:
        return None
    sum_x = 0
    sum_y = 0
    for b in blobs:
        sum_x += b.cx()
        sum_y += b.cy()
    center_x = sum_x // len(blobs)
    center_y = sum_y // len(blobs)
    return (center_x, center_y)
def is_rectangular_pattern(blobs):
    if len(blobs) != 4:
        return False
    points = [(b.cx(), b.cy()) for b in blobs]
    center_x = sum(p[0] for p in points) / 4
    center_y = sum(p[1] for p in points) / 4
    distances = [math.sqrt((p[0]-center_x)**2 + (p[1]-center_y)**2) for p in points]
    avg_dist = sum(distances) / len(distances)
    variance = sum((d - avg_dist)**2 for d in distances) / len(distances)
    std_dev = math.sqrt(variance)
    return std_dev < avg_dist * 0.3
def main():
    init_sensor()
    clock = time.clock()
    uart = None
    if ENABLE_UART_OUTPUT:
        uart = pyb.UART(UART_BUS, UART_BAUD, timeout_char=1000)
    while True:
        clock.tick()
        img = sensor.snapshot()
        ir_blobs = img.find_blobs([binary_threshold],
                                   pixels_threshold=PIXELS_THRESHOLD,
                                   area_threshold=AREA_THRESHOLD,
                                   merge=True)
        valid_blobs = []
        for b in ir_blobs:
            stats = img.get_statistics(roi=b.rect())
            max_intensity = stats.l_max()
            if max_intensity >= MIN_LIGHT_INTENSITY:
                valid_blobs.append((b, max_intensity))
        valid_blobs.sort(key=lambda x: x[1], reverse=True)
        top_blobs = [b for b, _ in valid_blobs[:MAX_LIGHTS]]
        for i, b in enumerate(top_blobs):
            color = 255 if i < EXPECTED_LIGHTS else 128
            img.draw_rectangle(b.rect(), color=color)
            img.draw_cross(b.cx(), b.cy(), color=color, size=5)
            label = 'L%d' % (i + 1)
            img.draw_string(b.x(), max(0, b.y()-10), label, color=color)
        geometric_center = None
        if len(top_blobs) >= EXPECTED_LIGHTS:
            four_lights = top_blobs[:EXPECTED_LIGHTS]
            if is_rectangular_pattern(four_lights):
                geometric_center = calculate_geometric_center(four_lights)
                if geometric_center:
                    cx, cy = geometric_center
                    img.draw_cross(cx, cy, color=255, size=15, thickness=2)
                    img.draw_circle(cx, cy, 10, color=255, thickness=2)
                    center_label = 'Center (%d,%d)' % (cx , cy)
                    img.draw_string(2, 30, center_label, color=255)
                    for b in four_lights:
                        img.draw_line(b.cx(), b.cy(), cx, cy, color=200)
            else:
                geometric_center = calculate_geometric_center(four_lights)
                if geometric_center:
                    cx, cy = geometric_center
                    img.draw_cross(cx, cy, color=128, size=10)
                    img.draw_string(2, 30, 'Warning: Not Rect', color=128)
        img.draw_cross(CENTER_X, CENTER_Y, color=100, size=10)
        if ENABLE_UART_OUTPUT and uart is not None and geometric_center:
            cx, cy = geometric_center
            error_x = TARGET_X - cx  # 使用目标中心计算误差
            error_y = cy - TARGET_Y
            error_x = max(-9999, min(9999, error_x))
            error_y = max(-9999, min(9999, error_y))
            x_sign = '1' if error_x >= 0 else '0'
            x_abs = abs(error_x)
            y_sign = '1' if error_y >= 0 else '0'
            y_abs = abs(error_y)
            frame = 'E%s%04d%s%04d' % (x_sign, x_abs, y_sign, y_abs)
            try:
                uart.write(frame)
                error_info = 'X:%+d Y:%+d' % (error_x, error_y)
                img.draw_string(2, 220, error_info, color=255)
                img.draw_string(2, 232, frame, color=255)
            except Exception as e:
                pass
        status = 'Lights: %d/%d' % (len(top_blobs), EXPECTED_LIGHTS)
        img.draw_string(2, 2, status, color=255)
        img.draw_string(2, 14, 'FPS:%.1f' % clock.fps(), color=255)
if __name__ == '__main__':
    main()
