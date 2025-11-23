"""
USART3 舵机控制测试脚本
用于测试STM32二维云台的舵机控制功能

命令格式: M[ID][±][AAAA]
示例: M210500 - 舵机2移动到+500
      M301000 - 舵机3移动到-1000
"""

import serial
import time
import sys


def send_servo_command(ser, motor_id, position):
    """
    发送舵机控制命令到USART3
    
    命令格式: M[ID][Sign][AAAA]
    - M: 命令头
    - ID: 电机地址 (2或3)
    - Sign: 符号 (1=正, 0=负)
    - AAAA: 4位位置值 (0000-9999)
    
    参数:
        ser: 串口对象
        motor_id: 电机ID (2 或 3)
        position: 目标位置 (-9999 到 9999)
    
    返回:
        True: 发送成功
        False: 发送失败
    """
    # 检查电机ID
    if motor_id not in [2, 3]:
        print(f"错误：电机ID {motor_id} 无效！只支持ID 2 和 3")
        return False
    
    # 检查位置范围
    if position < -9999 or position > 9999:
        print(f"错误：位置 {position} 超出范围！(-9999 到 9999)")
        return False
    
    # 确定符号位和绝对值
    if position >= 0:
        sign = '1'
        abs_pos = position
    else:
        sign = '0'
        abs_pos = -position
    
    # 构建命令字符串: M[ID][Sign][AAAA]
    # 位置值格式化为4位数字，不足补0
    command = f"M{motor_id}{sign}{abs_pos:04d}"
    
    # 发送命令
    try:
        ser.write(command.encode('ascii'))
        print(f"✓ 发送命令: {command}")
        print(f"  电机ID: {motor_id}, 目标位置: {position:+d}")
        return True
    except Exception as e:
        print(f"✗ 发送失败: {e}")
        return False


def test_sequence(ser):
    """
    执行测试序列
    """
    print("\n" + "="*60)
    print("开始舵机测试序列")
    print("="*60)
    
    # 测试1: 舵机2移动到位置+500
    print("\n[测试 1] 舵机2移动到位置+500")
    send_servo_command(ser, 2, 500)
    time.sleep(2)
    
    # 测试2: 舵机3移动到位置-1000
    print("\n[测试 2] 舵机3移动到位置-1000")
    send_servo_command(ser, 3, -1000)
    time.sleep(2)
    
    # 测试3: 舵机2回到原点
    print("\n[测试 3] 舵机2回到原点(0)")
    send_servo_command(ser, 2, 0)
    time.sleep(2)
    
    # 测试4: 舵机3回到原点
    print("\n[测试 4] 舵机3回到原点(0)")
    send_servo_command(ser, 3, 0)
    time.sleep(2)
    
    # 测试5: 同时控制两个舵机
    print("\n[测试 5] 同时控制两个舵机")
    send_servo_command(ser, 2, 1000)
    time.sleep(0.1)
    send_servo_command(ser, 3, 1000)
    time.sleep(2)
    
    # 测试6: 测试最大值
    print("\n[测试 6] 舵机2移动到最大位置+9999")
    send_servo_command(ser, 2, 9999)
    time.sleep(2)
    
    # 测试7: 测试最小值
    print("\n[测试 7] 舵机3移动到最小位置-9999")
    send_servo_command(ser, 3, -9999)
    time.sleep(2)
    
    # 测试8: 回到原点
    print("\n[测试 8] 所有舵机回到原点")
    send_servo_command(ser, 2, 0)
    time.sleep(0.1)
    send_servo_command(ser, 3, 0)
    
    print("\n" + "="*60)
    print("测试序列完成")
    print("="*60 + "\n")


def interactive_mode(ser):
    """
    交互模式：手动输入电机ID和位置
    """
    print("\n" + "="*60)
    print("进入交互模式 (输入 'q' 退出)")
    print("="*60)
    print("\n提示: 位置范围 -9999 到 +9999")
    
    while True:
        try:
            # 获取电机ID
            motor_input = input("\n请输入电机ID (2 或 3, 或 'q' 退出): ").strip()
            if motor_input.lower() == 'q':
                break
            
            try:
                motor_id = int(motor_input)
            except ValueError:
                print("错误：请输入有效的数字！")
                continue
            
            # 获取目标位置
            pos_input = input("请输入目标位置 (-9999 到 9999): ").strip()
            try:
                position = int(pos_input)
            except ValueError:
                print("错误：请输入有效的数字！")
                continue
            
            # 发送命令
            send_servo_command(ser, motor_id, position)
            
        except KeyboardInterrupt:
            print("\n\n用户中断")
            break
    
    print("\n退出交互模式")


def main():
    """
    主函数
    """
    # 串口配置
    # Windows: 通常是 COM1, COM2, COM3 等
    # Linux: 通常是 /dev/ttyUSB0, /dev/ttyACM0 等
    # macOS: 通常是 /dev/cu.usbserial-* 等
    
    print("="*60)
    print("USART3 舵机控制测试程序")
    print("="*60)
    
    # 输入串口号
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = input("\n请输入串口号 (例如 COM3 或 /dev/ttyUSB0): ").strip()
        if not port:
            port = "COM3"  # 默认值
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
        
        # 选择模式
        while True:
            print("\n请选择测试模式:")
            print("  1 - 自动测试序列")
            print("  2 - 交互模式")
            print("  q - 退出")
            
            choice = input("\n请输入选项: ").strip()
            
            if choice == '1':
                test_sequence(ser)
            elif choice == '2':
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
