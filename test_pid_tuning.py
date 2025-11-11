"""
PID参数动态调整工具
通过UART6实时调整云台PID控制参数

命令格式: P[Axis][Param][IIFFF]
示例: PXP02050 - X轴Kp=2.050
"""

import serial
import time
import sys


def send_pid_param(ser, axis, param_type, value):
    """
    发送PID参数调整命令
    
    参数:
        ser: 串口对象
        axis: 'X' 或 'Y'
        param_type: 'P', 'I', 或 'D'
        value: 参数值 (0.000-99.999)
    
    返回:
        True: 发送成功
        False: 发送失败
    """
    # 验证输入
    if axis not in ['X', 'Y']:
        print(f"错误：轴选择 {axis} 无效！必须是 'X' 或 'Y'")
        return False
    
    if param_type not in ['P', 'I', 'D']:
        print(f"错误：参数类型 {param_type} 无效！必须是 'P', 'I', 或 'D'")
        return False
    
    if value < 0 or value > 99.999:
        print(f"错误：参数值 {value} 超出范围！(0.000-99.999)")
        return False
    
    # 分离整数和小数部分
    int_part = int(value)
    frac_part = int(round((value - int_part) * 1000))
    
    # 处理四舍五入导致的进位
    if frac_part >= 1000:
        int_part += 1
        frac_part = 0
    
    # 确保范围有效
    if int_part > 99:
        int_part = 99
        frac_part = 999
    
    # 构建命令字符串: P[Axis][Param][II][FFF]
    command = f"P{axis}{param_type}{int_part:02d}{frac_part:03d}"
    
    # 发送命令
    try:
        ser.write(command.encode('ascii'))
        param_name = {'P': 'Kp', 'I': 'Ki', 'D': 'Kd'}[param_type]
        print(f"✓ 发送: {command} | {axis}轴 {param_name}={value:.3f}")
        return True
    except Exception as e:
        print(f"✗ 发送失败: {e}")
        return False


class PIDTuner:
    """PID参数调整器类"""
    
    def __init__(self, ser):
        self.ser = ser
        self.current_params = {
            'X': {'P': 1.0, 'I': 0.01, 'D': 0.5},
            'Y': {'P': 1.0, 'I': 0.01, 'D': 0.5}
        }
    
    def set_param(self, axis, param_type, value):
        """设置单个参数"""
        if send_pid_param(self.ser, axis, param_type, value):
            self.current_params[axis][param_type] = value
            time.sleep(0.05)  # 短暂延迟
            return True
        return False
    
    def set_axis_params(self, axis, kp, ki, kd):
        """设置某个轴的所有参数"""
        print(f"\n{'='*50}")
        print(f"设置{axis}轴PID参数")
        print(f"{'='*50}")
        self.set_param(axis, 'P', kp)
        self.set_param(axis, 'I', ki)
        self.set_param(axis, 'D', kd)
        print(f"\n{axis}轴参数设置完成: Kp={kp:.3f}, Ki={ki:.3f}, Kd={kd:.3f}\n")
    
    def set_all_params(self, kp_x, ki_x, kd_x, kp_y, ki_y, kd_y):
        """设置所有参数"""
        print("\n" + "="*50)
        print("设置所有PID参数")
        print("="*50)
        self.set_axis_params('X', kp_x, ki_x, kd_x)
        self.set_axis_params('Y', kp_y, ki_y, kd_y)
        print("所有参数设置完成\n")
    
    def reset_to_default(self):
        """恢复默认参数"""
        print("\n恢复默认参数 (Kp=1.0, Ki=0.01, Kd=0.5)...")
        self.set_all_params(1.0, 0.01, 0.5, 1.0, 0.01, 0.5)
    
    def show_current_params(self):
        """显示当前参数"""
        print("\n" + "="*50)
        print("当前PID参数")
        print("="*50)
        for axis in ['X', 'Y']:
            params = self.current_params[axis]
            print(f"{axis}轴: Kp={params['P']:.3f}, Ki={params['I']:.3f}, Kd={params['D']:.3f}")
        print("="*50 + "\n")


def preset_configs(tuner):
    """预设配置方案"""
    print("\n" + "="*50)
    print("预设PID配置方案")
    print("="*50)
    print("1 - 默认配置 (平衡)")
    print("2 - 快速响应 (激进)")
    print("3 - 平稳控制 (保守)")
    print("4 - 高精度跟踪")
    print("5 - 抗干扰配置")
    print("="*50)
    
    choice = input("\n选择配置方案 (1-5, 或回车跳过): ").strip()
    
    configs = {
        '1': (1.0, 0.01, 0.5, 1.0, 0.01, 0.5, "默认配置"),
        '2': (2.0, 0.02, 0.8, 2.0, 0.02, 0.8, "快速响应"),
        '3': (0.8, 0.005, 0.3, 0.8, 0.005, 0.3, "平稳控制"),
        '4': (1.5, 0.05, 1.0, 1.5, 0.05, 1.0, "高精度跟踪"),
        '5': (1.2, 0.008, 1.5, 1.2, 0.008, 1.5, "抗干扰"),
    }
    
    if choice in configs:
        kp_x, ki_x, kd_x, kp_y, ki_y, kd_y, name = configs[choice]
        print(f"\n应用配置: {name}")
        tuner.set_all_params(kp_x, ki_x, kd_x, kp_y, ki_y, kd_y)


def interactive_tuning(tuner):
    """交互式调参模式"""
    print("\n" + "="*50)
    print("交互式PID调参 (输入 'q' 退出)")
    print("="*50)
    
    while True:
        try:
            # 显示当前参数
            tuner.show_current_params()
            
            # 选择轴
            axis = input("选择轴 (X/Y, 或 'q' 退出): ").strip().upper()
            if axis == 'Q':
                break
            if axis not in ['X', 'Y']:
                print("无效输入，请输入 X 或 Y")
                continue
            
            # 选择参数类型
            param = input("选择参数 (P/I/D): ").strip().upper()
            if param not in ['P', 'I', 'D']:
                print("无效输入，请输入 P, I 或 D")
                continue
            
            # 输入参数值
            value_str = input(f"输入 {axis}轴 K{param.lower()} 的值 (0.000-99.999): ").strip()
            try:
                value = float(value_str)
            except ValueError:
                print("无效的数值格式")
                continue
            
            # 设置参数
            tuner.set_param(axis, param, value)
            print()
            
        except KeyboardInterrupt:
            print("\n\n用户中断")
            break
    
    print("\n退出交互模式")


def step_by_step_tuning(tuner):
    """分步调参向导"""
    print("\n" + "="*50)
    print("PID分步调参向导")
    print("="*50)
    print("\n建议的调参步骤：")
    print("1. 先调X轴")
    print("2. 然后调Y轴")
    print("3. 每个轴按 Kp → Kd → Ki 的顺序调整\n")
    
    for axis in ['X', 'Y']:
        print(f"\n{'='*50}")
        print(f"开始调整{axis}轴")
        print(f"{'='*50}")
        
        # 调整Kp
        print(f"\n步骤1: 调整{axis}轴 Kp (比例系数)")
        print("说明: 控制响应速度，从小到大调整，直到响应满意但不震荡")
        print("建议范围: 0.5 ~ 3.0")
        
        kp_values = [0.5, 1.0, 1.5, 2.0, 2.5]
        print(f"建议测试值: {kp_values}")
        
        kp = float(input(f"输入{axis}轴 Kp 值: ").strip() or "1.0")
        tuner.set_param(axis, 'P', kp)
        
        input("\n观察效果，按回车继续...")
        
        # 调整Kd
        print(f"\n步骤2: 调整{axis}轴 Kd (微分系数)")
        print("说明: 减少超调和震荡")
        print("建议范围: 0.1 ~ 2.0")
        
        kd_values = [0.3, 0.5, 0.8, 1.0, 1.5]
        print(f"建议测试值: {kd_values}")
        
        kd = float(input(f"输入{axis}轴 Kd 值: ").strip() or "0.5")
        tuner.set_param(axis, 'D', kd)
        
        input("\n观察效果，按回车继续...")
        
        # 调整Ki
        print(f"\n步骤3: 调整{axis}轴 Ki (积分系数)")
        print("说明: 消除稳态误差，从很小的值开始")
        print("建议范围: 0.001 ~ 0.1")
        
        ki_values = [0.001, 0.005, 0.01, 0.02, 0.05]
        print(f"建议测试值: {ki_values}")
        
        ki = float(input(f"输入{axis}轴 Ki 值: ").strip() or "0.01")
        tuner.set_param(axis, 'I', ki)
        
        print(f"\n{axis}轴调整完成！")
        input("按回车继续下一个轴..." if axis == 'X' else "按回车完成调参...")
    
    print("\n" + "="*50)
    print("分步调参完成！")
    print("="*50)
    tuner.show_current_params()


def quick_adjust(tuner):
    """快速调整模式"""
    print("\n" + "="*50)
    print("快速调整模式")
    print("="*50)
    
    print("\n当前问题:")
    print("1 - 响应太慢")
    print("2 - 震荡不稳定")
    print("3 - 超调严重")
    print("4 - 有稳态误差")
    print("5 - 对噪声敏感")
    
    choice = input("\n选择问题 (1-5): ").strip()
    
    suggestions = {
        '1': ("增大Kp", lambda t, a: t.set_param(a, 'P', t.current_params[a]['P'] * 1.5)),
        '2': ("减小Kp, 增大Kd", lambda t, a: (
            t.set_param(a, 'P', t.current_params[a]['P'] * 0.7),
            t.set_param(a, 'D', t.current_params[a]['D'] * 1.5)
        )),
        '3': ("增大Kd", lambda t, a: t.set_param(a, 'D', t.current_params[a]['D'] * 1.5)),
        '4': ("增大Ki", lambda t, a: t.set_param(a, 'I', t.current_params[a]['I'] * 2)),
        '5': ("减小Kd", lambda t, a: t.set_param(a, 'D', t.current_params[a]['D'] * 0.7)),
    }
    
    if choice in suggestions:
        desc, adjust_func = suggestions[choice]
        print(f"\n建议: {desc}")
        
        axis = input("选择轴 (X/Y 或 B=两者): ").strip().upper()
        
        if axis == 'B':
            axes = ['X', 'Y']
        elif axis in ['X', 'Y']:
            axes = [axis]
        else:
            print("无效选择")
            return
        
        for a in axes:
            adjust_func(tuner, a)
        
        print("\n调整完成")


def main():
    """主函数"""
    print("="*50)
    print("PID参数动态调整工具")
    print("="*50)
    
    # 输入串口号
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = input("\n请输入串口号 (例如 COM6): ").strip()
        if not port:
            port = "COM6"
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
        
        # 创建调参器
        tuner = PIDTuner(ser)
        
        # 主菜单
        while True:
            print("\n" + "="*50)
            print("主菜单")
            print("="*50)
            print("1 - 预设配置方案")
            print("2 - 交互式调参")
            print("3 - 分步调参向导")
            print("4 - 快速调整")
            print("5 - 显示当前参数")
            print("6 - 恢复默认参数")
            print("q - 退出")
            print("="*50)
            
            choice = input("\n请选择: ").strip().lower()
            
            if choice == '1':
                preset_configs(tuner)
            elif choice == '2':
                interactive_tuning(tuner)
            elif choice == '3':
                step_by_step_tuning(tuner)
            elif choice == '4':
                quick_adjust(tuner)
            elif choice == '5':
                tuner.show_current_params()
            elif choice == '6':
                tuner.reset_to_default()
            elif choice == 'q':
                break
            else:
                print("无效选项")
        
        # 关闭串口
        ser.close()
        print("\n✓ 串口已关闭")
        
    except serial.SerialException as e:
        print(f"\n✗ 串口错误: {e}")
        sys.exit(1)
    
    except KeyboardInterrupt:
        print("\n\n程序被用户中断")
        try:
            ser.close()
        except:
            pass
    
    except Exception as e:
        print(f"\n✗ 发生错误: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
