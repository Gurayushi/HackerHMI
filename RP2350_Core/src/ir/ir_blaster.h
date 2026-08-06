#ifndef IR_BLASTER_H
#define IR_BLASTER_H

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include "ir_carrier.pio.h"
#include "tv_buster_database.h"
#include "ir_protocols.h"

// Chân GPIO xuất xung hồng ngoại (Nối vào bóng LED IR TX)
#define IR_TX_PIN 22

extern void dwin_write_text(uint16_t address, const char* text);

static PIO ir_pio = pio0;
static uint ir_sm = 0;
static uint ir_offset = 0;
static bool ir_initialized = false;

// Hàm khởi tạo module phát hồng ngoại dùng PIO
static inline void ir_blaster_init() {
    if (ir_initialized) return;
    
    // Tìm PIO block và state machine rảnh rỗi để nạp chương trình
    ir_pio = pio0;
    if (pio_can_add_program(ir_pio, &ir_carrier_program)) {
        ir_offset = pio_add_program(ir_pio, &ir_carrier_program);
    } else {
        ir_pio = pio1;
        if (pio_can_add_program(ir_pio, &ir_carrier_program)) {
            ir_offset = pio_add_program(ir_pio, &ir_carrier_program);
        } else {
            return; // Lỗi: Không thể nạp chương trình PIO
        }
    }
    
    ir_sm = pio_claim_unused_sm(ir_pio, true);
    ir_carrier_program_init(ir_pio, ir_sm, ir_offset, IR_TX_PIN);
    ir_initialized = true;
}

// Bắn chuỗi xung thô chứa thời lượng các sườn xung bật/tắt (microseconds)
static inline void ir_transmit_raw(const uint32_t *data, uint16_t length) {
    if (!ir_initialized) {
        ir_blaster_init();
    }
    if (!ir_initialized) return;

    for (uint16_t i = 0; i < length; i++) {
        uint32_t us = data[i];
        // Đổi từ micro giây sang số lượng chu kỳ sóng mang 38kHz:
        // cycles = (us * 38) / 1000
        uint32_t cycles = (us * 38) / 1000;
        if (cycles == 0) cycles = 1; // Tối thiểu 1 chu kỳ
        
        // Trạng thái: Chẵn = phát sóng mang (1), Lẻ = tắt (0)
        uint32_t state = (i % 2 == 0) ? 1 : 0;
        
        // Đóng gói: 31 bits cao là số chu kỳ, 1 bit thấp là trạng thái
        uint32_t pio_val = (cycles << 1) | state;
        
        pio_sm_put_blocking(ir_pio, ir_sm, pio_val);
    }
    
    // Gửi thêm một khoảng nghỉ cuối cùng 2ms để bảo đảm LED được tắt hẳn
    pio_sm_put_blocking(ir_pio, ir_sm, ((2000 * 38) / 1000) << 1 | 0);
}

// Bắn phá tắt tivi vạn năng (TV Buster)
static inline void ir_tv_buster_fire() {
    dwin_write_text(0x0098, "\n[!] TV BUSTER: Launching universal TV attack...\n");
    
    uint32_t timings[80];
    for (int i = 0; i < TV_BUSTER_DB_SIZE; i++) {
        const tv_code_t *code = &tv_buster_database[i];
        
        char log_msg[64];
        snprintf(log_msg, sizeof(log_msg), "[+] Blasting %s TV Power code...\n", code->brand);
        dwin_write_text(0x0098, log_msg);
        
        uint16_t len = 0;
        switch (code->protocol) {
            case IR_PROTO_NEC:
                len = ir_encode_nec(code->address, code->command, timings);
                break;
            case IR_PROTO_SIRC_12:
                len = ir_encode_sirc_12(code->address, code->command, timings);
                break;
            case IR_PROTO_RC5:
                len = ir_encode_rc5(code->address, code->command, timings);
                break;
        }
        
        if (len > 0) {
            ir_transmit_raw(timings, len);
        }
        
        // Khoảng nghỉ giữa các hãng 150ms để đầu thu tivi xử lý kịp
        sleep_ms(150);
    }
    
    dwin_write_text(0x0098, "[+] TV BUSTER: Completed. All codes sent!\n");
}

#endif // IR_BLASTER_H
