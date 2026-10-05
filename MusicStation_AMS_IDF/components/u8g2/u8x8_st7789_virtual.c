#include "u8x8.h"

static const u8x8_display_info_t st7789_virtual_display_info =
{
    /* chip_enable_level */        0,
    /* chip_disable_level */       1,

    /* post_chip_enable_wait_ns */ 0,
    /* pre_chip_disable_wait_ns */ 0,
    /* reset_pulse_width_ms */     0,
    /* post_reset_wait_ms */       0,

    /* sda_setup_time_ns */        0,
    /* sck_pulse_width_ns */       0,
    /* sck_clock_hz */             4000000UL,
    /* spi_mode */                 0,

    /* i2c_bus_clock_100kHz */     4,

    /* data_setup_time_ns */       0,
    /* write_pulse_width_ns */     0,

    /* tile_width */               30,
    /* tile_height */              40,

    /* default_x_offset */         0,
    /* flipmode_x_offset */        0,

    /* pixel_width */              240,
    /* pixel_height */             320
};

uint8_t u8x8_d_st7789_virtual(
    u8x8_t *u8x8,
    uint8_t msg,
    uint8_t arg_int,
    void *arg_ptr
)
{
    (void)arg_int;
    (void)arg_ptr;

    switch (msg)
    {
        case U8X8_MSG_DISPLAY_SETUP_MEMORY:
            u8x8_d_helper_display_setup_memory(
                u8x8,
                &st7789_virtual_display_info
            );
            break;

        case U8X8_MSG_DISPLAY_INIT:
            u8x8_d_helper_display_init(u8x8);
            break;

        default:
            break;
    }

    return 1;
}