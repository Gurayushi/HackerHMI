#ifndef DWIN_UI_H
#define DWIN_UI_H

#include <stdint.h>
#include <string.h>
#include "hardware/uart.h"

// Cổng UART giao tiếp với màn hình DWIN (921600 baud)
#define DWIN_UART_ID uart0
#define DWIN_BAUD_RATE 921600
#define DWIN_TX_PIN 0
#define DWIN_RX_PIN 1

// Hàm khởi tạo UART cho DWIN
static inline void dwin_init() {
    uart_init(DWIN_UART_ID, DWIN_BAUD_RATE);
    gpio_set_function(DWIN_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(DWIN_RX_PIN, GPIO_FUNC_UART);
}

#include "pico/mutex.h"
extern mutex_t dwin_uart_mutex;

// Hàm đẩy chuỗi Text xuống DWIN HMI
static inline void dwin_write_text(uint16_t address, const char* text) {
    size_t len = strlen(text);
    if (len > 240) len = 240; 

    uint8_t packet[256];
    packet[0] = 0x5A; // Header 1
    packet[1] = 0xA5; // Header 2
    packet[2] = 3 + len; // Length
    packet[3] = 0x82; // Command: Write Variable
    packet[4] = (address >> 8) & 0xFF; // Address High
    packet[5] = address & 0xFF; // Address Low
    
    memcpy(&packet[6], text, len);
    
    mutex_enter_blocking(&dwin_uart_mutex);
    uart_write_blocking(DWIN_UART_ID, packet, 6 + len);
    mutex_exit(&dwin_uart_mutex);
}

// Hàm chuyển trang hiển thị trên DWIN HMI (Địa chỉ thanh ghi hệ thống 0x0084)
static inline void dwin_switch_page(uint16_t page_id) {
    uint8_t packet[8];
    packet[0] = 0x5A;
    packet[1] = 0xA5;
    packet[2] = 0x04; // Length
    packet[3] = 0x82; // Write Command
    packet[4] = 0x00; // Address High
    packet[5] = 0x84; // Address Low (system register for page switching)
    packet[6] = (page_id >> 8) & 0xFF;
    packet[7] = page_id & 0xFF;
    
    mutex_enter_blocking(&dwin_uart_mutex);
    uart_write_blocking(DWIN_UART_ID, packet, 8);
    mutex_exit(&dwin_uart_mutex);
}

// Hàm lắng nghe bàn phím ảo và nút bấm từ DWIN HMI
static inline void dwin_listen_keyboard_input(char* out_buffer) {
    out_buffer[0] = '\0';
    if (!uart_is_readable(DWIN_UART_ID)) {
        return;
    }

    // Đọc header 5A A5
    uint8_t h1 = uart_getc(DWIN_UART_ID);
    if (h1 != 0x5A) return;
    uint8_t h2 = uart_getc(DWIN_UART_ID);
    if (h2 != 0xA5) return;

    // Đọc độ dài
    uint8_t len = uart_getc(DWIN_UART_ID);
    if (len < 3) return;

    // Đọc lệnh (0x83 hoặc 0x82)
    uint8_t cmd = uart_getc(DWIN_UART_ID);
    
    // Đọc địa chỉ
    uint8_t addr_h = uart_getc(DWIN_UART_ID);
    uint8_t addr_l = uart_getc(DWIN_UART_ID);
    uint16_t address = (addr_h << 8) | addr_l;

    // Đọc số lượng Word dữ liệu
    uint8_t word_count = uart_getc(DWIN_UART_ID);
    uint8_t data_len = (len - 4); // 4 bytes đã đọc: cmd, addr_h, addr_l, word_count

    uint8_t data[256];
    for (int i = 0; i < data_len; i++) {
        data[i] = uart_getc(DWIN_UART_ID);
    }
    data[data_len] = '\0';

    // Phân tích gói tin:
    if (cmd == 0x83 || cmd == 0x82) {
        // Lấy chuỗi ASCII trả về
        if (data_len > 0) {
            strncpy(out_buffer, (char*)data, data_len);
            out_buffer[data_len] = '\0';
        }
    }
}

// Hàm đẩy giá trị 16-bit Integer xuống DWIN HMI
static inline void dwin_write_val(uint16_t address, uint16_t value) {
    uint8_t packet[8];
    packet[0] = 0x5A;
    packet[1] = 0xA5;
    packet[2] = 0x04; // Length
    packet[3] = 0x82; // Write Command
    packet[4] = (address >> 8) & 0xFF;
    packet[5] = address & 0xFF;
    packet[6] = (value >> 8) & 0xFF;
    packet[7] = value & 0xFF;
    
    uart_write_blocking(DWIN_UART_ID, packet, 8);
}

#endif // DWIN_UI_H
