#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Custom Headers
#include "dwin_ui.h"
#include "terminal.h"

// Khai báo SPI kết nối với ESP32-C6
#define C6_SPI_PORT spi0
#define C6_PIN_MISO 16
#define C6_PIN_CS   17
#define C6_PIN_SCK  18
#define C6_PIN_MOSI 19

// Biến toàn cục chứa bộ đệm Terminal 30KB
RingBuffer terminal_buffer;

void init_spi_master() {
    spi_init(C6_SPI_PORT, 1000 * 1000); // Tốc độ 1 MHz
    gpio_set_function(C6_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(C6_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(C6_PIN_MOSI, GPIO_FUNC_SPI);
    
    // Khởi tạo Chip Select (CS)
    gpio_init(C6_PIN_CS);
    gpio_set_dir(C6_PIN_CS, GPIO_OUT);
    gpio_put(C6_PIN_CS, 1);
}

// Hàm đẩy Cấu hình (Nhập từ bàn phím HMI) sang ESP32-C6 để lưu NVS
void push_config_to_c6(const char* config_str) {
    gpio_put(C6_PIN_CS, 0); // Kéo CS xuống LOW để chọn C6
    spi_write_blocking(C6_SPI_PORT, (const uint8_t*)config_str, strlen(config_str));
    gpio_put(C6_PIN_CS, 1); // Đẩy CS lên HIGH
}

int main() {
    stdio_init_all();
    
    // 1. Khởi tạo thuật toán Terminal Ring-Buffer 30KB
    term_init(&terminal_buffer);
    
    // 2. Khởi tạo Giao tiếp HMI (UART)
    dwin_init();
    
    // 3. Khởi tạo Giao tiếp Não phụ ESP32-C6 (SPI)
    init_spi_master();
    
    // Gửi màn hình khởi động lên HMI
    dwin_write_text(0x0098, "HackerHMI - Booting RP2350 Core...\n");

    while (true) {
        // LUỒNG 1: Xử lý dữ liệu SSH từ C6 bắn qua (SPI Slave)
        // ...
        
        // LUỒNG 2: Xử lý thao tác cảm ứng từ người dùng (UART)
        char hmi_input[128] = {0};
        dwin_listen_keyboard_input(hmi_input);
        
        if (strlen(hmi_input) > 0) {
            // Nếu người dùng vừa gõ phím lưu cấu hình SSH/Wi-Fi
            // Bắn dữ liệu đó sang C6 để lưu vào Flash NVS
            push_config_to_c6(hmi_input);
            dwin_write_text(0x0098, "Config sent to ESP32-C6 for NVS Storage.\n");
        }
        
        sleep_ms(10);
    }
}
