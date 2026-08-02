#include "led.h"

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "attack.h"

#define LED_GPIO        27

typedef struct {
    int on_ms;
    int off_ms;
    int repeat;     /* pulses per cycle */
    int gap_ms;     /* idle time between cycles */
} pattern_t;

static const pattern_t PATTERN_IDLE      = { 80,  120, 1, 4800 };
static const pattern_t PATTERN_ATTACKING = { 80,  80,  1, 0    };

static void led_write(int on)
{
    gpio_set_level(LED_GPIO, on ? 0 : 1);
}

static const pattern_t *current_pattern(void)
{
    if (attack_is_running()) {
        return &PATTERN_ATTACKING;
    }
    return &PATTERN_IDLE;
}

static void led_task(void *arg)
{
    (void)arg;
    while (1) {
        const pattern_t *p = current_pattern();
        for (int i = 0; i < p->repeat; i++) {
            led_write(1);
            vTaskDelay(pdMS_TO_TICKS(p->on_ms));
            led_write(0);
            vTaskDelay(pdMS_TO_TICKS(p->off_ms));
        }
        if (p->gap_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(p->gap_ms));
        }
    }
}

void led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    led_write(0);
    xTaskCreate(led_task, "led", 2048, NULL, 1, NULL);
}
