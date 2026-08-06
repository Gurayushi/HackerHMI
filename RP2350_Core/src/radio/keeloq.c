#include "keeloq.h"

#define NLF_CONSTANT 0x3A5C742E

static uint32_t keeloq_bit(uint32_t data, uint64_t key, int i) {
    uint32_t key_bit = (key >> (i & 63)) & 1;
    uint32_t nlf_index = ((data >> 31) & 1) << 4 |
                        ((data >> 26) & 1) << 3 |
                        ((data >> 20) & 1) << 2 |
                        ((data >> 9)  & 1) << 1 |
                        ((data >> 1)  & 1);
    uint32_t nlf_bit = (NLF_CONSTANT >> nlf_index) & 1;
    uint32_t feedback = ((data >> 16) & 1) ^ (data & 1) ^ nlf_bit ^ key_bit;
    return (data >> 1) | (feedback << 31);
}

uint32_t keeloq_decrypt(uint32_t data, uint64_t key) {
    for (int i = 527; i >= 0; i--) {
        uint32_t key_bit = (key >> (i & 63)) & 1;
        uint32_t nlf_index = ((data >> 30) & 1) << 4 |
                            ((data >> 25) & 1) << 3 |
                            ((data >> 19) & 1) << 2 |
                            ((data >> 8)  & 1) << 1 |
                            ((data >> 0)  & 1);
        uint32_t nlf_bit = (NLF_CONSTANT >> nlf_index) & 1;
        uint32_t feedback = ((data >> 31) & 1) ^ ((data >> 15) & 1) ^ nlf_bit ^ key_bit;
        data = (data << 1) | (feedback & 1);
    }
    return data;
}

uint32_t keeloq_encrypt(uint32_t data, uint64_t key) {
    for (int i = 0; i < 528; i++) {
        data = keeloq_bit(data, key, i);
    }
    return data;
}

bool keeloq_decode_buffer(const uint32_t* durations, uint16_t count, uint64_t manuf_key, uint32_t* out_serial, uint16_t* out_btn, uint16_t* out_cnt) {
    if (count < 130) return false;
    
    // Find PWM sync (TE ~ 400us, header ~ 12*TE)
    uint64_t raw_bits = 0;
    uint8_t bit_cnt = 0;
    
    for (uint16_t i = 0; i < count - 1 && bit_cnt < 66; i += 2) {
        uint32_t high = durations[i];
        uint32_t low = durations[i+1];
        if (high == 0 || low == 0) continue;
        
        if (high > (low * 15 / 10)) { // High > 1.5 * Low -> '0'
            raw_bits = (raw_bits << 1);
            bit_cnt++;
        } else if (low > (high * 15 / 10)) { // Low > 1.5 * High -> '1'
            raw_bits = (raw_bits << 1) | 1;
            bit_cnt++;
        }
    }
    
    if (bit_cnt < 66) return false;
    
    uint32_t enc = (uint32_t)(raw_bits >> 34);
    uint32_t serial = (uint32_t)((raw_bits >> 6) & 0x0FFFFFFF);
    uint8_t btn = (uint8_t)(raw_bits & 0x3F);
    
    uint32_t dec = keeloq_decrypt(enc, manuf_key);
    
    *out_serial = serial;
    *out_btn = btn;
    *out_cnt = (uint16_t)(dec & 0xFFFF);
    return true;
}
