#include "ir_protocols.h"

// Hàm mã hóa và truyền tin theo giao thức NEC
uint16_t ir_encode_nec(uint32_t address, uint32_t command, uint32_t *out_timings) {
    uint16_t idx = 0;
    
    // 1. Khởi động (Leading Burst): 9000us Mark, 4500us Space
    out_timings[idx++] = 9000;
    out_timings[idx++] = 4500;
    
    // 2. Mã hóa 32-bit payload: Addr LSB (8-bit) + Addr MSB (8-bit) + Cmd (8-bit) + ~Cmd (8-bit)
    uint32_t frame = (address & 0xFFFF) | ((command & 0xFF) << 16) | ((~(command & 0xFF) & 0xFF) << 24);
    
    for (int i = 0; i < 32; i++) {
        out_timings[idx++] = 562; // Bit Mark
        if (frame & (1UL << i)) {
            out_timings[idx++] = 1687; // Logic 1 Space
        } else {
            out_timings[idx++] = 562;  // Logic 0 Space
        }
    }
    
    // 3. Kết thúc (Stop Bit): 562us Mark
    out_timings[idx++] = 562;
    out_timings[idx++] = 10000; // Nghỉ sau truyền
    
    return idx;
}

// Hàm mã hóa và truyền tin theo giao thức Sony SIRC 12-bit
uint16_t ir_encode_sirc_12(uint32_t address, uint32_t command, uint32_t *out_timings) {
    uint16_t idx = 0;
    
    // 1. Khởi động: 2400us Mark, 600us Space
    out_timings[idx++] = 2400;
    out_timings[idx++] = 600;
    
    // 2. Mã hóa 12-bit payload: 7-bit command + 5-bit address
    uint32_t frame = (command & 0x7F) | ((address & 0x1F) << 7);
    
    for (int i = 0; i < 12; i++) {
        if (frame & (1 << i)) {
            out_timings[idx++] = 1200; // Logic 1 Mark
        } else {
            out_timings[idx++] = 600;  // Logic 0 Mark
        }
        out_timings[idx++] = 600; // Khoảng nghỉ giữa các bit
    }
    
    // 3. Kết thúc
    out_timings[idx++] = 0; // Không có mark
    out_timings[idx++] = 10000;
    
    return idx;
}

// Hàm mã hóa và truyền tin theo giao thức Philips RC5 (Manchester coding)
uint16_t ir_encode_rc5(uint32_t address, uint32_t command, uint32_t *out_timings) {
    uint16_t idx = 0;
    
    // 14 bits của RC5: 2 Start bits (1, 1), 1 Toggle (0), 5-bit Addr, 6-bit Cmd
    uint32_t frame = 0x3000 | ((address & 0x1F) << 6) | (command & 0x3F);
    
    // Manchester encoding: mỗi bit chia làm 2 nửa trạng thái, mỗi nửa rộng 889us
    uint8_t states[28];
    for (int i = 0; i < 14; i++) {
        uint8_t bit = (frame >> (13 - i)) & 1;
        if (bit) {
            // Logic 1: Nửa đầu Space (0), Nửa sau Mark (1)
            states[i * 2] = 0;
            states[i * 2 + 1] = 1;
        } else {
            // Logic 0: Nửa đầu Mark (1), Nửa sau Space (0)
            states[i * 2] = 1;
            states[i * 2 + 1] = 0;
        }
    }
    
    // Bỏ qua các nửa bit tĩnh 0 ở đầu tiên (nếu có)
    int start_index = 0;
    while (start_index < 28 && states[start_index] == 0) {
        start_index++;
    }
    
    uint8_t current_state = 1; // Bắt đầu bằng Mark (1)
    uint32_t duration = 0;
    
    for (int i = start_index; i < 28; i++) {
        if (states[i] == current_state) {
            duration += 889;
        } else {
            out_timings[idx++] = duration;
            duration = 889;
            current_state = states[i];
        }
    }
    out_timings[idx++] = duration;
    
    // Nếu kết thúc bằng xung cao, thêm khoảng nghỉ 10ms để tắt hẳn
    if (current_state == 1) {
        out_timings[idx++] = 10000;
    }
    
    return idx;
}
