#include "display.h"

#include <stdio.h>
#include <string.h>

#include "font5x7.h"

/* 128x64 screen layout (y coordinates) */
#define Y_HEADER  0
#define Y_BIG     10    /* scale 3 -> 21 px high */
#define Y_BAR     33
#define BAR_H     6
#define Y_LINES   41    /* three 8 px lines: 41, 49, 57 */

void disp_clear(uint8_t *fb)
{
    memset(fb, 0, DISP_FB_SIZE);
}

void disp_pixel(uint8_t *fb, int x, int y, bool on)
{
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) {
        return;
    }
    uint8_t *p = &fb[(y / 8) * DISP_W + x];
    if (on) {
        *p |= (uint8_t)(1 << (y % 8));
    } else {
        *p &= (uint8_t)~(1 << (y % 8));
    }
}

void disp_fill_rect(uint8_t *fb, int x, int y, int w, int h, bool on)
{
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            disp_pixel(fb, i, j, on);
        }
    }
}

void disp_rect(uint8_t *fb, int x, int y, int w, int h)
{
    disp_fill_rect(fb, x, y, w, 1, true);
    disp_fill_rect(fb, x, y + h - 1, w, 1, true);
    disp_fill_rect(fb, x, y, 1, h, true);
    disp_fill_rect(fb, x + w - 1, y, 1, h, true);
}

static void draw_char(uint8_t *fb, int x, int y, char c, int scale)
{
    if (c < FONT5X7_FIRST || c > FONT5X7_LAST) {
        c = '?';
    }
    const uint8_t *glyph = font5x7[c - FONT5X7_FIRST];
    for (int col = 0; col < 5; col++) {
        for (int row = 0; row < 7; row++) {
            if (glyph[col] & (1 << row)) {
                disp_fill_rect(fb, x + col * scale, y + row * scale, scale, scale, true);
            }
        }
    }
}

void disp_text(uint8_t *fb, int x, int y, const char *s, int scale)
{
    for (; *s; s++) {
        draw_char(fb, x, y, *s, scale);
        x += 6 * scale;
    }
}

int disp_text_width(const char *s, int scale)
{
    int n = (int)strlen(s);
    return n ? n * 6 * scale - scale : 0;   /* no gap after the last char */
}

static void text_centered(uint8_t *fb, int y, const char *s, int scale)
{
    disp_text(fb, (DISP_W - disp_text_width(s, scale)) / 2, y, s, scale);
}

static void render_no_data(uint8_t *fb, const disp_radar_cfg_t *cfg, int height)
{
    char line[24];

    if (height == 32) {
        disp_text(fb, 0, 0, "LD2450 keine Daten", 1);
        text_centered(fb, 9, "----", 2);
        snprintf(line, sizeof(line), "TX>RX%d RX>TX%d", cfg->rx_gpio, cfg->tx_gpio);
        disp_text(fb, 0, 25, line, 1);
        return;
    }

    disp_text(fb, 0, Y_HEADER, "LD2450  keine Daten", 1);
    text_centered(fb, Y_BIG, "----", 3);
    disp_text(fb, 0, Y_LINES, "Verkabelung pruefen:", 1);
    snprintf(line, sizeof(line), "Radar TX -> GPIO%d", cfg->rx_gpio);
    disp_text(fb, 0, Y_LINES + 8, line, 1);
    snprintf(line, sizeof(line), "Radar RX -> GPIO%d", cfg->tx_gpio);
    disp_text(fb, 0, Y_LINES + 16, line, 1);
}

void disp_render_radar(uint8_t *fb, const ld2450_frame_t *f, bool have_data,
                       const disp_radar_cfg_t *cfg, int height,
                       uint32_t filtered_nearest_distance_mm)
{
    char line[24];

    disp_clear(fb);
    if (!have_data) {
        render_no_data(fb, cfg, height);
        return;
    }

    int count = ld2450_active_count(f);
    int nearest = ld2450_nearest(f);

    if (height == 32) {
        snprintf(line, sizeof(line), "Ziele:%d  LD2450", count);
        disp_text(fb, 0, 0, line, 1);
        if (nearest < 0) {
            text_centered(fb, 9, "--", 2);
            disp_text(fb, 0, 25, "kein Ziel", 1);
            return;
        }

        const ld2450_target_t *t = &f->t[nearest];
        float distance_m = filtered_nearest_distance_mm > 0
                               ? filtered_nearest_distance_mm / 1000.0f
                               : ld2450_distance_m(t);
        snprintf(line, sizeof(line), "%s%.2fm",
                 filtered_nearest_distance_mm > 0 ? "~" : "", distance_m);
        text_centered(fb, 7, line, 2);
        snprintf(line, sizeof(line), "%4.0f deg %s", ld2450_angle_deg(t),
                 ld2450_direction_short(t));
        disp_text(fb, 0, 25, line, 1);
        return;
    }

    snprintf(line, sizeof(line), "Ziele: %d", count);
    disp_text(fb, 0, Y_HEADER, line, 1);
    disp_text(fb, DISP_W - disp_text_width("LD2450", 1), Y_HEADER, "LD2450", 1);

    /* Proximity bar: full = right at the sensor, empty = max range or more */
    disp_rect(fb, 0, Y_BAR, DISP_W, BAR_H);

    if (nearest < 0) {
        text_centered(fb, Y_BIG, "--", 3);
        text_centered(fb, Y_LINES + 8, "kein Ziel", 1);
        return;
    }

    float d = filtered_nearest_distance_mm > 0
                  ? filtered_nearest_distance_mm / 1000.0f
                  : ld2450_distance_m(&f->t[nearest]);
    snprintf(line, sizeof(line), "%s%.2fm",
             filtered_nearest_distance_mm > 0 ? "~" : "", d);
    text_centered(fb, Y_BIG, line, 3);

    int d_mm = (int)(d * 1000.0f);
    if (d_mm < cfg->max_range_mm) {
        int fill = (DISP_W - 4) * (cfg->max_range_mm - d_mm) / cfg->max_range_mm;
        disp_fill_rect(fb, 2, Y_BAR + 2, fill, BAR_H - 4, true);
    }

    /* One line per target, nearest marked with '>' */
    int y = Y_LINES;
    for (int i = 0; i < LD2450_MAX_TARGETS; i++) {
        const ld2450_target_t *t = &f->t[i];
        if (!t->active) {
            continue;
        }
        snprintf(line, sizeof(line), "%c%d %5.2fm %4.0fd %s",
                 i == nearest ? '>' : ' ', i + 1, ld2450_distance_m(t),
                 ld2450_angle_deg(t), ld2450_direction_short(t));
        disp_text(fb, 0, y, line, 1);
        y += 8;
    }
}
