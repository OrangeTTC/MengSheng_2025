//
// Created by pickaxehit on 24-9-27.
//

#ifndef OLED_H
#define OLED_H
#include <stddef.h>
#include <stdint.h>

extern uint8_t ssd1306_buffer[];

typedef struct {
    uint8_t width : 1;
    uint8_t comb : 1;
    uint8_t comb_off : 6;
} unifont_prop_t;

void ssd1306_init(void);
void ssd1306_refresh(void);
void ssd1306_clear(uint8_t color);
void ssd1306_draw_char_5_7(uint8_t x, uint8_t y, char c, uint8_t color);
void ssd1306_draw_string_5_7(uint8_t x, uint8_t y, const char *s, uint8_t color, uint8_t boundary);
void ssd1306_draw_char_12_24(uint8_t x, uint8_t y, char c, uint8_t color);
void ssd1306_draw_string_12_24(uint8_t x, uint8_t y, const char *s, uint8_t color, uint8_t boundary);
int ssd1306_printf(uint8_t x, uint8_t y, uint8_t font_size, uint8_t color, uint8_t auto_wrap, const char *fmt, ...);
uint8_t ssd1306_draw_char_unifont(uint8_t x, uint8_t y, uint32_t code, uint8_t color);

void ssd1306_clear_page(uint8_t page_num, uint8_t color);

#endif// OLED_H

