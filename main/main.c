/*
 * ESP32 DEVKIT1 + HLK-LD2450 radar + SSD1306 OLED.
 *
 * Reads target frames from the radar on UART2, prints them on the USB
 * serial console and shows the nearest target ("proximity") on the OLED.
 */
#include <stdio.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "display.h"
#include "ld2450.h"
#include "oled.h"

static const char *TAG = "radar";

#define RADAR_UART           CONFIG_RADAR_UART_NUM
#define RADAR_RX_BUF_SIZE    2048
#define PRINT_INTERVAL_MS    500
#define OLED_INTERVAL_MS     100
#define RADAR_TIMEOUT_MS     1000   /* no frame for this long -> "keine Daten" */
#define NO_DATA_WARN_MS      3000
#define FILTER_TARGET_JUMP_RESET_MM 1000

static uint8_t s_fb[DISP_FB_SIZE];

static void radar_uart_init(void)
{
    const uart_config_t cfg = {
        .baud_rate = CONFIG_RADAR_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(RADAR_UART, RADAR_RX_BUF_SIZE, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(RADAR_UART, &cfg));
    /* uart_set_pin(port, ESP32 TX, ESP32 RX, ...) */
    ESP_ERROR_CHECK(uart_set_pin(RADAR_UART, CONFIG_RADAR_TX_GPIO, CONFIG_RADAR_RX_GPIO,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI(TAG, "LD2450 on UART%d, %d baud, ESP RX=GPIO%d, ESP TX=GPIO%d",
             RADAR_UART, CONFIG_RADAR_BAUD, CONFIG_RADAR_RX_GPIO, CONFIG_RADAR_TX_GPIO);
}

static void print_targets(const ld2450_frame_t *f, uint32_t filtered_nearest_distance_mm)
{
    int nearest = ld2450_nearest(f);
    printf("====================\n");
    printf("Objekte erkannt: %d\n", ld2450_active_count(f));
    for (int i = 0; i < LD2450_MAX_TARGETS; i++) {
        const ld2450_target_t *t = &f->t[i];
        if (!t->active) {
            continue;
        }
        float distance_m = (i == nearest && filtered_nearest_distance_mm > 0)
                               ? filtered_nearest_distance_mm / 1000.0f
                               : ld2450_distance_m(t);
        printf("Objekt %d:\n", i + 1);
        printf("  Position  : X=%d mm, Y=%d mm\n", t->x_mm, t->y_mm);
        printf("  Distanz   : %.2f m%s\n", distance_m,
               i == nearest ? " (Medianfilter)" : "");
        printf("  Winkel    : %.1f Grad\n", ld2450_angle_deg(t));
        printf("  Speed     : %d cm/s (%s)\n", t->speed_cms, ld2450_direction(t));
        printf("  Aufloesung: %u mm\n", t->resolution_mm);
    }
}

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

void app_main(void)
{
    radar_uart_init();

#if CONFIG_OLED_ENABLE
    bool oled_ok = oled_init();
#else
    bool oled_ok = false;
#endif
    const disp_radar_cfg_t disp_cfg = {
        .max_range_mm = CONFIG_RADAR_MAX_RANGE_MM,
        .rx_gpio = CONFIG_RADAR_RX_GPIO,
        .tx_gpio = CONFIG_RADAR_TX_GPIO,
    };

    ld2450_parser_t parser;
    ld2450_parser_init(&parser);
    ld2450_frame_t frame = {0}, rx_frame;
    ld2450_range_filter_t range_filter;
    ld2450_range_filter_reset(&range_filter);
    bool have_frame = false;
    int filtered_target = -1;
    uint32_t previous_raw_distance_mm = 0;
    uint32_t filtered_nearest_distance_mm = 0;
    int64_t last_frame = 0, last_print = 0, last_oled = 0, last_warn = now_ms();
    uint8_t rx[128];

    for (;;) {
        /* Blocks up to 20 ms, so this loop never starves the idle task. */
        int n = uart_read_bytes(RADAR_UART, rx, sizeof(rx), pdMS_TO_TICKS(20));
        for (int i = 0; i < n; i++) {
            if (ld2450_feed(&parser, rx[i], &rx_frame)) {
                frame = rx_frame;
                have_frame = true;
                int nearest = ld2450_nearest(&frame);
                if (nearest >= 0) {
                    if (nearest != filtered_target) {
                        ld2450_range_filter_reset(&range_filter);
                        filtered_target = nearest;
                    }
                    uint32_t raw_distance_mm =
                        (uint32_t)(ld2450_distance_m(&frame.t[nearest]) * 1000.0f + 0.5f);
                    uint32_t jump_mm = raw_distance_mm > previous_raw_distance_mm
                                           ? raw_distance_mm - previous_raw_distance_mm
                                           : previous_raw_distance_mm - raw_distance_mm;
                    if (previous_raw_distance_mm > 0 &&
                        jump_mm > FILTER_TARGET_JUMP_RESET_MM) {
                        ld2450_range_filter_reset(&range_filter);
                    }
                    filtered_nearest_distance_mm =
                        ld2450_range_filter_update(&range_filter, raw_distance_mm);
                    previous_raw_distance_mm = raw_distance_mm;
                } else {
                    ld2450_range_filter_reset(&range_filter);
                    filtered_target = -1;
                    previous_raw_distance_mm = 0;
                    filtered_nearest_distance_mm = 0;
                }
                last_frame = now_ms();
            }
        }

        int64_t now = now_ms();
        bool fresh = have_frame && now - last_frame < RADAR_TIMEOUT_MS;
        if (!fresh) {
            ld2450_range_filter_reset(&range_filter);
            filtered_target = -1;
            previous_raw_distance_mm = 0;
            filtered_nearest_distance_mm = 0;
        }

        if (fresh && now - last_print >= PRINT_INTERVAL_MS) {
            print_targets(&frame, filtered_nearest_distance_mm);
            last_print = now;
        }
        if (!fresh && now - last_warn >= NO_DATA_WARN_MS) {
            ESP_LOGW(TAG, "Keine Radardaten (%lu kaputte Frames). Verkabelung, 5V und Baudrate pruefen.",
                     (unsigned long)parser.bad_frames);
            last_warn = now;
        }
        if (oled_ok && now - last_oled >= OLED_INTERVAL_MS) {
            disp_render_radar(s_fb, &frame, fresh, &disp_cfg, CONFIG_OLED_HEIGHT,
                              filtered_nearest_distance_mm);
            oled_flush(s_fb);
            last_oled = now;
        }
    }
}
