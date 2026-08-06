#include "rfid_nfc.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define PN532_I2C_PORT i2c0
#define PN532_SDA_PIN  4
#define PN532_SCL_PIN  5
#define PN532_ADDR     0x24

#define EM4095_SHD_PIN 20
#define EM4095_MOD_PIN 21
#define EM4095_OUT_PIN 26
#define RFID_EMU_PIN   27

extern volatile bool g_core1_run;

void rfid_nfc_init(void) {
    i2c_init(PN532_I2C_PORT, 400 * 1000);
    gpio_set_function(PN532_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PN532_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PN532_SDA_PIN);
    gpio_pull_up(PN532_SCL_PIN);
    
    gpio_init(EM4095_SHD_PIN);
    gpio_set_dir(EM4095_SHD_PIN, GPIO_OUT);
    gpio_put(EM4095_SHD_PIN, 1);

    gpio_init(EM4095_MOD_PIN);
    gpio_set_dir(EM4095_MOD_PIN, GPIO_OUT);
    gpio_put(EM4095_MOD_PIN, 0);

    gpio_init(RFID_EMU_PIN);
    gpio_set_dir(RFID_EMU_PIN, GPIO_OUT);
    gpio_put(RFID_EMU_PIN, 0);

    gpio_init(EM4095_OUT_PIN);
    gpio_set_dir(EM4095_OUT_PIN, GPIO_IN);
}

void pn532_i2c_write(uint8_t* data, uint8_t length) {
    i2c_write_blocking(PN532_I2C_PORT, PN532_ADDR, data, length, false);
}

void pn532_i2c_read(uint8_t* buffer, uint8_t length) {
    uint8_t status = 0;
    uint32_t start = time_us_32();
    while (status != 0x01) {
        if (!g_core1_run) {
            break;
        }
        if ((time_us_32() - start) > 5000000) {
            break;
        }
        i2c_read_blocking(PN532_I2C_PORT, PN532_ADDR, &status, 1, false);
        sleep_ms(1);
    }
    if (status == 0x01) {
        i2c_read_blocking(PN532_I2C_PORT, PN532_ADDR, buffer, length, false);
    } else {
        memset(buffer, 0, length);
    }
}

bool pn532_read_nfc(uint8_t* out_uid, uint8_t* uid_len) {
    uint8_t cmd_scan[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
    pn532_i2c_write(cmd_scan, sizeof(cmd_scan));
    
    uint8_t response[24];
    pn532_i2c_read(response, sizeof(response));
    
    if (response[7] == 0xD5 && response[8] == 0x4B && response[9] > 0) {
        *uid_len = response[14];
        if (*uid_len <= 10) {
            memcpy(out_uid, &response[15], *uid_len);
            return true;
        }
    }
    return false;
}

bool pn532_emulate_uid(uint8_t* uid, uint8_t uid_len) {
    uint8_t cmd_emu[32] = {0x00, 0x00, 0xFF, 0x00, 0x00, 0xD4, 0x8C, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20};
    uint8_t data_len = 9 + uid_len;
    cmd_emu[3] = data_len;
    cmd_emu[4] = (uint8_t)(~data_len + 1);
    
    memcpy(&cmd_emu[10], uid, uid_len);
    cmd_emu[10 + uid_len] = 0x20;
    
    uint8_t sum = 0;
    for (uint8_t i = 5; i < 11 + uid_len; i++) {
        sum += cmd_emu[i];
    }
    cmd_emu[11 + uid_len] = (uint8_t)(~sum + 1);
    cmd_emu[12 + uid_len] = 0x00;
    
    pn532_i2c_write(cmd_emu, 13 + uid_len);
    
    uint8_t response[16];
    pn532_i2c_read(response, sizeof(response));
    
    return (response[7] == 0xD5 && response[8] == 0x8D);
}

bool em4095_decode_em4100_frame(uint64_t frame, char* out_uid) {
    uint8_t bytes[5] = {0};
    uint8_t col_parity = 0;
    
    if (frame & 1) return false;
    if ((frame >> 55) != 0x1FF) return false;
    
    for (int i = 0; i < 10; i++) {
        uint8_t nibble_chunk = (frame >> (5 + (9 - i) * 5)) & 0x1F;
        uint8_t data = (nibble_chunk >> 1) & 0x0F;
        uint8_t parity = nibble_chunk & 1;
        
        uint8_t calc_p = (data ^ (data >> 1) ^ (data >> 2) ^ (data >> 3)) & 1;
        if (calc_p != parity) return false;
        
        if (i % 2 == 0) {
            bytes[i / 2] |= (data << 4);
        } else {
            bytes[i / 2] |= data;
        }
        col_parity ^= data;
    }
    
    uint8_t read_col_parity = (frame >> 1) & 0x0F;
    if (col_parity != read_col_parity) return false;
    
    snprintf(out_uid, 16, "%02X%02X%02X%02X%02X", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4]);
    return true;
}

bool em4095_read_em4100(char* out_uid) {
    gpio_put(EM4095_SHD_PIN, 0);
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
    
    if (idx < 130) return false;
    
    uint64_t frame = 0;
    uint8_t bit_idx = 0;
    bool last_bit = false;
    
    for (uint16_t i = 1; i < idx && bit_idx < 64; i++) {
        uint32_t dur = timings[i];
        if (dur >= 180 && dur <= 320) {
            i++;
            if (i >= idx) break;
            frame = (frame << 1) | (last_bit ? 1 : 0);
            bit_idx++;
        } else if (dur >= 380 && dur <= 640) {
            last_bit = !last_bit;
            frame = (frame << 1) | (last_bit ? 1 : 0);
            bit_idx++;
        }
    }
    
    if (bit_idx >= 64) {
        for (int shift = 0; shift <= (bit_idx - 64); shift++) {
            if (em4095_decode_em4100_frame(frame >> shift, out_uid)) {
                return true;
            }
        }
    }
    
    return false;
}

bool em4095_write_t5577_block(uint8_t block, uint32_t data) {
    gpio_put(EM4095_SHD_PIN, 0);
    sleep_ms(30);
    
    #define SEND_GAP(bit) \
        gpio_put(EM4095_MOD_PIN, 1); \
        sleep_us(100); \
        gpio_put(EM4095_MOD_PIN, 0); \
        sleep_us((bit) ? 448 : 192);

    SEND_GAP(1);
    SEND_GAP(0);
    SEND_GAP(0);
    
    for (int i = 2; i >= 0; i--) {
        SEND_GAP((block >> i) & 1);
    }
    
    for (int i = 31; i >= 0; i--) {
        SEND_GAP((data >> i) & 1);
    }
    
    sleep_ms(10);
    gpio_put(EM4095_SHD_PIN, 1);
    return true;
}

bool em4095_write_em4100(const char* uid_hex) {
    uint8_t uid[5] = {0};
    for (int i = 0; i < 5; i++) {
        unsigned int val;
        sscanf(uid_hex + i * 2, "%2x", &val);
        uid[i] = (uint8_t)val;
    }
    
    uint64_t frame = 0x1FF;
    uint8_t col_parity = 0;
    
    for (int i = 0; i < 5; i++) {
        uint8_t d1 = (uid[i] >> 4) & 0x0F;
        uint8_t d2 = uid[i] & 0x0F;
        col_parity ^= d1 ^ d2;
        
        uint8_t p1 = (d1 ^ (d1 >> 1) ^ (d1 >> 2) ^ (d1 >> 3)) & 1;
        uint8_t p2 = (d2 ^ (d2 >> 1) ^ (d2 >> 2) ^ (d2 >> 3)) & 1;
        
        frame = (frame << 5) | (d1 << 1) | p1;
        frame = (frame << 5) | (d2 << 1) | p2;
    }
    
    frame = (frame << 5) | (col_parity << 1) | 0;
    
    uint32_t block1 = (uint32_t)(frame >> 32);
    uint32_t block2 = (uint32_t)(frame & 0xFFFFFFFF);
    
    if (!em4095_write_t5577_block(0, 0x00148040)) return false;
    sleep_ms(10);
    if (!em4095_write_t5577_block(1, block1)) return false;
    sleep_ms(10);
    if (!em4095_write_t5577_block(2, block2)) return false;
    
    return true;
}

void em4095_emulate_em4100(const char* uid_hex) {
    gpio_put(EM4095_SHD_PIN, 1);
    
    uint8_t uid[5] = {0};
    for (int i = 0; i < 5; i++) {
        unsigned int val;
        sscanf(uid_hex + i * 2, "%2x", &val);
        uid[i] = (uint8_t)val;
    }
    
    uint64_t frame = 0x1FF;
    uint8_t col_parity = 0;
    
    for (int i = 0; i < 5; i++) {
        uint8_t d1 = (uid[i] >> 4) & 0x0F;
        uint8_t d2 = uid[i] & 0x0F;
        col_parity ^= d1 ^ d2;
        
        uint8_t p1 = (d1 ^ (d1 >> 1) ^ (d1 >> 2) ^ (d1 >> 3)) & 1;
        uint8_t p2 = (d2 ^ (d2 >> 1) ^ (d2 >> 2) ^ (d2 >> 3)) & 1;
        
        frame = (frame << 5) | (d1 << 1) | p1;
        frame = (frame << 5) | (d2 << 1) | p2;
    }
    
    frame = (frame << 5) | (col_parity << 1) | 0;
    
    while (true) {
        for (int i = 63; i >= 0; i--) {
            bool bit = (frame >> i) & 1;
            if (bit) {
                gpio_put(RFID_EMU_PIN, 1);
                sleep_us(256);
                gpio_put(RFID_EMU_PIN, 0);
                sleep_us(256);
            } else {
                gpio_put(RFID_EMU_PIN, 0);
                sleep_us(256);
                gpio_put(RFID_EMU_PIN, 1);
                sleep_us(256);
            }
        }
        
        if (!g_core1_run) {
            break;
        }
        sleep_ms(1);
    }
    gpio_put(RFID_EMU_PIN, 0);
}

bool em4095_read_hid_prox(char* out_uid) {
    gpio_put(EM4095_SHD_PIN, 0);
    sleep_ms(20);
    
    uint32_t timings[400];
    uint16_t idx = 0;
    
    uint32_t start_time = time_us_32();
    bool last_state = gpio_get(EM4095_OUT_PIN);
    
    while ((time_us_32() - start_time) < 60000 && idx < 400) {
        bool current_state = gpio_get(EM4095_OUT_PIN);
        if (current_state != last_state) {
            uint32_t now = time_us_32();
            timings[idx++] = now - start_time;
            start_time = now;
            last_state = current_state;
        }
    }
    
    gpio_put(EM4095_SHD_PIN, 1);
    
    if (idx < 150) return false;
    
    uint32_t bit_time_accumulator = 0;
    uint32_t pulse_count_in_bit = 0;
    uint32_t sum_pulse_durs = 0;
    
    uint32_t decoded_bits[100];
    uint8_t bit_idx = 0;
    
    for (uint16_t i = 0; i < idx && bit_idx < 100; i++) {
        uint32_t dur = timings[i];
        bit_time_accumulator += dur;
        pulse_count_in_bit++;
        sum_pulse_durs += dur;
        
        if (bit_time_accumulator >= 512) {
            uint32_t avg_dur = sum_pulse_durs / pulse_count_in_bit;
            if (avg_dur <= 35) {
                decoded_bits[bit_idx++] = 0;
            } else {
                decoded_bits[bit_idx++] = 1;
            }
            bit_time_accumulator = 0;
            pulse_count_in_bit = 0;
            sum_pulse_durs = 0;
        }
    }
    
    if (bit_idx < 26) return false;
    
    for (int start = 0; start <= (bit_idx - 26); start++) {
        uint32_t packet = 0;
        for (int i = 0; i < 26; i++) {
            packet = (packet << 1) | decoded_bits[start + i];
        }
        
        uint8_t p_leading = (packet >> 25) & 1;
        uint8_t p_trailing = packet & 1;
        
        uint32_t data = (packet >> 1) & 0xFFFFFF;
        uint8_t fc = (data >> 16) & 0xFF;
        uint16_t cn = data & 0xFFFF;
        
        uint8_t calc_even = 0;
        for (int i = 12; i < 24; i++) {
            calc_even ^= (data >> i) & 1;
        }
        uint8_t calc_odd = 1;
        for (int i = 0; i < 12; i++) {
            calc_odd ^= (data >> i) & 1;
        }
        
        if (calc_even == p_leading && calc_odd == p_trailing) {
            snprintf(out_uid, 32, "HID:%d:%d", fc, cn);
            return true;
        }
    }
    
    return false;
}

void em4095_emulate_hid_prox(const char* uid_str) {
    gpio_put(EM4095_SHD_PIN, 1);
    
    int fc = 0;
    int cn = 0;
    if (sscanf(uid_str, "HID:%d:%d", &fc, &cn) != 2) return;
    
    uint32_t packet = 0;
    uint32_t data = ((fc & 0xFF) << 16) | (cn & 0xFFFF);
    
    uint8_t p_leading = 0;
    for (int i = 12; i < 24; i++) {
        p_leading ^= (data >> i) & 1;
    }
    uint8_t p_trailing = 1;
    for (int i = 0; i < 12; i++) {
        p_trailing ^= (data >> i) & 1;
    }
    
    packet = (p_leading << 25) | (data << 1) | p_trailing;
    
    while (true) {
        for (int i = 25; i >= 0; i--) {
            bool bit = (packet >> i) & 1;
            uint32_t start_bit = time_us_32();
            if (bit) {
                while ((time_us_32() - start_bit) < 512) {
                    gpio_put(RFID_EMU_PIN, 1);
                    sleep_us(40);
                    gpio_put(RFID_EMU_PIN, 0);
                    sleep_us(40);
                }
            } else {
                while ((time_us_32() - start_bit) < 512) {
                    gpio_put(RFID_EMU_PIN, 1);
                    sleep_us(32);
                    gpio_put(RFID_EMU_PIN, 0);
                    sleep_us(32);
                }
            }
        }
        
        if (!g_core1_run) {
            break;
        }
        sleep_ms(1);
    }
    gpio_put(RFID_EMU_PIN, 0);
}
