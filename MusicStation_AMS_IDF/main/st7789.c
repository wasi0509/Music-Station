#include "st7789.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ST7789";

static spi_device_handle_t tft_spi;

/*
 * Send a command to the ST7789
 */
static void st7789_write_command(uint8_t command)
{
    gpio_set_level(TFT_DC, 0);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = 8;
    t.tx_buffer = &command;

    ESP_ERROR_CHECK(spi_device_transmit(tft_spi, &t));
}

/*
 * Send data to the ST7789
 */
static void st7789_write_data(const uint8_t *data, size_t length)
{
    gpio_set_level(TFT_DC, 1);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = length * 8;
    t.tx_buffer = data;

    ESP_ERROR_CHECK(spi_device_transmit(tft_spi, &t));
}

/*
 * Send one byte
 */
static void st7789_write_data_byte(uint8_t data)
{
    st7789_write_data(&data, 1);
}

/*
 * Set drawing window
 */
static void st7789_set_window(
    uint16_t x0,
    uint16_t y0,
    uint16_t x1,
    uint16_t y1)
{
    uint8_t data[4];

    // Column address
    st7789_write_command(0x2A);

    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;

    st7789_write_data(data, 4);

    // Row address
    st7789_write_command(0x2B);

    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;

    st7789_write_data(data, 4);

    // Memory write
    st7789_write_command(0x2C);
}

/*
 * Fill the entire screen
 *
 * RGB565 color
 */
void st7789_fill_screen(uint16_t color)
{
    st7789_set_window(
        0,
        0,
        TFT_WIDTH - 1,
        TFT_HEIGHT - 1
    );

    // Build a reasonably sized buffer.
    // 240 pixels × 2 bytes = 480 bytes.
    static uint8_t line_buffer[TFT_WIDTH * 2];

    for (int x = 0; x < TFT_WIDTH; x++) {
        line_buffer[x * 2]     = color >> 8;
        line_buffer[x * 2 + 1] = color & 0xFF;
    }

    for (int y = 0; y < TFT_HEIGHT; y++) {
        st7789_write_data(
            line_buffer,
            sizeof(line_buffer)
        );
    }
}

/*
 * Initialize display
 */
esp_err_t st7789_init(void)
{
    ESP_LOGI(TAG, "Initializing ST7789...");

    // --------------------------------------------------------
    // GPIO
    // --------------------------------------------------------

    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << TFT_DC) |
            (1ULL << TFT_RST) |
            (1ULL << TFT_BL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Backlight ON
    gpio_set_level(TFT_BL, 1);

    // CS is also output
    gpio_set_direction(TFT_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(TFT_CS, 1);

    // --------------------------------------------------------
    // Hardware reset
    // --------------------------------------------------------

    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(TFT_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(150));

    gpio_set_level(TFT_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    // --------------------------------------------------------
    // SPI bus
    // --------------------------------------------------------

    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TFT_WIDTH * 2
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(
            SPI2_HOST,
            &buscfg,
            SPI_DMA_CH_AUTO
        )
    );

    // --------------------------------------------------------
    // SPI device
    // --------------------------------------------------------

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = TFT_CS,
        .queue_size = 1
    };

    ESP_ERROR_CHECK(
        spi_bus_add_device(
            SPI2_HOST,
            &devcfg,
            &tft_spi
        )
    );

    // --------------------------------------------------------
    // ST7789 initialization
    // --------------------------------------------------------

    // Software reset
    st7789_write_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    st7789_write_command(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // Pixel format: 16-bit RGB565
    st7789_write_command(0x3A);
    st7789_write_data_byte(0x55);

    // Memory access control
    st7789_write_command(0x36);
    st7789_write_data_byte(0x00);

    // Inversion ON
    st7789_write_command(0x21);

    // Normal display mode
    st7789_write_command(0x13);

    // Display ON
    st7789_write_command(0x29);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "ST7789 initialized.");

    return ESP_OK;
}

/*
 * Simple display test
 */
void st7789_draw_test(void)
{
    ESP_LOGI(TAG, "Drawing RED...");
    st7789_fill_screen(0xF800);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Drawing GREEN...");
    st7789_fill_screen(0x07E0);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Drawing BLUE...");
    st7789_fill_screen(0x001F);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Drawing WHITE...");
    st7789_fill_screen(0xFFFF);
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "Drawing BLACK...");
    st7789_fill_screen(0x0000);

    ESP_LOGI(TAG, "TFT test complete.");
}

/*
 * Draw one pixel
 */
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    st7789_set_window(x, y, x, y);

    uint8_t data[2];
    data[0] = color >> 8;
    data[1] = color & 0xFF;

    st7789_write_data(data, 2);
}


/*
 * Fill rectangle
 */
void st7789_fill_rect(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    if (x + w > TFT_WIDTH) {
        w = TFT_WIDTH - x;
    }

    if (y + h > TFT_HEIGHT) {
        h = TFT_HEIGHT - y;
    }

    st7789_set_window(
        x,
        y,
        x + w - 1,
        y + h - 1
    );

    static uint8_t buffer[240 * 2];

    for (uint16_t i = 0; i < w; i++) {
        buffer[i * 2]     = color >> 8;
        buffer[i * 2 + 1] = color & 0xFF;
    }

    for (uint16_t row = 0; row < h; row++) {
        st7789_write_data(buffer, w * 2);
    }
}


/*
 * Tiny 5x7 ASCII font
 *
 * Each character is 5 columns wide.
 *
 * Supported:
 * Space, punctuation, numbers, A-Z
 */

static const uint8_t font5x7[][5] = {

    /* 0 - Space */
    {0x00,0x00,0x00,0x00,0x00},

    /* 1 - ! */
    {0x00,0x00,0x5F,0x00,0x00},

    /* 2 - " */
    {0x00,0x07,0x00,0x07,0x00},

    /* 3 - ' */
    {0x00,0x03,0x00,0x00,0x00},

    /* 4 - ( */
    {0x00,0x1C,0x22,0x41,0x00},

    /* 5 - ) */
    {0x00,0x41,0x22,0x1C,0x00},

    /* 6 - , */
    {0x00,0x40,0x30,0x00,0x00},

    /* 7 - - */
    {0x08,0x08,0x08,0x08,0x08},

    /* 8 - . */
    {0x00,0x60,0x60,0x00,0x00},

    /* 9 - / */
    {0x20,0x10,0x08,0x04,0x02},

    /* 10 - : */
    {0x00,0x36,0x36,0x00,0x00},

    /* 11 - ; */
    {0x00,0x56,0x36,0x00,0x00},

    /* 12 - ? */
    {0x02,0x01,0x51,0x09,0x06},

    /* 13 - + */
    {0x08,0x08,0x3E,0x08,0x08},

    /* 14 - & */
    {0x36,0x49,0x55,0x22,0x50},

    /* 15 - 0 */
    {0x3E,0x51,0x49,0x45,0x3E},

    /* 16 - 1 */
    {0x00,0x42,0x7F,0x40,0x00},

    /* 17 - 2 */
    {0x42,0x61,0x51,0x49,0x46},

    /* 18 - 3 */
    {0x21,0x41,0x45,0x4B,0x31},

    /* 19 - 4 */
    {0x18,0x14,0x12,0x7F,0x10},

    /* 20 - 5 */
    {0x27,0x45,0x45,0x45,0x39},

    /* 21 - 6 */
    {0x3C,0x4A,0x49,0x49,0x30},

    /* 22 - 7 */
    {0x01,0x71,0x09,0x05,0x03},

    /* 23 - 8 */
    {0x36,0x49,0x49,0x49,0x36},

    /* 24 - 9 */
    {0x06,0x49,0x49,0x29,0x1E},

    /* 25 - A */
    {0x7E,0x11,0x11,0x11,0x7E},

    /* 26 - B */
    {0x7F,0x49,0x49,0x49,0x36},

    /* 27 - C */
    {0x3E,0x41,0x41,0x41,0x22},

    /* 28 - D */
    {0x7F,0x41,0x41,0x22,0x1C},

    /* 29 - E */
    {0x7F,0x49,0x49,0x49,0x41},

    /* 30 - F */
    {0x7F,0x09,0x09,0x09,0x01},

    /* 31 - G */
    {0x3E,0x41,0x49,0x49,0x7A},

    /* 32 - H */
    {0x7F,0x08,0x08,0x08,0x7F},

    /* 33 - I */
    {0x00,0x41,0x7F,0x41,0x00},

    /* 34 - J */
    {0x20,0x40,0x41,0x3F,0x01},

    /* 35 - K */
    {0x7F,0x08,0x14,0x22,0x41},

    /* 36 - L */
    {0x7F,0x40,0x40,0x40,0x40},

    /* 37 - M */
    {0x7F,0x02,0x0C,0x02,0x7F},

    /* 38 - N */
    {0x7F,0x04,0x08,0x10,0x7F},

    /* 39 - O */
    {0x3E,0x41,0x41,0x41,0x3E},

    /* 40 - P */
    {0x7F,0x09,0x09,0x09,0x06},

    /* 41 - Q */
    {0x3E,0x41,0x51,0x21,0x5E},

    /* 42 - R */
    {0x7F,0x09,0x19,0x29,0x46},

    /* 43 - S */
    {0x46,0x49,0x49,0x49,0x31},

    /* 44 - T */
    {0x01,0x01,0x7F,0x01,0x01},

    /* 45 - U */
    {0x3F,0x40,0x40,0x40,0x3F},

    /* 46 - V */
    {0x1F,0x20,0x40,0x20,0x1F},

    /* 47 - W */
    {0x3F,0x40,0x38,0x40,0x3F},

    /* 48 - X */
    {0x63,0x14,0x08,0x14,0x63},

    /* 49 - Y */
    {0x07,0x08,0x70,0x08,0x07},

    /* 50 - Z */
    {0x61,0x51,0x49,0x45,0x43}
};

/*
 * Lowercase 5x7 font
 *
 * a-z
 */
static const uint8_t lowercase_font5x7[][5] = {

    /* a */
    {0x20,0x54,0x54,0x54,0x78},

    /* b */
    {0x7F,0x48,0x44,0x44,0x38},

    /* c */
    {0x38,0x44,0x44,0x44,0x20},

    /* d */
    {0x38,0x44,0x44,0x48,0x7F},

    /* e */
    {0x38,0x54,0x54,0x54,0x18},

    /* f */
    {0x08,0x7E,0x09,0x01,0x02},

    /* g */
    {0x0C,0x52,0x52,0x52,0x3E},

    /* h */
    {0x7F,0x08,0x04,0x04,0x78},

    /* i */
    {0x00,0x44,0x7D,0x40,0x00},

    /* j */
    {0x20,0x40,0x44,0x3D,0x00},

    /* k */
    {0x7F,0x10,0x28,0x44,0x00},

    /* l */
    {0x00,0x41,0x7F,0x40,0x00},

    /* m */
    {0x7C,0x04,0x18,0x04,0x78},

    /* n */
    {0x7C,0x08,0x04,0x04,0x78},

    /* o */
    {0x38,0x44,0x44,0x44,0x38},

    /* p */
    {0x7C,0x14,0x14,0x14,0x08},

    /* q */
    {0x08,0x14,0x14,0x18,0x7C},

    /* r */
    {0x7C,0x08,0x04,0x04,0x08},

    /* s */
    {0x48,0x54,0x54,0x54,0x20},

    /* t */
    {0x04,0x3F,0x44,0x40,0x20},

    /* u */
    {0x3C,0x40,0x40,0x20,0x7C},

    /* v */
    {0x1C,0x20,0x40,0x20,0x1C},

    /* w */
    {0x3C,0x40,0x30,0x40,0x3C},

    /* x */
    {0x44,0x28,0x10,0x28,0x44},

    /* y */
    {0x0C,0x50,0x50,0x50,0x3C},

    /* z */
    {0x44,0x64,0x54,0x4C,0x44}
};

/*
 * Draw one character
 */
void st7789_draw_char(
    uint16_t x,
    uint16_t y,
    char c,
    uint16_t color,
    uint8_t scale)
{
    if (scale == 0) {
        scale = 1;
    }

    const uint8_t *bitmap = NULL;

    if (c == ' ') {
        bitmap = font5x7[0];
    }
    else if (c == '!') {
        bitmap = font5x7[1];
    }
    else if (c == '"') {
        bitmap = font5x7[2];
    }
    else if (c == '\'') {
        bitmap = font5x7[3];
    }
    else if (c == '(') {
        bitmap = font5x7[4];
    }
    else if (c == ')') {
        bitmap = font5x7[5];
    }
    else if (c == ',') {
        bitmap = font5x7[6];
    }
    else if (c == '-') {
        bitmap = font5x7[7];
    }
    else if (c == '.') {
        bitmap = font5x7[8];
    }
    else if (c == '/') {
        bitmap = font5x7[9];
    }
    else if (c == ':') {
        bitmap = font5x7[10];
    }
    else if (c == ';') {
        bitmap = font5x7[11];
    }
    else if (c == '?') {
        bitmap = font5x7[12];
    }
    else if (c == '+') {
        bitmap = font5x7[13];
    }
    else if (c == '&') {
        bitmap = font5x7[14];
    }
    else if (c >= '0' && c <= '9') {
        bitmap = font5x7[15 + (c - '0')];
    }
    else if (c >= 'A' && c <= 'Z') {
        bitmap = font5x7[25 + (c - 'A')];
    }
    else if (c >= 'a' && c <= 'z') {
        // Use lowercase font
        bitmap = lowercase_font5x7[c - 'a'];
    }
    else {
        // Unsupported character
        bitmap = font5x7[0];
    }

    for (int col = 0; col < 5; col++) {

        uint8_t line = bitmap[col];

        for (int row = 0; row < 7; row++) {

            if (line & (1 << row)) {

                st7789_fill_rect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}

/*
 * Draw text
 */
void st7789_draw_text(
    uint16_t x,
    uint16_t y,
    const char *text,
    uint16_t color,
    uint8_t scale)
{
    if (text == NULL) {
        return;
    }

    while (*text) {

        st7789_draw_char(
            x,
            y,
            *text,
            color,
            scale
        );

        x += 6 * scale;

        if (x >= TFT_WIDTH) {
            break;
        }

        text++;
    }
}

/*
 * Draw an RGB565 image.
 */
void st7789_draw_rgb565(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t *data)
{
    if (data == NULL) {
        return;
    }

    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) {
        return;
    }

    if (x + width > TFT_WIDTH) {
        width = TFT_WIDTH - x;
    }

    if (y + height > TFT_HEIGHT) {
        height = TFT_HEIGHT - y;
    }

    st7789_set_window(
        x,
        y,
        x + width - 1,
        y + height - 1
    );

    for (uint16_t row = 0; row < height; row++) {

        const uint8_t *row_data =
            data + (row * width * 2);

        st7789_write_data(
            row_data,
            width * 2
        );
    }
}