#include "oled.h"

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_ssd1306.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "display.h"

static const char *TAG = "oled";

#define OLED_I2C_PORT     0
#define OLED_I2C_SPEED_HZ (400 * 1000)

static esp_lcd_panel_handle_t s_panel;

bool oled_init(void)
{
    i2c_master_bus_handle_t bus = NULL;
    const i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = OLED_I2C_PORT,
        .sda_io_num = CONFIG_OLED_SDA_GPIO,
        .scl_io_num = CONFIG_OLED_SCL_GPIO,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));

    if (i2c_master_probe(bus, CONFIG_OLED_I2C_ADDR, 100) != ESP_OK) {
        ESP_LOGE(TAG, "No OLED at I2C address 0x%02X (SDA=GPIO%d, SCL=GPIO%d)",
                 CONFIG_OLED_I2C_ADDR, CONFIG_OLED_SDA_GPIO, CONFIG_OLED_SCL_GPIO);
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    const esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = CONFIG_OLED_I2C_ADDR,
        .scl_speed_hz = OLED_I2C_SPEED_HZ,
        .control_phase_bytes = 1,   /* according to the SSD1306 datasheet */
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_bit_offset = 6,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_config, &io));

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = CONFIG_OLED_HEIGHT,
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
        .vendor_config = &ssd1306_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io, &panel_config, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
#if CONFIG_OLED_ROTATE_180
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel, true, true));
#endif
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));

    ESP_LOGI(TAG, "SSD1306 128x%d ready at 0x%02X", CONFIG_OLED_HEIGHT, CONFIG_OLED_I2C_ADDR);
    return true;
}

void oled_flush(const uint8_t *fb)
{
    /* A 128x32 panel only shows the upper half of the 128x64 buffer. */
    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, DISP_W, CONFIG_OLED_HEIGHT, fb);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "draw failed: %s", esp_err_to_name(err));
    }
}
