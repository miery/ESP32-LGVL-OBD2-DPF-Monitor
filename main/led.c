#include "led.h"
#include "led_strip.h"

#define LED_STRIP_BLINK_GPIO 39
#define LED_STRIP_LED_NUMBERS 8
#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

static led_strip_handle_t led_strip;

void led_init(void) {

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_BLINK_GPIO,
        .max_leds = LED_STRIP_LED_NUMBERS,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
    };

    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

void led_set_red(void) {
    for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++)
        led_strip_set_pixel(led_strip, i, 200, 0, 0);

    led_strip_refresh(led_strip);
}

void led_set_idle(void) {
    for (int i = 0; i < LED_STRIP_LED_NUMBERS; i++)
        led_strip_set_pixel(led_strip, i, 50, 50, 50);

    led_strip_refresh(led_strip);
}