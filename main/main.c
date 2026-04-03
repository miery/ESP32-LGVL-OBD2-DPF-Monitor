// Target Hardware: VIEWE SMARTRING (ESP32-S3 + SH8601 AMOLED)
// Manufacturer Source: https://github.com/VIEWESMART/VIEWE-SMARTRING
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_sh8601.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "OBD_DPF";

// =====================================================
// HARDWARE CONFIGURATION
// =====================================================
#define TARGET_NAME "vLinker MC-IOS"

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

// =====================================================
// OBD LOGIC (Astra J - A17DTJ)
// =====================================================
typedef enum {
    STATE_DISCONNECTED,
    STATE_CONNECTED,
    STATE_INIT_OBD,
    STATE_READY
} app_state_t;

static app_state_t state = STATE_DISCONNECTED;

static uint16_t conn_handle;
static uint16_t obd_char_handle = 0;

// Parsed values
static float val_soot = 0.0; 
static int val_diff = 0, val_temp = 0, val_dist = 0;
static int val_regen = 0; // 0 = no regen, >0 = % progress

static bool data_ready = false;

// Initialization commands (includes CAN 500k)
static const char* init_cmds[] = {
    "ATZ",      // Reset
    "ATE0",     // Echo off
    "ATL0",     // Linefeeds off
    "ATSP6",    // Force CAN protocol (500k)
    "ATSH7E0",  // Set header
//    "ATCRA7E8", // Receive filter
//    "ATCAF1",   // Enable formatting
//    "1001"      // Standard session
};

static int init_step = 0;

// OBD queries
static const char* obd_queries[] = {
    "223275", // Soot %
    "220005", // Coolant temperature
    "22007A", // Differential pressure
    "223277",  // Distance between regenerations
	"223274" // Regeneration status
};

static int query_idx = 0;

// =====================================================
// UI GLOBALS
// =====================================================
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_val_soot, *lbl_val_dist, *lbl_val_temp, *lbl_val_diff;

// LCD initialization commands
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

// =====================================================
// LVGL DISPLAY FLUSH CALLBACK
// =====================================================
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

// =====================================================
// CREATE METRIC UI BOX
// =====================================================
static void create_metric_box(
    lv_obj_t *parent,
    const char* title,
    const char* unit,
    int col,
    int row,
    lv_obj_t **val_label
) {
    lv_obj_t * obj = lv_obj_create(parent);

    lv_obj_set_grid_cell(obj,
        LV_GRID_ALIGN_STRETCH, col, 1,
        LV_GRID_ALIGN_STRETCH, row, 1);

    // Box style - całkowicie czarny, bez ramek zmieniających optykę
    lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
    lv_obj_set_style_border_width(obj, 0, 0); 
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    // Title label - statyczna pozycja
    lv_obj_t * l_title = lv_label_create(obj);
    lv_label_set_text(l_title, title);
    lv_obj_set_style_text_font(l_title, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(l_title, lv_color_white(), 0);
    lv_obj_align(l_title, LV_ALIGN_TOP_MID, 0, 0);

    // Value label - KLUCZ DO BRAKU SKAKANIA
    *val_label = lv_label_create(obj);
    lv_label_set_text(*val_label, "---"); // Domyślny tekst
    lv_obj_set_style_text_font(*val_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(*val_label, lv_color_white(), 0);
    
    // Ustawiamy stałą szerokość etykiety i centrowanie tekstu
    lv_obj_set_width(*val_label, 150); 
    lv_obj_set_style_text_align(*val_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(*val_label, LV_ALIGN_CENTER, 0, 5);

    // Unit label
    lv_obj_t * l_unit = lv_label_create(obj);
    lv_label_set_text(l_unit, unit);
    lv_obj_set_style_text_font(l_unit, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l_unit, lv_color_white(), 0);
    lv_obj_align(l_unit, LV_ALIGN_BOTTOM_MID, 0, 0);
}

// =====================================================
// UPDATE REGENERATION STATUS
// =====================================================
void set_regeneration_active(int status) {
    if (status > 0) {
        // Niebieski kolor dla aktywnego wypalania
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x0000FF), 0);
        lv_label_set_text(lbl_status, "REGEN ACTIVE!"); 
    } else {
        // Biały standardowy napis
        lv_obj_set_style_text_color(lbl_status, lv_color_white(), 0);
        lv_label_set_text(lbl_status, "ASTRA J: CONNECTED");
    }
}
// =====================================================
// BLE COMMUNICATION
// =====================================================
static void ble_write_obd(const char *cmd) {
    if (obd_char_handle == 0 || conn_handle == 0) return;

    uint8_t data[32];
    int len = snprintf((char*)data, sizeof(data), "%s\r", cmd);

    ESP_LOGI(TAG, "TX: %s", cmd);

    ble_gattc_write_flat(conn_handle, obd_char_handle, data, len, NULL, NULL);
}

static int gap_cb(struct ble_gap_event *event, void *arg);

// Start BLE scan
static void start_scan(void) {
    ESP_LOGI(TAG, "Starting scan...");

    struct ble_gap_disc_params disc_params = {0};
    disc_params.filter_duplicates = 1;

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, 0, &disc_params, gap_cb, NULL);
}

// Characteristic discovery callback
static int chr_cb(uint16_t conn_id, const struct ble_gatt_error *error,
                  const struct ble_gatt_chr *chr, void *arg) {

    if (error->status == 0 && chr != NULL) {
        if ((chr->properties & BLE_GATT_CHR_PROP_WRITE) &&
            (chr->properties & BLE_GATT_CHR_PROP_NOTIFY)) {

            obd_char_handle = chr->val_handle;

            ESP_LOGI(TAG, "OBD handle found: %d", obd_char_handle);

            // Enable notifications
            uint16_t notify_en = 0x0001;
            ble_gattc_write_flat(conn_id, obd_char_handle + 1, &notify_en, 2, NULL, NULL);

            state = STATE_INIT_OBD;
            init_step = 0;
        }
    }
    return 0;
}

// GAP event handler
static int gap_cb(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;

    switch (event->type) {

        case BLE_GAP_EVENT_DISC:
            ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

            if (fields.name_len > 0 &&
                strncmp((char *)fields.name, TARGET_NAME, fields.name_len) == 0) {

                ESP_LOGI(TAG, "Device found! Connecting...");
                ble_gap_disc_cancel();

                struct ble_gap_conn_params conn_p = {
                    .scan_itvl=128,
                    .scan_window=64,
                    .itvl_min=24,
                    .itvl_max=40,
                    .supervision_timeout=256
                };

                ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr,
                                30000, &conn_p, gap_cb, NULL);
            }
            break;

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connected!");

                conn_handle = event->connect.conn_handle;
                state = STATE_CONNECTED;

                ble_gattc_disc_all_chrs(conn_handle, 1, 0xFFFF, chr_cb, NULL);
            } else {
                start_scan();
            }
            break;

        case BLE_GAP_EVENT_NOTIFY_RX: {
            char rx[64] = {0};

            os_mbuf_copydata(event->notify_rx.om, 0,
                             OS_MBUF_PKTLEN(event->notify_rx.om), rx);

            ESP_LOGI(TAG, "RX: %s", rx);

            unsigned int a, b, c;
            char *p;

			if ((p = strstr(rx, "62 32 77")) && sscanf(p + 9, "%x %x %x", &a, &b, &c) == 3) {
                val_dist = (a << 16) | (b << 8) | c;
                data_ready = true;
            }
			else if ((p = strstr(rx, "62 32 75")) && sscanf(p + 9, "%x", &a) == 1) {
				val_soot = (float)a;
                data_ready = true;
            }
			else if ((p = strstr(rx, "62 00 05")) && sscanf(p + 9, "%x", &a) == 1) {
                val_temp = a - 40;
                data_ready = true;
            }
			else if ((p = strstr(rx, "62 00 7A")) && sscanf(p + 9, "%x %x %x", &a, &b, &c) == 3) {
				float raw_val = (float)((a << 16) | (b << 8) | c);
				val_diff = (int)((raw_val - 65535.0f) / 10.0f); 
				data_ready = true;
			}
			else if ((p = strstr(rx, "62 32 74")) && sscanf(p + 9, "%x", &a) == 1) {
				val_regen = a; 
				data_ready = true;
            }
            break;
        }

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "Disconnected! Restarting scan...");
            state = STATE_DISCONNECTED;
            start_scan();
            break;
    }

    return 0;
}

// LVGL tick
static void lv_tick_task(void *arg) {
    lv_tick_inc(10);
}

// =====================================================
// MAIN APPLICATION
// =====================================================
void app_main(void) {

    // Initialize NVS and BLE
    nvs_flash_init();
    nimble_port_init();

    ble_hs_cfg.sync_cb = start_scan;

    xTaskCreatePinnedToCore(
        (void*)nimble_port_run,
        "nimble",
        4096,
        NULL,
        10,
        NULL,
        0
    );

    // Backlight GPIO
    gpio_config_t bk = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BK_LIGHT
    };

    gpio_config(&bk);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    // SPI bus init
    spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        PIN_NUM_PCLK,
        PIN_NUM_DATA0,
        PIN_NUM_DATA1,
        PIN_NUM_DATA2,
        PIN_NUM_DATA3,
        LCD_H_RES * LCD_V_RES * 2
    );

    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // LCD init
    esp_lcd_panel_io_handle_t io_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config =
        SH8601_PANEL_IO_QSPI_CONFIG(PIN_NUM_CS, NULL, NULL);

    esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST,
        &io_config,
        &io_handle
    );

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

    // LVGL init
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

    // LVGL tick timer
    const esp_timer_create_args_t t_args = {
        .callback = lv_tick_task,
        .name = "lv_tick"
    };

    esp_timer_handle_t timer;
    esp_timer_create(&t_args, &timer);
    esp_timer_start_periodic(timer, 10000);

    // =====================================================
    // UI SETUP
    // =====================================================
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    lbl_status = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(lbl_status, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_22, 0);

    lv_obj_set_style_bg_opa(lbl_status, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(lbl_status, lv_color_black(), 0);
    lv_obj_set_style_pad_all(lbl_status, 8, 0);

    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 35);

    lv_label_set_text(lbl_status, "SEARCHING OBD...");

    lv_obj_t *main_cont = lv_obj_create(lv_scr_act());

    lv_obj_set_size(main_cont, LCD_H_RES - 30, LCD_V_RES - 110);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 85);

    static lv_coord_t col_dsc[] = {
        LV_GRID_FR(1),
        LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };

    static lv_coord_t row_dsc[] = {
        LV_GRID_FR(1),
        LV_GRID_FR(1),
        LV_GRID_TEMPLATE_LAST
    };

    lv_obj_set_layout(main_cont, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(main_cont, col_dsc, row_dsc);

    lv_obj_set_style_bg_opa(main_cont, 0, 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);

    // Metric boxes (translated labels)
    create_metric_box(main_cont, "SOOT", "%", 0, 0, &lbl_val_soot);
    create_metric_box(main_cont, "DISTANCE", "km", 1, 0, &lbl_val_dist);
    create_metric_box(main_cont, "TEMP", "°C", 0, 1, &lbl_val_temp);
    create_metric_box(main_cont, "DELTA P", "hPa", 1, 1, &lbl_val_diff);

    uint32_t last_obd_ms = 0;

    while (1) {
        lv_timer_handler();

        uint32_t now = esp_timer_get_time() / 1000;

        // OBD initialization sequence
        if (state == STATE_INIT_OBD) {
            if (now - last_obd_ms > 1500) {
                ble_write_obd(init_cmds[init_step]);

                init_step++;

                if (init_step >= 4) {
                    state = STATE_READY;
                    lv_label_set_text(lbl_status, "Astra J: Connected");
                }

                last_obd_ms = now;
            }
        }

        // Normal operation
        else if (state == STATE_READY) {

            if (now - last_obd_ms > 1500) {
                ble_write_obd(obd_queries[query_idx]);
                query_idx = (query_idx + 1) % 5;
                last_obd_ms = now;
            }

            if (data_ready) {
                lv_label_set_text_fmt(lbl_val_soot, "%.1f", val_soot);
                lv_label_set_text_fmt(lbl_val_temp, "%d", val_temp);
                lv_label_set_text_fmt(lbl_val_diff, "%d", val_diff);
                lv_label_set_text_fmt(lbl_val_dist, "%d", val_dist);

                set_regeneration_active(val_regen > 0);
                data_ready = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}