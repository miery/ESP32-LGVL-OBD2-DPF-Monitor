#include "main.h"
#include "display.h"
#include "ui.h"
#include "ble_obd.h"
#include "obd_logic.h"
#include "led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "lvgl.h"

// Global variables (moved from original file)
app_state_t state = STATE_DISCONNECTED;

uint16_t conn_handle = 0;
uint16_t obd_char_handle = 0;

int val_diff = 0, val_temp = 0, val_dist = 0, val_soot = 0;
int val_regen = 0;
bool data_ready = false;

// LVGL tick
static void lv_tick_task(void *arg) {
    lv_tick_inc(10);
}

void app_main(void) {

    nvs_flash_init();

    led_init();

    ble_init();

    display_init();

    ui_init();

    // LVGL tick timer
    const esp_timer_create_args_t t_args = {
        .callback = lv_tick_task,
        .name = "lv_tick"
    };

    esp_timer_handle_t lv_tick_timer = NULL;
    esp_timer_create(&t_args, &lv_tick_timer);
    esp_timer_start_periodic(lv_tick_timer, 10 * 1000);

    uint32_t last_obd_ms = 0;

    while (1) {

        lv_timer_handler();

        uint32_t now = esp_timer_get_time() / 1000;

        obd_process(now, &last_obd_ms);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}