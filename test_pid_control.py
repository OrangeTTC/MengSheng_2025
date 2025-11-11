"""
云台PID控制 - 视觉误差测试工具
通过UART6发送位置误差数据来测试PID控制系统

命令格式: E[X_Sign][XXXX][Y_Sign][YYYY]
示例: E1005001200 - X误差=+500, Y误差=+1200
"""

import serial
import time
import sys
import math


def send_vision_error(ser, error_x, error_y):
    """
    发送视觉位置误差到STM32
    
    命令格式: E[X_Sign][XXXX][Y_Sign][YYYY]
    
    参数:
        ser: 串口对象
        error_x: X轴位置误差 (-9999 到 9999) 像素
        error_y: Y轴位置误差 (-9999 到 9999) 像素
    
    返回:
        True: 发送成功
        False: 发送失败
    """
    # 检查误差范围
    if error_x < -9999 or error_x > 9999:
        print(f"错误：X轴误差 {error_x} 超出范围！(-9999 到 9999)")
        return False
    
    if error_y < -9999 or error_y > 9999:
        print(f"错误：Y轴误差 {error_y} 超出范围！(-9999 到 9999)")
        return False
    
    # 确定符号位和绝对值
    x_sign = '1' if error_x >= 0 else '0'
    y_sign = '1' if error_y >= 0 else '0'
    x_abs = abs(error_x)
    y_abs = abs(error_y)
    
    # 构建命令字符串: E[X_Sign][XXXX][Y_Sign][YYYY]
    # 误差值格式化为4位数字，不足补0
    command = f"E{x_sign}{x_abs:04d}{y_sign}{y_abs:04d}"
    
    # 发送命令
    try:
        ser.write(command.encode('ascii'))
        print(f"✓ 发送: {command} | X误差: {error_x:+5d}px, Y误差: {error_y:+5d}px")
        return True
    except Exception as e:
        print(f"✗ 发送失败: {e}")
        return False


def test_basic_positions(ser):
    """
    基本位置测试序列
    """
    print("\n" + "="*60)
    print("基本位置测试")
    print("="*60)
    
    tests = [
        (0, 0, "目标居中"),
        (500, 0, "目标在右侧"),
        (-500, 0, "目标在左侧"),
        (0, 500, "目标在上方"),
        (0, -500, "目标在下方"),
        (500, 500, "目标在右上方"),
        (-500, -500, "目标在左下方"),
        (0, 0, "回到中心"),
    ]
    
    for error_x, error_y, desc in tests:
        print(f"\n[测试] {desc}")
        send_vision_error(ser, error_x, error_y)
        time.sleep(2)
    
    print("\n" + "="*60)
    print("基本测试完成")
    print("="*60 + "\n")


def test_circular_motion(ser, radius=500, steps=36, speed=0.1):
    """
    圆形运动轨迹测试
    
    参数:
        radius: 圆形半径（像素）
        steps: 圆周分割点数
        speed: 每步延迟时间（秒）
    """
    print("\n" + "="*60)
    print(f"圆形运动测试 (半径={radius}px, 步数={steps})")
    print("="*60 + "\n")
    
    for i in range(steps):
        angle = (360.0 / steps) * i
        rad = math.radians(angle)
        
        error_x = int(radius * math.cos(rad))
        error_y = int(radius * math.sin(rad))
        
        send_vision_error(ser, error_x, error_y)
        time.sleep(speed)
    
    # 回到中心
    print("\n回到中心...")
    send_vision_error(ser, 0, 0)
    
    print("\n圆形运动测试完成\n")


def test_square_motion(ser, size=800, speed=1.0):
    """
    方形运动轨迹测试
    
    参数:
        size: 方形边长（像素）
        speed: 每边延迟时间（秒）
    """
    print("\n" + "="*60)
    print(f"方形运动测试 (边长={size}px)")
    print("="*60 + "\n")
    
    half = size // 2
    
    corners = [
        (half, half, "右上角"),
        (-half, half, "左上角"),
        (-half, -half, "左下角"),
        (half, -half, "右下角"),
        (0, 0, "中心"),
    ]
    
    for error_x, error_y, desc in corners:
        print(f"\n移动到 {desc}")
        send_vision_error(ser, error_x, error_y)
        time.sleep(speed)
    
    print("\n方形运动测试完成\n")


def test_step_response(ser, step_size=1000, duration=5):
    """
    阶跃响应测试（用于PID参数调整）
    
    参数:
        step_size: 阶跃大小（像素）
        duration: 持续时间（秒）
    """
    print("\n" + "="*60)
    print(f"阶跃响应测试 (阶跃大小={step_size}px)")
    print("="*60 + "\n")
    
    print("初始状态：居中")
    send_vision_error(ser, 0, 0)
    time.sleep(2)
    
    print(f"\n阶跃：X轴 +{step_size}px")
    send_vision_error(ser, step_size, 0)
    print(f"观察{duration}秒...")
    time.sleep(duration)
    
    print("\n回到中心")
    send_vision_error(ser, 0, 0)
    time.sleep(2)
    
    print(f"\n阶跃：Y轴 +{step_size}px")
    send_vision_error(ser, 0, step_size)
    print(f"观察{duration}秒...")
    time.sleep(duration)
    
    print("\n回到中心")
    send_vision_error(ser, 0, 0)
    
    print("\n阶跃响应测试完成\n")


def interactive_mode(ser):
    """
    交互模式：手动输入误差值
    """
    print("\n" + "="*60)
    print("进入交互模式 (输入 'q' 退出)")
    print("="*60)
    print("\n提示: 误差范围 -9999 到 +9999 像素")
    print("正值表示目标在右/上方，负值表示目标在左/下方\n")
    
    while True:
        try:
            # 获取X轴误差
            x_input = input("请输入X轴误差 (或 'q' 退出): ").strip()
            if x_input.lower() == 'q':
                break
            
            try:
                error_x = int(x_input)
            except ValueError:
                print("错误：请输入有效的数字！")
                continue
            
            # 获取Y轴误差
            y_input = input("请输入Y轴误差: ").strip()
            try:
                error_y = int(y_input)
            except ValueError:
                print("错误：请输入有效的数字！")
                continue
            
            # 发送命令
            send_vision_error(ser, error_x, error_y)
            print()
            
        except KeyboardInterrupt:
            print("\n\n用户中断")
            break
    
    print("\n退出交互模式")


def continuous_tracking_simulation(ser, frequency=20, duration=10):
    """
    模拟连续跟踪（随机目标移动）
    
    参数:
        frequency: 更新频率 (Hz)
        duration: 持续时间（秒）
    """
    import random
    
    print("\n" + "="*60)
    print(f"连续跟踪模拟 (频率={frequency}Hz, 时长={duration}秒)")
    print("="*60 + "\n")
    
    interval = 1.0 / frequency
    steps = int(duration * frequency)
    
    # 初始误差
    error_x = 0
    error_y = 0
    
    for i in range(steps):
        # 模拟目标缓慢移动
        error_x += random.randint(-50, 50)
        error_y += random.randint(-50, 50)
        
        # 限制范围
        error_x = max(-1500, min(1500, error_x))
        error_y = max(-1500, min(1500, error_y))
        
        send_vision_error(ser, error_x, error_y)
        time.sleep(interval)
    
    # 回到中心
    print("\n回到中心...")
    send_vision_error(ser, 0, 0)
    
    print("\n连续跟踪模拟完成\n")


def main():
    """
    主函数
    """
    print("="*60)
    print("云台PID控制 - 视觉误差测试工具")
    print("="*60)
    
    # 输入串口号
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = input("\n请输入串口号 (例如 COM6 或 /dev/ttyUSB0): ").strip()
        if not port:
            port = "COM6"  # 默认值
            print(f"使用默认端口: {port}")
    
    baudrate = 115200
    
    try:
        # 打开串口
        print(f"\n正在打开串口 {port}，波特率 {baudrate}...")
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )
        print(f"✓ 串口 {port} 已成功打开\n")
        
        # 主菜单
        while True:
            print("\n请选择测试模式:")
            print("  1 - 基本位置测试")
            print("  2 - 圆形运动测试")
            print("  3 - 方形运动测试")
            print("  4 - 阶跃响应测试（PID调试）")
            print("  5 - 连续跟踪模拟")
            print("  6 - 交互模式")
            print("  q - 退出")
            
            choice = input("\n请输入选项: ").strip()
            
            if choice == '1':
                test_basic_positions(ser)
            elif choice == '2':
                radius = input("输入圆形半径（默认500）: ").strip()
                radius = int(radius) if radius else 500
                test_circular_motion(ser, radius=radius)
            elif choice == '3':
                size = input("输入方形边长（默认800）: ").strip()
                size = int(size) if size else 800
                test_square_motion(ser, size=size)
            elif choice == '4':
                step = input("输入阶跃大小（默认1000）: ").strip()
                step = int(step) if step else 1000
                test_step_response(ser, step_size=step)
            elif choice == '5':
                freq = input("输入更新频率Hz（默认20）: ").strip()
                freq = int(freq) if freq else 20
                continuous_tracking_simulation(ser, frequency=freq)
            elif choice == '6':
                interactive_mode(ser)
            elif choice.lower() == 'q':
                break
            else:
                print("无效选项，请重新选择")
        
        # 关闭串口
        ser.close()
        print("\n✓ 串口已关闭")
        
    except serial.SerialException as e:
        print(f"\n✗ 串口错误: {e}")
        print("\n提示:")
        print("  1. 检查串口号是否正确")
        print("  2. 确认设备已连接")
        print("  3. 检查是否有其他程序占用该串口")
        sys.exit(1)
    
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
        try:
            ser.close()
        except:
            pass
    
    except Exception as e:
        print(f"\n✗ 发生错误: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
