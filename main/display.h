/*
 * 128x64 monochrome frame buffer + radar screen layout (hardware independent).
 *
 * Buffer layout is the native SSD1306 "page" format: byte (y / 8) * 128 + x,
 * bit (y % 8). It can be sent to the panel as is.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ld2450.h"

#define DISP_W       128
#define DISP_H       64
#define DISP_FB_SIZE (DISP_W * DISP_H / 8)

typedef struct {
    int max_range_mm;   /* distance at which the proximity bar is empty */
    int rx_gpio;        /* shown on the "no data" screen as wiring hint */
    int tx_gpio;
} disp_radar_cfg_t;

void disp_clear(uint8_t *fb);
void disp_pixel(uint8_t *fb, int x, int y, bool on);
void disp_fill_rect(uint8_t *fb, int x, int y, int w, int h, bool on);
void disp_rect(uint8_t *fb, int x, int y, int w, int h);
/* Draws text with the 5x7 font, 6 * scale pixels per character. */
void disp_text(uint8_t *fb, int x, int y, const char *s, int scale);
int disp_text_width(const char *s, int scale);

/* Draw the complete radar screen. have_data = false shows a wiring hint. */
void disp_render_radar(uint8_t *fb, const ld2450_frame_t *f, bool have_data,
                       const disp_radar_cfg_t *cfg);
