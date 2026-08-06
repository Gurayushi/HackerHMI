#include "ibutton.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

extern volatile bool g_core1_run;

uint8_t ibutton_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C; // Maxim 1-Wire polynomial X^8 + X^5 + X^4 + 1
            }
            inbyte >>= 1;
        }
    }
    return crc;
}

static inline void ow_drive_low(void) {
    gpio_set_dir(IBUTTON_PIN, GPIO_OUT);
    gpio_put(IBUTTON_PIN, 0);
}

static inline void ow_release(void) {
    gpio_set_dir(IBUTTON_PIN, GPIO_IN);
    gpio_pull_up(IBUTTON_PIN);
}

static inline bool ow_read_pin(void) {
    return gpio_get(IBUTTON_PIN);
}

void ibutton_init(void) {
    gpio_init(IBUTTON_PIN);
    ow_release();
}

static bool ow_reset(void) {
    ow_drive_low();
    sleep_us(480);
    ow_release();
    sleep_us(70);
    bool presence = !ow_read_pin();
    sleep_us(410);
    return presence;
}

static void ow_write_bit(bool bit) {
    if (bit) {
        ow_drive_low();
        sleep_us(6);
        ow_release();
        sleep_us(64);
    } else {
        ow_drive_low();
        sleep_us(60);
        ow_release();
        sleep_us(10);
    }
}

static bool ow_read_bit(void) {
    ow_drive_low();
    sleep_us(6);
    ow_release();
    sleep_us(9);
    bool bit = ow_read_pin();
    sleep_us(55);
    return bit;
}

static void ow_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit((data >> i) & 1);
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) {
        if (ow_read_bit()) {
            value |= (1 << i);
        }
    }
    return value;
}

bool ibutton_read_ds1990a(char* out_key_hex) {
    ibutton_init();
    if (!ow_reset()) {
        return false;
    }
    
    // Send Read ROM Command (0x33)
    ow_write_byte(0x33);
    
    uint8_t rom[8] = {0};
    for (int i = 0; i < 8; i++) {
        rom[i] = ow_read_byte();
    }
    
    // DS1990A Family Code must be 0x01
    if (rom[0] != 0x01) {
        return false;
    }
    
    // Verify CRC8
    if (ibutton_crc8(rom, 7) != rom[7]) {
        return false;
    }
    
    snprintf(out_key_hex, 32, "DS1990:%02X%02X%02X%02X%02X%02X",
             rom[1], rom[2], rom[3], rom[4], rom[5], rom[6]);
    return true;
}

void ibutton_emulate_ds1990a(const char* key_hex) {
    ibutton_init();
    
    uint8_t rom[8] = {0};
    rom[0] = 0x01; // Family code
    
    uint8_t raw[6] = {0};
    if (sscanf(key_hex, "DS1990:%2hhX%2hhX%2hhX%2hhX%2hhX%2hhX",
               &raw[0], &raw[1], &raw[2], &raw[3], &raw[4], &raw[5]) == 6) {
        memcpy(&rom[1], raw, 6);
    }
    rom[7] = ibutton_crc8(rom, 7);
    
    while (true) {
        // Wait for Reset pulse from Master (low > 300us)
        while (ow_read_pin()) {
            if (!g_core1_run) return;
            sleep_us(10);
        }
        
        uint32_t t_start = time_us_32();
        while (!ow_read_pin()) {
            if (!g_core1_run) return;
            if ((time_us_32() - t_start) > 2000) break;
        }
        uint32_t low_dur = time_us_32() - t_start;
        
        if (low_dur >= 300 && low_dur <= 1000) {
            // Wait presence delay
            sleep_us(30);
            ow_drive_low();
            sleep_us(150);
            ow_release();
            
            // Read Command from Master
            uint8_t cmd = ow_read_byte();
            if (cmd == 0x33 || cmd == 0x0F) { // Read ROM or Search ROM
                for (int b = 0; b < 8; b++) {
                    ow_write_byte(rom[b]);
                }
            }
        }
        
        if (!g_core1_run) break;
    }
}
