/*
 * Host tests for the LD2450 parser and the OLED layout (no ESP32 needed).
 *
 *   ./test_ld2450            run the tests
 *   ./test_ld2450 preview    also print the OLED screens as ASCII art
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "ld2450.h"

static int failures;

#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
            failures++;                                                     \
        }                                                                   \
    } while (0)

/* Example frame from the HLK-LD2450 datasheet: one target at
 * x = -782 mm, y = 1713 mm, speed = -16 cm/s, resolution = 320 mm. */
static const uint8_t DATASHEET_FRAME[LD2450_FRAME_LEN] = {
    0xAA, 0xFF, 0x03, 0x00,
    0x0E, 0x03, 0xB1, 0x86, 0x10, 0x00, 0x40, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x55, 0xCC,
};

/* Encode a value the way the sensor does (bit15 = 1 means positive). */
static void put_s16(uint8_t *p, int v)
{
    uint16_t raw = (uint16_t)(v >= 0 ? (0x8000 | v) : -v);
    p[0] = raw & 0xFF;
    p[1] = raw >> 8;
}

static void make_frame(uint8_t *buf, const int targets[3][4])
{
    memset(buf, 0, LD2450_FRAME_LEN);
    memcpy(buf, "\xAA\xFF\x03\x00", 4);
    for (int i = 0; i < 3; i++) {
        uint8_t *d = buf + 4 + i * 8;
        if (targets[i][0] == 0 && targets[i][1] == 0) {
            continue;   /* empty slot: all zero */
        }
        put_s16(d, targets[i][0]);
        put_s16(d + 2, targets[i][1]);
        put_s16(d + 4, targets[i][2]);
        d[6] = targets[i][3] & 0xFF;
        d[7] = targets[i][3] >> 8;
    }
    buf[28] = 0x55;
    buf[29] = 0xCC;
}

static int feed_all(ld2450_parser_t *p, const uint8_t *data, int len, ld2450_frame_t *out)
{
    int frames = 0;
    for (int i = 0; i < len; i++) {
        if (ld2450_feed(p, data[i], out)) {
            frames++;
        }
    }
    return frames;
}

static void test_datasheet_frame(void)
{
    ld2450_frame_t f;
    CHECK(ld2450_decode_frame(DATASHEET_FRAME, &f));
    CHECK(f.t[0].x_mm == -782);
    CHECK(f.t[0].y_mm == 1713);
    CHECK(f.t[0].speed_cms == -16);
    CHECK(f.t[0].resolution_mm == 320);
    CHECK(f.t[0].active);
    CHECK(!f.t[1].active && !f.t[2].active);
    CHECK(ld2450_active_count(&f) == 1);
    CHECK(ld2450_nearest(&f) == 0);
    CHECK(fabsf(ld2450_distance_m(&f.t[0]) - 1.8829f) < 0.001f);
    CHECK(strcmp(ld2450_direction(&f.t[0]), "kommt naeher") == 0);
}

static void test_parser_resync(void)
{
    ld2450_parser_t p;
    ld2450_frame_t f;
    uint8_t stream[3 + LD2450_FRAME_LEN + 2 + LD2450_FRAME_LEN];
    int n = 0;

    /* garbage, a frame, a repeated 0xAA, another frame */
    stream[n++] = 0x12;
    stream[n++] = 0xAA;
    stream[n++] = 0x00;
    memcpy(stream + n, DATASHEET_FRAME, LD2450_FRAME_LEN);
    n += LD2450_FRAME_LEN;
    stream[n++] = 0xAA;
    stream[n++] = 0xAA;
    memcpy(stream + n, DATASHEET_FRAME + 1, LD2450_FRAME_LEN - 1);
    n += LD2450_FRAME_LEN - 1;

    ld2450_parser_init(&p);
    CHECK(feed_all(&p, stream, n, &f) == 2);
    CHECK(f.t[0].y_mm == 1713);
}

static void test_bad_tail(void)
{
    ld2450_parser_t p;
    ld2450_frame_t f;
    uint8_t bad[LD2450_FRAME_LEN];
    memcpy(bad, DATASHEET_FRAME, sizeof(bad));
    bad[29] = 0x00;

    ld2450_parser_init(&p);
    CHECK(feed_all(&p, bad, sizeof(bad), &f) == 0);
    CHECK(p.bad_frames == 1);
    /* parser recovers on the next good frame */
    CHECK(feed_all(&p, DATASHEET_FRAME, LD2450_FRAME_LEN, &f) == 1);
}

static void test_nearest(void)
{
    const int targets[3][4] = {
        {300, 2500, 0, 360},
        {-150, 900, -40, 360},
        {1200, 3000, 25, 360},
    };
    uint8_t buf[LD2450_FRAME_LEN];
    ld2450_frame_t f;
    make_frame(buf, targets);
    CHECK(ld2450_decode_frame(buf, &f));
    CHECK(ld2450_active_count(&f) == 3);
    CHECK(ld2450_nearest(&f) == 1);
    CHECK(f.t[1].x_mm == -150);
    CHECK(f.t[2].speed_cms == 25);
}

static void print_screen(const char *title, const uint8_t *fb)
{
    printf("\n%s\n+", title);
    for (int x = 0; x < DISP_W; x++) {
        putchar('-');
    }
    printf("+\n");
    for (int y = 0; y < DISP_H; y++) {
        putchar('|');
        for (int x = 0; x < DISP_W; x++) {
            putchar(fb[(y / 8) * DISP_W + x] & (1 << (y % 8)) ? '#' : ' ');
        }
        printf("|\n");
    }
    putchar('+');
    for (int x = 0; x < DISP_W; x++) {
        putchar('-');
    }
    printf("+\n");
}

static void preview(void)
{
    const disp_radar_cfg_t cfg = {.max_range_mm = 6000, .rx_gpio = 16, .tx_gpio = 17};
    const int targets[3][4] = {
        {300, 2500, 0, 360},
        {-150, 900, -40, 360},
        {1200, 3000, 25, 360},
    };
    uint8_t buf[LD2450_FRAME_LEN];
    uint8_t fb[DISP_FB_SIZE];
    ld2450_frame_t f;

    make_frame(buf, targets);
    ld2450_decode_frame(buf, &f);
    disp_render_radar(fb, &f, true, &cfg, 64, 0);
    print_screen("3 targets:", fb);

    ld2450_decode_frame(DATASHEET_FRAME, &f);
    disp_render_radar(fb, &f, true, &cfg, 64, 0);
    print_screen("Datasheet frame (1 target):", fb);

    memset(&f, 0, sizeof(f));
    disp_render_radar(fb, &f, true, &cfg, 64, 0);
    print_screen("Radar OK, nobody in range:", fb);

    disp_render_radar(fb, &f, false, &cfg, 64, 0);
    print_screen("No data from radar:", fb);
}

int main(int argc, char **argv)
{
    test_datasheet_frame();
    test_parser_resync();
    test_bad_tail();
    test_nearest();

    if (argc > 1 && strcmp(argv[1], "preview") == 0) {
        preview();
    }

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("All tests passed\n");
    return 0;
}
