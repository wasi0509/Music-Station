idf_component_register(
    SRCS
        "main.c"
        "ble_ams.c"
        "st7789.c"
        "music_display.c"
        "artwork.c"
        "u8g2_st7789_test.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        bt
        nvs_flash
        driver
        u8g2
        esp_jpeg

    PRIV_REQUIRES
        esp_http_client
        esp-tls
)