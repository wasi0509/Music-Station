#include "u8g2_st7789_test.h"
#include "u8g2.h"
#include "st7789.h"
#include <string.h>
#include <stdint.h>

#define TEST_WIDTH  240
#define TEST_HEIGHT 320

static uint8_t buffer[TEST_WIDTH * TEST_HEIGHT / 8];

void u8g2_st7789_test(void)
{
    u8g2_t u8g2;

    memset(&u8g2, 0, sizeof(u8g2));
    memset(buffer, 0, sizeof(buffer));

    /*
     * Do NOT call u8g2_SetupBuffer().
     * It requires the U8x8 display structure.
     */

    u8g2.tile_buf_ptr = buffer;
    u8g2.tile_buf_height = TEST_HEIGHT / 8;
    u8g2.tile_curr_row = 0;
    u8g2.pixel_buf_width = TEST_WIDTH;
    u8g2.pixel_buf_height = TEST_HEIGHT;

    u8g2_SetFont(
        &u8g2,
        u8g2_font_unifont_t_japanese3
    );

    u8g2_SetFontMode(&u8g2, 1);

    // u8g2_ClearBuffer(&u8g2);

    u8g2_DrawUTF8(
        &u8g2,
        10,
        30,
        "Music Station"
    );

    u8g2_DrawUTF8(
        &u8g2,
        10,
        60,
        "中文测试"
    );

    u8g2_DrawUTF8(
        &u8g2,
        10,
        90,
        "日本語テスト"
    );

    u8g2_DrawUTF8(
        &u8g2,
        10,
        120,
        "한국어 테스트"
    );

    for (int y = 0; y < TEST_HEIGHT; y++) {
        for (int x = 0; x < TEST_WIDTH; x++) {

            int byte_index = (y / 8) * TEST_WIDTH + x;
            int bit_index = y % 8;

            if (buffer[byte_index] & (1 << bit_index)) {
                st7789_draw_pixel(x, y, 0xFFFF);
            }
        }
    }
}