#include "rolling_codes.h"
#include "radio_cc1101.h"

bool cc1101_decode_somfy(const uint32_t* durations, uint16_t count, uint32_t* out_remote_id, uint16_t* out_rolling_cnt) {
    if (count < 140) return false;
    
    // Somfy RTS Manchester demodulation (TE ~ 640us)
    uint64_t raw_frame = 0;
    uint8_t bit_cnt = 0;
    
    for (uint16_t i = 0; i < count - 1 && bit_cnt < 56; i += 2) {
        uint32_t dur = durations[i] + durations[i+1];
        if (dur > 1000 && dur < 1500) {
            raw_frame = (raw_frame << 1) | (durations[i] > durations[i+1] ? 1 : 0);
            bit_cnt++;
        }
    }
    
    if (bit_cnt < 56) return false;
    
    *out_remote_id = (uint32_t)((raw_frame >> 16) & 0xFFFFFF);
    *out_rolling_cnt = (uint16_t)(raw_frame & 0xFFFF);
    return true;
}

bool cc1101_decode_secplus(const uint32_t* durations, uint16_t count, uint32_t* out_id) {
    if (count < 80) return false;
    
    uint32_t code = 0;
    for (uint16_t i = 0; i < 39 && (i * 2) < count; i += 2) {
        code = (code << 1) | (durations[i] > 500 ? 1 : 0);
    }
    
    *out_id = code;
    return true;
}
