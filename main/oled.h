/* SSD1306 I2C OLED driver glue (ESP-IDF esp_lcd). */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Returns false if no display answers on the configured I2C address. */
bool oled_init(void);

/* Send a DISP_FB_SIZE frame buffer (see display.h) to the panel. */
void oled_flush(const uint8_t *fb);
