#include "display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_sh8601.h"
#include "lvgl.h"
#include "esp_heap_caps.h"

#define LCD_HOST SPI2_HOST

#define PIN_NUM_CS 7
#define PIN_NUM_PCLK 13
#define PIN_NUM_DATA0 12
#define PIN_NUM_DATA1 8
#define PIN_NUM_DATA2 14
#define PIN_NUM_DATA3 9
#define PIN_NUM_RST 11
#define PIN_NUM_BK_LIGHT 40

#define LCD_H_RES 466
#define LCD_V_RES 466
#define LCD_BIT_PER_PIXEL 16
#define LVGL_BUF_SIZE (LCD_H_RES * 20)

// LCD init commands (unchanged)
static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x00}, 0, 0},
    {0xC4, (uint8_t []){0x80}, 1, 0},
    {0x3A, (uint8_t []){0x55}, 1, 0},
    {0x35, (uint8_t []){0x00}, 0, 10},
    {0x53, (uint8_t []){0x20}, 1, 10},
    {0x51, (uint8_t []){0xFF}, 1, 10},
    {0x63, (uint8_t []){0xFF}, 1, 10},
    {0x2A, (uint8_t []){0x00,0x06,0x01,0xDD}, 4, 0},
    {0x2B, (uint8_t []){0x00,0x00,0x01,0xD1}, 4, 0},
    {0x11, (uint8_t []){0x00}, 0, 60},
    {0x29, (uint8_t []){0x00}, 0, 0},
};

// LVGL flush callback
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {

    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t) drv->user_data;

    esp_lcd_panel_draw_bitmap(
        panel,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        (void *)color_map
    );

    lv_disp_flush_ready(drv);
}

void display_init(void) {

    // Backlight
    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };
    gpio_config(&bk);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    // SPI
    spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        PIN_NUM_PCLK,
        PIN_NUM_DATA0,
        PIN_NUM_DATA1,
        PIN_NUM_DATA2,
        PIN_NUM_DATA3,
        LCD_H_RES * LCD_V_RES * 2
    );

    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config =
        SH8601_PANEL_IO_QSPI_CONFIG(PIN_NUM_CS, NULL, NULL);

    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    esp_lcd_panel_handle_t panel = NULL;

    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds)/sizeof(sh8601_lcd_init_cmd_t),
        .flags = { .use_qspi_interface = 1 },
    };

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };

    esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel);

    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);
    esp_lcd_panel_set_gap(panel, 6, 0);
    esp_lcd_panel_disp_on_off(panel, true);

    // LVGL
    lv_init();

    lv_color_t *buf1 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(LVGL_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel;

    lv_disp_drv_register(&disp_drv);
}