#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Custom Headers (UI & Giao tiếp)
#include "dwin_ui.h"
#include "terminal.h"
#include "hid_device.h"

// Vũ khí Hacker
#include "badusb.h"
#include "radio_cc1101.h"
#include "rfid_nfc.h"
#include "ir_blaster.h"

// Khai báo SPI0 kết nối với ESP32-C5
#define C5_SPI_PORT spi0
#define C5_PIN_MISO 16
#define C5_PIN_CS   17
#define C5_PIN_SCK  18
#define C5_PIN_MOSI 19

// Biến toàn cục chứa bộ đệm Terminal 30KB
RingBuffer terminal_buffer;

void init_spi_for_c5() {
    spi_init(C5_SPI_PORT, 1000 * 1000); // Tốc độ 1 MHz
    gpio_set_function(C5_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(C5_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(C5_PIN_MOSI, GPIO_FUNC_SPI);
    
    // Khởi tạo chân CS
    gpio_init(C5_PIN_CS);
    gpio_set_dir(C5_PIN_CS, GPIO_OUT);
    gpio_put(C5_PIN_CS, 1);
}

// Hàm đẩy Cấu hình (Nhập từ bàn phím HMI) sang ESP32-C5 để lưu NVS
void push_config_to_c5(const char* config_str) {
    gpio_put(C5_PIN_CS, 0); // Kéo CS xuống LOW để chọn C5
    spi_write_blocking(C5_SPI_PORT, (const uint8_t*)config_str, strlen(config_str));
    gpio_put(C5_PIN_CS, 1); // Đẩy CS lên HIGH
}

int main() {
    stdio_init_all();
    
    // --- KHỞI TẠO HỆ THỐNG CƠ BẢN ---
    term_init(&terminal_buffer);
    dwin_init();
    init_spi_for_c5();
    
    // --- KHỞI TẠO VŨ KHÍ HACKER ---
    badusb_init();
    cc1101_init(433.92); // Mặc định tần số mở cổng 433MHz
    rfid_nfc_init();
    ir_blaster_init();
    
    dwin_write_text(0x0098, "HackerHMI - Booting RP2350 Core...\n");
    dwin_write_text(0x0098, "[+] All Hacker Modules Loaded.\n");

    while (true) {
        // LUỒNG 1: Giữ kết nối BadUSB luôn sống
        badusb_task();

        // LUỒNG 2: Xử lý thao tác cảm ứng từ người dùng (UART)
        char hmi_input[128] = {0};
        dwin_listen_keyboard_input(hmi_input);
        
        // --- XỬ LÝ LỆNH TỪ MÀN HÌNH CẢM ỨNG ---
        if (strlen(hmi_input) > 0) {
            if (strncmp(hmi_input, "CMD_IR_FIRE", 11) == 0) {
                // Nút "Tắt Tivi" được bấm
                ir_transmit_code(0xA90, 12); 
            }
            else if (strncmp(hmi_input, "CMD_RF_OPEN", 11) == 0) {
                // Nút "Mở cổng" được bấm
                uint8_t payload[] = {0x01, 0x02, 0x03}; // Mã giả lập
                cc1101_transmit_signal(payload, 3);
            }
            // --- CÁC NÚT BẤM ĐỔI HỆ ĐIỀU HÀNH CHO TOUCHPAD ---
            else if (strncmp(hmi_input, "CMD_OS_WIN", 10) == 0) {
                hid_set_os(0); dwin_write_text(0x0098, "[+] Switched to Windows Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_MAC", 10) == 0) {
                hid_set_os(1); dwin_write_text(0x0098, "[+] Switched to MacOS Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_LINUX", 12) == 0) {
                hid_set_os(2); dwin_write_text(0x0098, "[+] Switched to Linux Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_ANDROID", 14) == 0) {
                hid_set_os(3); dwin_write_text(0x0098, "[+] Switched to Android Mode\n");
            }
            // -------------------------------------------------
            else {
                // Các chuỗi gõ phím thông thường (IP, Username) sẽ gửi sang C5 để lưu
                push_config_to_c5(hmi_input);
                dwin_write_text(0x0098, "Config sent to ESP32-C5 for NVS Storage.\n");
            }
        }
        
        // LUỒNG 3: Quét thẻ từ liên tục
        char rfid_uid[32] = {0};
        if (rdm6300_read_card(rfid_uid)) {
            // Hiển thị UID thẻ lên màn hình DWIN
            dwin_write_text(0x0098, "Thẻ từ vừa quét: ");
            dwin_write_text(0x0098, rfid_uid);
            dwin_write_text(0x0098, "\n");
        }
        
        sleep_ms(5);
    }
}
