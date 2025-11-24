//
// Created by pickaxehit on 24-9-27.
// rewreat by MY_ELY_EGO
// 适配STM32F411CEU6 + I2C1 + 32M主频 + 0.91寸(128×32)OLED
//

#include "stm32f4xx_hal.h"
#include "i2c.h"        // 必须包含CubeMX生成的I2C1初始化头文件
#include "ssd1306.h"
#include "font_5_7.h"
#include "font_12_24.h"
#include <stdarg.h>     // 标准可变参数支持（Keil兼容）
#include <string.h>     // 字符串/内存操作函数
#include <stdio.h>      // vsnprintf函数支持

// -------------------------- 硬件配置（根据实际情况调整）--------------------------
#define OLED_I2C_HANDLE  &hi2c1       // I2C句柄（CubeMX配置的I2C1）
#define OLED_I2C_ADDR    0x78         // OLED I2C设备地址（8位，7位地址0x3C左移1位）
#define OLED_WIDTH       128          // 屏幕宽度
#define OLED_HEIGHT      32           // 屏幕高度（0.91寸标准）
#define OLED_BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)  // 缓冲区大小：128×32/8=512字节
// --------------------------------------------------------------------------------

// OLED显示缓冲区（1字节对应8个像素点，按"页"存储）
uint8_t ssd1306_buffer[OLED_BUFFER_SIZE];

// OLED初始化指令集（0.91寸128×32专用，优化显示方向）
uint8_t ssd1306_init_command[] = {
    0xAE,        // 关闭显示
    0x00,        // 设置列地址低位（0x00~0x0F）
    0x10,        // 设置列地址高位（0x10~0x1F）
    0x40,        // 设置显示起始行（0x40=第0行）
    0x81,        // 对比度控制使能
    0xCF,        // 对比度值（0x00~0xFF，越大越亮）
    0xA1,        // 段重映射（0xA0=正常，0xA1=左右翻转）
    0xC8,        // COM扫描方向（0xC0=上下翻转，0xC8=正常，重点修复！）
    0xA6,        // 正常显示（0xA6=正常，0xA7=反色）
    0xA8, 0x1F,  // 设置多路复用率（0x1F=32行，匹配128×32分辨率）
    0xD3, 0x00,  // 设置显示偏移（0x00=无偏移）
    0xD5, 0x80,  // 设置振荡频率（0x80=默认）
    0xD9, 0xF1,  // 设置预充电周期（0xF1=优化I2C模式）
    0xDA, 0x02,  // 设置COM引脚配置（0x02=序列配置，适配128×32）
    0xDB, 0x40,  // 设置VCOMH输出电平（0x40=默认）
    0x20, 0x00,  // 设置内存寻址模式（0x00=水平寻址，匹配缓冲区）
    0x8D, 0x14,  // 启用电荷泵（0x14=开启，必须开启否则无显示）
    0xA4,        // 显示全部点阵（0xA4=正常，0xA5=强制点亮所有点）
    0xAF         // 开启显示
};

/**
 * @brief  I2C内存写函数（封装HAL库，阻塞式传输）
 * @param  dev_addr: 设备地址（8位）
 * @param  reg_addr: 寄存器/命令地址（8位）
 * @param  data: 要写入的数据指针
 * @param  size: 数据长度
 * @retval HAL_StatusTypeDef: 传输状态
 */
HAL_StatusTypeDef i2c_mem_write(uint16_t dev_addr, uint16_t reg_addr, uint8_t *data, uint16_t size) {
    return HAL_I2C_Mem_Write(OLED_I2C_HANDLE, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, 
                             data, size, HAL_MAX_DELAY);  // 超时时间设为最大，确保传输完成
}

/**
 * @brief  I2C内存读函数（预留，当前驱动未用到）
 * @param  dev_addr: 设备地址（8位）
 * @param  reg_addr: 寄存器/命令地址（8位）
 * @param  data: 读取数据存储指针
 * @param  size: 读取数据长度
 * @retval HAL_StatusTypeDef: 传输状态
 */
HAL_StatusTypeDef i2c_mem_read(uint16_t dev_addr, uint16_t reg_addr, uint8_t *data, uint16_t size) {
    return HAL_I2C_Mem_Read(OLED_I2C_HANDLE, dev_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, 
                            data, size, HAL_MAX_DELAY);
}

/**
 * @brief  OLED初始化函数
 * @param  无
 * @retval 无
 */
void ssd1306_init(void) {
    HAL_Delay(100);  // 上电延时，确保OLED稳定
    // 发送初始化指令（reg_addr=0x00表示写入命令）
    i2c_mem_write(OLED_I2C_ADDR, 0x00, ssd1306_init_command, sizeof(ssd1306_init_command));
    HAL_Delay(10);   // 初始化完成延时
    ssd1306_clear(0);  // 清屏（默认黑色背景）
}

/**
 * @brief  刷新缓冲区数据到OLED屏幕
 * @param  无
 * @retval 无
 */
void ssd1306_refresh(void) {
    // 发送缓冲区数据（reg_addr=0x40表示写入数据）
    i2c_mem_write(OLED_I2C_ADDR, 0x40, ssd1306_buffer, sizeof(ssd1306_buffer));
}

/**
 * @brief  清屏函数
 * @param  color: 0=黑色（清屏），1=白色（全屏点亮）
 * @retval 无
 */
void ssd1306_clear(uint8_t color) {
    memset(ssd1306_buffer, color ? 0xFF : 0x00, sizeof(ssd1306_buffer));
}

/**
 * @brief  清屏指定页（OLED分为4页，每页8行：0~3页）
 * @param  page_num: 页号（0~3）
 * @param  color: 0=黑色，1=白色
 * @retval 无
 */
void ssd1306_clear_page(uint8_t page_num, uint8_t color) {
    if (page_num >= 4) return;  // 边界检查（0.91寸只有4页）
    uint16_t start_idx = page_num * OLED_WIDTH;  // 对应页的缓冲区起始地址
    memset(&ssd1306_buffer[start_idx], color ? 0xFF : 0x00, OLED_WIDTH);
}

/**
 * @brief  内部函数：绘制5×7字体的单个像素列（私有，不对外暴露）
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  c: 要绘制的字符
 * @param  color: 0=黑色，1=白色
 * @param  col_idx: 字符的列索引（0~4）
 * @retval 无
 */
static void ssd1306_char_5_7_mem_cp(uint8_t x, uint8_t y, char c, uint8_t color, uint8_t col_idx) {
    if (y >= OLED_HEIGHT || x + col_idx >= OLED_WIDTH) return;  // 边界检查

    uint8_t font_data = font_5_7_data[(c - 32) * 5 + col_idx];  // 获取5×7字体数据（ASCII码从32开始）
    uint8_t page = y / 8;  // 当前字符所在的页
    uint8_t y_offset = y % 8;  // 页内垂直偏移

    if (color == 0) {  // 黑色：清除对应位
        ssd1306_buffer[page * OLED_WIDTH + x + col_idx] &= ~(font_data << y_offset);
        // 跨页处理（如果字符高度超过当前页）
        if (page + 1 < 4) {
            ssd1306_buffer[(page + 1) * OLED_WIDTH + x + col_idx] &= ~(font_data >> (8 - y_offset));
        }
    } else {  // 白色：设置对应位
        ssd1306_buffer[page * OLED_WIDTH + x + col_idx] |= (font_data << y_offset);
        if (page + 1 < 4) {
            ssd1306_buffer[(page + 1) * OLED_WIDTH + x + col_idx] |= (font_data >> (8 - y_offset));
        }
    }
}

/**
 * @brief  绘制单个5×7字体字符
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  c: 要绘制的ASCII字符（32~126）
 * @param  color: 0=黑色，1=白色
 * @retval 无
 */
void ssd1306_draw_char_5_7(uint8_t x, uint8_t y, char c, uint8_t color) {
    if (y >= OLED_HEIGHT || c < 32 || c > 126) return;  // 边界+字符有效性检查

    // 绘制字符的5列数据（每列1字节）
    for (uint8_t col = 0; col < 5; col++) {
        ssd1306_char_5_7_mem_cp(x, y, c, color, col);
    }
}

/**
 * @brief  绘制5×7字体字符串
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  str: 要绘制的字符串指针（ASCII）
 * @param  color: 0=黑色，1=白色
 * @param  auto_wrap: 0=超出屏幕不显示，1=自动换行
 * @retval 无
 */
void ssd1306_draw_string_5_7(uint8_t x, uint8_t y, const char *str, uint8_t color, uint8_t auto_wrap) {
    if (y >= OLED_HEIGHT || str == NULL) return;

    uint8_t curr_x = x;
    uint8_t curr_y = y;
    uint8_t char_width = 5;  // 5×7字体宽度（含1像素间距）
    uint8_t line_height = 8; // 行高（5×7字体实际高度7，预留1像素间距）

    while (*str != '\0') {
        if (*str == '\n') {  // 换行符处理
            curr_x = 0;
            curr_y += line_height;
            str++;
            continue;
        }

        // 超出屏幕宽度处理
        if (curr_x + char_width > OLED_WIDTH) {
            if (auto_wrap) {  // 自动换行
                curr_x = 0;
                curr_y += line_height;
                if (curr_y >= OLED_HEIGHT) return;  // 超出屏幕高度，停止绘制
            } else {  // 不换行，直接停止
                return;
            }
        }

        // 绘制单个字符
        ssd1306_draw_char_5_7(curr_x, curr_y, *str, color);
        curr_x += char_width + 1;  // 字符间距1像素
        str++;
    }
}

/**
 * @brief  内部函数：绘制12×24字体的单个像素列（私有，不对外暴露）
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  font_col_data: 字体列数据指针（3字节=24位）
 * @param  color: 0=黑色，1=白色
 * @param  col_idx: 字符的列索引（0~11）
 * @retval 无
 */
static void ssd1306_char_12_24_mem_cp(uint8_t x, uint8_t y, const uint8_t *font_col_data, uint8_t color, uint8_t col_idx) {
    if (y + 24 > OLED_HEIGHT || x + col_idx >= OLED_WIDTH) return;  // 边界检查（12×24字体高度24）

    uint8_t start_page = y / 8;  // 起始页
    uint8_t y_offset = y % 8;    // 页内垂直偏移
    uint16_t buf_idx = start_page * OLED_WIDTH + x + col_idx;  // 缓冲区索引

    // 组合3字节为24位数据（对应24个像素点）
    uint32_t char_data = (uint32_t)font_col_data[0] | 
                         (uint32_t)font_col_data[1] << 8 | 
                         (uint32_t)font_col_data[2] << 16;
    uint32_t shifted_data = char_data << y_offset;  // 垂直偏移调整

    // 遍历4个页（12×24字体最多跨4页）
    for (uint8_t page = 0; page < 4; page++) {
        if (start_page + page >= 4) break;  // 超出最大页号
        uint8_t byte_val = (uint8_t)((shifted_data >> (page * 8)) & 0xFF);  // 当前页的8位数据
        if (byte_val == 0) continue;  // 无数据则跳过

        buf_idx = (start_page + page) * OLED_WIDTH + x + col_idx;
        if (color) {
            ssd1306_buffer[buf_idx] |= byte_val;  // 白色：置1
        } else {
            ssd1306_buffer[buf_idx] &= ~byte_val; // 黑色：清0
        }
    }
}

/**
 * @brief  绘制单个12×24字体字符
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  c: 要绘制的ASCII字符（32~126）
 * @param  color: 0=黑色，1=白色
 * @retval 无
 */
void ssd1306_draw_char_12_24(uint8_t x, uint8_t y, char c, uint8_t color) {
    const uint8_t char_width = 12;   // 12×24字体宽度
    const uint8_t bytes_per_col = 3; // 每列3字节（24位）
//    const uint16_t chars_total = 95; // ASCII字符总数（32~126）

    // 边界+字符有效性检查
    if (y + 24 > OLED_HEIGHT || x + char_width > OLED_WIDTH || c < 32 || c > 126) {
        return;
    }

    // 计算字体数据偏移（每个字符占12列×3字节=36字节）
    uint16_t char_offset = (c - 32) * char_width * bytes_per_col;
    const uint8_t *font_data = &font_12_24_data[char_offset];

    // 绘制字符的12列数据
    for (uint8_t col = 0; col < char_width; col++) {
        const uint8_t *col_data = &font_data[col * bytes_per_col];
        ssd1306_char_12_24_mem_cp(x, y, col_data, color, col);
    }
}

/**
 * @brief  绘制12×24字体字符串
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  str: 要绘制的字符串指针（ASCII）
 * @param  color: 0=黑色，1=白色
 * @param  auto_wrap: 0=超出屏幕不显示，1=自动换行
 * @retval 无
 */
void ssd1306_draw_string_12_24(uint8_t x, uint8_t y, const char *str, uint8_t color, uint8_t auto_wrap) {
    if (y + 24 > OLED_HEIGHT || str == NULL) return;

    const uint8_t char_width = 12;    // 字符宽度
    const uint8_t char_spacing = 1;   // 字符间距
    const uint8_t line_height = 24;   // 行高（12×24字体高度24，修复原8的错误！）
    const uint8_t max_x = OLED_WIDTH - char_width;  // 最大X坐标（确保字符完整显示）

    uint8_t curr_x = x;
    uint8_t curr_y = y;

    while (*str != '\0') {
        if (*str == '\n') {  // 换行符处理
            curr_x = 0;
            curr_y += line_height;
            str++;
            continue;
        }

        // 超出屏幕宽度处理
        if (curr_x > max_x) {
            if (auto_wrap) {  // 自动换行
                curr_x = 0;
                curr_y += line_height;
                if (curr_y + 24 > OLED_HEIGHT) return;  // 超出高度停止
            } else {
                return;
            }
        }

        // 绘制单个字符
        ssd1306_draw_char_12_24(curr_x, curr_y, *str, color);
        curr_x += char_width + char_spacing;  // 移动到下一个字符
        str++;
    }
}

/**
 * @brief  格式化打印函数（支持5×7/12×24字体切换）
 * @param  x: 起始X坐标
 * @param  y: 起始Y坐标
 * @param  font_size: 字体大小（1=5×7，2=12×24）
 * @param  color: 0=黑色，1=白色
 * @param  auto_wrap: 0=不换行，1=自动换行
 * @param  fmt: 格式化字符串
 * @param  ...: 可变参数
 * @retval int: 格式化字符数
 */
int ssd1306_printf(uint8_t x, uint8_t y, uint8_t font_size, uint8_t color, uint8_t auto_wrap, const char *fmt, ...) {
    char buf[64];  // 格式化缓冲区（足够存储常用字符串）
    va_list args;  // 标准可变参数列表（Keil兼容）

    // 初始化可变参数
    va_start(args, fmt);
    // 格式化字符串（避免缓冲区溢出，限制长度为sizeof(buf)-1）
    int ret = vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);  // 释放可变参数

    // 添加字符串结束符（防止vsnprintf未自动添加）
    buf[sizeof(buf) - 1] = '\0';

    // 根据字体大小绘制字符串
    if (font_size == 1) {
        ssd1306_draw_string_5_7(x, y, buf, color, auto_wrap);
    } else if (font_size == 2) {
        ssd1306_draw_string_12_24(x, y, buf, color, auto_wrap);
    }

    return ret;
}

// 以下Unicode相关函数未使用，注释掉避免编译警告
#if 0
__weak void unifont_get_glyphs(uint32_t code, uint8_t *buffer, unifont_prop_t *prop) {
    return;
}

uint8_t ssd1306_draw_char_unifont(uint8_t x, uint8_t y, uint32_t code, uint8_t color) {
    if (y >= 16)
        return x;

    uint8_t unifont_buf[32];
    unifont_prop_t prop;
    unifont_get_glyphs(code, unifont_buf, &prop);

    if (prop.comb) {
        x -= prop.comb_off;
    }

    if (color == 0) {
        for (uint8_t i = 0; i < 16; i++, y++) {
            uint8_t buf_pos = i << prop.width;
            uint8_t page_y = y / 8;
            for (int j = 8 - 1; j >= 0; --j) {
                if (x + 8 - j > 128) {
                    continue;
                }
                ssd1306_buffer[page_y * 128 + x + 8 - j - 1] &= ~((unifont_buf[buf_pos] >> j & 0x01) << y % 8);
                if (prop.width) {
                    if (x + 16 - j > 128) {
                        continue;
                    }
                    ssd1306_buffer[page_y * 128 + x - j + 16 - 1] &= ~((unifont_buf[buf_pos + 1] >> j & 0x01) << y % 8);
                }
            }
        }
    } else {
        for (uint8_t i = 0; i < 16; i++, y++) {
            uint8_t buf_pos = i << prop.width;
            uint8_t page_y = y / 8;
            for (int j = 8 - 1; j >= 0; --j) {
                if (x + 8 - j > 128) {
                    continue;
                }
                ssd1306_buffer[page_y * 128 + x + 8 - j - 1] |= (unifont_buf[buf_pos] >> j & 0x01) << y % 8;
                if (prop.width) {
                    if (x + 16 - j > 128) {
                        continue;
                    }
                    ssd1306_buffer[page_y * 128 + x - j + 16 - 1] |= (unifont_buf[buf_pos + 1] >> j & 0x01) << y % 8;
                }
            }
        }
    }
    if (prop.comb) {
        return x + prop.comb_off;
    }
    if (prop.width) {
        return x + 16;
    }
    return x + 8;
}
#endif
