#include "rfid_protocols_ext.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define EM4095_SHD_PIN 20
#define EM4095_OUT_PIN 26
#define RFID_EMU_PIN   27

extern volatile bool g_core1_run;

bool em4095_read_indala(char* out_uid) {
    gpio_put(EM4095_SHD_PIN, 0); // Enable transmitter
    sleep_ms(20);
    
    uint32_t timings[300];
    uint16_t idx = 0;
    uint32_t start_time = time_us_32();
    bool last_state = gpio_get(EM4095_OUT_PIN);
    
    while ((time_us_32() - start_time) < 50000 && idx < 300) {
        bool current_state = gpio_get(EM4095_OUT_PIN);
        if (current_state != last_state) {
            uint32_t now = time_us_32();
            timings[idx++] = now - start_time;
            start_time = now;
            last_state = current_state;
        }
    }
    
    gpio_put(EM4095_SHD_PIN, 1);
    if (idx < 128) return false;
    
    // Indala 64-bit PSK demodulation
    uint64_t raw = 0;
    for (uint16_t i = 0; i < 64 && (i * 2) < idx; i++) {
        uint32_t dur = timings[i * 2];
        raw = (raw << 1) | (dur > 200 ? 1 : 0);
    }
    
    snprintf(out_uid, 32, "INDALA:%08lX%08lX", (uint32_t)(raw >> 32), (uint32_t)(raw & 0xFFFFFFFF));
    return true;
}

bool em4095_read_awid(char* out_uid) {
    gpio_put(EM4095_SHD_PIN, 0);
    sleep_ms(20);
    
    uint32_t timings[200];
    uint16_t idx = 0;
    uint32_t start_time = time_us_32();
    bool last_state = gpio_get(EM4095_OUT_PIN);
    
    while ((time_us_32() - start_time) < 40000 && idx < 200) {
        bool current_state = gpio_get(EM4095_OUT_PIN);
        if (current_state != last_state) {
            uint32_t now = time_us_32();
            timings[idx++] = now - start_time;
            start_time = now;
            last_state = current_state;
        }
    }
    
    gpio_put(EM4095_SHD_PIN, 1);
    if (idx < 50) return false;
    
    uint32_t card_num = 0;
    for (int i = 0; i < 26 && (i * 2) < idx; i++) {
        card_num = (card_num << 1) | (timings[i * 2] > 250 ? 1 : 0);
    }
    
    snprintf(out_uid, 32, "AWID:26:%06lX", card_num & 0xFFFFFF);
    return true;
}

bool em4095_read_fdxb(char* out_uid) {
    gpio_put(EM4095_SHD_PIN, 0);
    sleep_ms(25);
    
    uint32_t timings[300];
    uint16_t idx = 0;
    uint32_t start_time = time_us_32();
    bool last_state = gpio_get(EM4095_OUT_PIN);
    
    while ((time_us_32() - start_time) < 60000 && idx < 300) {
        bool current_state = gpio_get(EM4095_OUT_PIN);
        if (current_state != last_state) {
            uint32_t now = time_us_32();
            timings[idx++] = now - start_time;
            start_time = now;
            last_state = current_state;
        }
    }
    
    gpio_put(EM4095_SHD_PIN, 1);
    if (idx < 128) return false;
    
    // ISO 11784/11785 FDX-B (134.2kHz BPSK)
    uint64_t animal_id = 0;
    for (int i = 0; i < 38 && (i * 2) < idx; i++) {
        animal_id = (animal_id << 1) | (timings[i * 2] > 180 ? 1 : 0);
    }
    
    snprintf(out_uid, 32, "FDXB:%010llX", (unsigned long long)animal_id);
    return true;
}

void em4095_emulate_psk(const char* uid_hex) {
    gpio_put(EM4095_SHD_PIN, 1);
    
    while (true) {
        for (int i = 0; i < 64; i++) {
            gpio_put(RFID_EMU_PIN, (i & 1));
            sleep_us(128);
        }
        if (!g_core1_run) break;
    }
    gpio_put(RFID_EMU_PIN, 0);
}
