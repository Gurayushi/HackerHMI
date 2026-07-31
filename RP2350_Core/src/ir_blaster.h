#ifndef IR_BLASTER_H
#define IR_BLASTER_H

#include "hardware/pio.h"
#include <stdint.h>

// Chân GPIO xuất xung hồng ngoại (Nối vào bóng LED IR TX)
#define IR_TX_PIN 22
// Cấu hình tần số mang (Carrier Frequency) thường dùng cho Tivi là 38kHz
#define IR_CARRIER_FREQ_HZ 38000

// Hàm khởi tạo module phát hồng ngoại dùng PIO
void ir_blaster_init() {
    // Để xuất được xung 38kHz chính xác, chúng ta không dùng vòng lặp for/while thông thường
    // mà sẽ lợi dụng tính năng PIO (Programmable I/O) độc quyền của Raspberry Pi RP2350.
    
    // TODO: Nạp file mã máy .pio (State Machine) vào bộ nhớ PIO0.
    // Cấu hình chân IR_TX_PIN chịu sự điều khiển của PIO0.
}

// Bắn mã Hex (Vd mã tắt Tivi Sony: 0xA90) ra ngoài
void ir_transmit_code(uint32_t hex_code, uint8_t bit_length) {
    // TODO: Đẩy mã hex_code vào hàng đợi TX FIFO của PIO.
    // PIO State Machine sẽ tự động băm mã này thành các xung sáng/tắt 38kHz.
}

#endif // IR_BLASTER_H
