#ifndef ESP32_FLASHER_H
#define ESP32_FLASHER_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#define ESP32_UART_PORT   uart1
#define ESP32_BAUD_RATE   115200
#define ESP32_PIN_TX      8
#define ESP32_PIN_RX      9
#define ESP32_PIN_EN      20
#define ESP32_PIN_IO9     21

// Khởi tạo các chân GPIO điều khiển và cổng UART kết nối với ESP32
void esp32_flasher_init() {
    // Cấu hình UART1 kết nối với ESP32 UART0
    uart_init(ESP32_UART_PORT, ESP32_BAUD_RATE);
    gpio_set_function(ESP32_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(ESP32_PIN_RX, GPIO_FUNC_UART);

    // Cấu hình chân EN (Reset)
    gpio_init(ESP32_PIN_EN);
    gpio_set_dir(ESP32_PIN_EN, GPIO_OUT);
    gpio_put(ESP32_PIN_EN, 1); // Mặc định giữ mức cao

    // Cấu hình chân IO9 (Strapping pin boot của dòng chip RISC-V như C5/C6)
    gpio_init(ESP32_PIN_IO9);
    gpio_set_dir(ESP32_PIN_IO9, GPIO_OUT);
    gpio_put(ESP32_PIN_IO9, 1); // Mặc định giữ mức cao
}

// Hàm giả lập tự động kích hoạt chế độ nạp (Strapping sequence)
void esp32_enter_bootloader() {
    gpio_put(ESP32_PIN_IO9, 0); // Kéo IO9 xuống LOW để đưa vào chế độ ROM Bootloader
    sleep_ms(10);
    
    gpio_put(ESP32_PIN_EN, 0);  // Reset ESP32 bằng cách kéo EN xuống LOW
    sleep_ms(50);
    
    gpio_put(ESP32_PIN_EN, 1);  // Đưa EN lên HIGH để chip khởi chạy ở trạng thái Bootloader
    sleep_ms(50);
    
    gpio_put(ESP32_PIN_IO9, 1); // Thả chân IO9 về trạng thái bình thường
}

// Vòng lặp chuyển tiếp dữ liệu tốc độ cao (Baudrate 921600 để nạp nhanh)
void esp32_uart_bridge_task() {
    uart_set_baudrate(ESP32_UART_PORT, 921600);
    
    while (true) {
        // Đọc dữ liệu từ cổng USB CDC của PC và đẩy sang ESP32
        int usb_char = getchar_timeout_us(0);
        if (usb_char != PICO_ERROR_TIMEOUT) {
            uart_putc_raw(ESP32_UART_PORT, (uint8_t)usb_char);
        }

        // Đọc dữ liệu từ ESP32 UART và đẩy ngược lại về PC qua cổng USB
        if (uart_is_readable(ESP32_UART_PORT)) {
            uint8_t esp_char = uart_getc(ESP32_UART_PORT);
            putchar_raw(esp_char);
        }
    }
}

#endif // ESP32_FLASHER_H
