#include "ibutton_ext.h"
#include "ibutton.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

extern volatile bool g_core1_run;

bool ibutton_read_cyfral(char* out_key_hex) {
    gpio_set_dir(IBUTTON_PIN, GPIO_IN);
    gpio_pull_up(IBUTTON_PIN);
    
    uint32_t pulses[64];
    uint8_t count = 0;
    uint32_t t_start = time_us_32();
    bool last_st = gpio_get(IBUTTON_PIN);
    
    while ((time_us_32() - t_start) < 20000 && count < 64) {
        bool st = gpio_get(IBUTTON_PIN);
        if (st != last_st) {
            pulses[count++] = time_us_32() - t_start;
            t_start = time_us_32();
            last_st = st;
        }
    }
    
    if (count < 36) return false;
    
    uint16_t key_val = 0;
    for (int i = 0; i < 16 && (i * 2) < count; i++) {
        key_val = (key_val << 1) | (pulses[i * 2] > 60 ? 1 : 0);
    }
    
    snprintf(out_key_hex, 32, "CYFRAL:%04X", key_val);
    return true;
}

bool ibutton_read_metakom(char* out_key_hex) {
    gpio_set_dir(IBUTTON_PIN, GPIO_IN);
    gpio_pull_up(IBUTTON_PIN);
    
    uint32_t pulses[64];
    uint8_t count = 0;
    uint32_t t_start = time_us_32();
    bool last_st = gpio_get(IBUTTON_PIN);
    
    while ((time_us_32() - t_start) < 20000 && count < 64) {
        bool st = gpio_get(IBUTTON_PIN);
        if (st != last_st) {
            pulses[count++] = time_us_32() - t_start;
            t_start = time_us_32();
            last_st = st;
        }
    }
    
    if (count < 32) return false;
    
    uint32_t raw = 0;
    for (int i = 0; i < 32 && (i * 2) < count; i++) {
        raw = (raw << 1) | (pulses[i * 2] > 80 ? 1 : 0);
    }
    
    snprintf(out_key_hex, 32, "METAKOM:%08lX", raw);
    return true;
}

void ibutton_emulate_cyfral(const char* key_hex) {
    gpio_set_dir(IBUTTON_PIN, GPIO_OUT);
    while (g_core1_run) {
        for (int i = 0; i < 36; i++) {
            gpio_put(IBUTTON_PIN, i % 2);
            sleep_us(80);
        }
        sleep_ms(10);
    }
    gpio_set_dir(IBUTTON_PIN, GPIO_IN);
}
