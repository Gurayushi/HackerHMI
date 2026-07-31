#ifndef DWIN_UI_H
#define DWIN_UI_H

#include <stdint.h>
#include "hardware/uart.h"

// Cổng UART giao tiếp với màn hình DWIN (921600 baud)
#define DWIN_UART_ID uart0
#define DWIN_BAUD_RATE 921600

// Hàm khởi tạo UART cho DWIN
void dwin_init() {
    uart_init(DWIN_UART_ID, DWIN_BAUD_RATE);
    // TODO: Thiết lập GPIO TX/RX cho Pico 2
}

// Hàm đẩy chuỗi Text xuống ô nhớ 0x0098 của DWIN (Dùng cho SSH Terminal)
void dwin_write_text(uint16_t address, const char* text) {
    // Cấu trúc gói tin DWIN: [Header] [Length] [Command 0x82] [Address] [Data]
    uint8_t packet[256];
    packet[0] = 0x5A; // Header 1
    packet[1] = 0xA5; // Header 2
    
    // TODO: Viết logic đóng gói dữ liệu và đẩy qua uart_write_blocking()
}

// Hàm lắng nghe bàn phím ảo (Virtual Keyboard) từ DWIN
void dwin_listen_keyboard_input(char* out_buffer) {
    // Chờ nhận dữ liệu từ UART RX khi người dùng bấm nút [Lưu] trên màn hình
    // DWIN sẽ trả về chuỗi ASCII người dùng vừa gõ.
    // TODO: Viết thuật toán đọc ngắt UART.
}

#endif // DWIN_UI_H
