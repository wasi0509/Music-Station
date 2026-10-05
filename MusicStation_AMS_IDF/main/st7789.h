#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define TFT_CS     GPIO_NUM_10
#define TFT_DC     GPIO_NUM_9
#define TFT_RST    GPIO_NUM_8
#define TFT_SCLK   GPIO_NUM_12
#define TFT_MOSI   GPIO_NUM_11
#define TFT_BL     GPIO_NUM_7

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

esp_err_t st7789_init(void);

void st7789_fill_screen(uint16_t color);

void st7789_draw_test(void);

/* Basic drawing */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

void st7789_fill_rect(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t color
);

/* Simple 5x7 text */
void st7789_draw_char(
    uint16_t x,
    uint16_t y,
    char c,
    uint16_t color,
    uint8_t scale
);

void st7789_draw_text(
    uint16_t x,
    uint16_t y,
    const char *text,
    uint16_t color,
    uint8_t scale
);

void st7789_draw_rgb565(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *data
);


#endif