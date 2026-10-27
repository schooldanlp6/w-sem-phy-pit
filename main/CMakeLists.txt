idf_component_register(SRCS "main.c" "ld2450.c" "display.c" "oled.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_driver_uart esp_driver_i2c esp_lcd esp_timer)
