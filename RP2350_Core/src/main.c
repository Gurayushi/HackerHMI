#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

// Custom Headers (UI & Giao tiếp)
#include "dwin_ui.h"
#include "terminal.h"
#include "hid_device.h"

// Vũ khí Hacker
#include "badusb.h"
#include "radio_cc1101.h"
#include "rfid_nfc.h"
#include "ir_blaster.h"
#include "esp32_flasher.h"

// Khai báo SPI0 kết nối với ESP32-C5
#define C5_SPI_PORT      spi0
#define C5_PIN_MISO      16
#define C5_PIN_CS        17
#define C5_PIN_SCK       18
#define C5_PIN_MOSI      19
#define C5_PIN_HANDSHAKE 22

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

    // Khởi tạo chân Handshake lắng nghe ngắt từ ESP32-C5
    gpio_init(C5_PIN_HANDSHAKE);
    gpio_set_dir(C5_PIN_HANDSHAKE, GPIO_IN);
    gpio_pull_up(C5_PIN_HANDSHAKE);
}

// Hàm đẩy Cấu hình (Nhập từ bàn phím HMI) sang ESP32-C5 để lưu NVS
void push_config_to_c5(const char* config_str) {
    gpio_put(C5_PIN_CS, 0); // Kéo CS xuống LOW để chọn C5
    spi_write_blocking(C5_SPI_PORT, (const uint8_t*)config_str, strlen(config_str));
    gpio_put(C5_PIN_CS, 1); // Đẩy CS lên HIGH
}

int main() {
    // --- ÉP XUNG LÊN 300MHz (Gấp đôi mặc định 150MHz) ---
    // Nâng nhẹ điện áp lõi để giữ hệ thống chạy ổn định, không lỗi dữ liệu
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(2);
    set_sys_clock_khz(300000, true);
    
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
    esp32_flasher_init();
    
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
            // --- STREAM DECK: CÁC PHÍM TẮT ĐA NHIỆM (MACRO MULTI-ACTION) ---
            else if (strncmp(hmi_input, "CMD_MACRO_1", 11) == 0) {
                hid_run_macro(1); dwin_write_text(0x0098, "[+] Executing Macro 1 (Work Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_2", 11) == 0) {
                hid_run_macro(2); dwin_write_text(0x0098, "[+] Executing Macro 2 (Gaming Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_3", 11) == 0) {
                hid_run_macro(3); dwin_write_text(0x0098, "[+] Executing Macro 3 (Dev Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_4", 11) == 0) {
                hid_run_macro(4); dwin_write_text(0x0098, "[+] Executing Custom Macro\n");
            }
            // --- STREAM DECK: ĐIỀU CHỈNH ÂM LƯỢNG / ĐỘ SÁNG ---
            else if (strncmp(hmi_input, "VOL_VAL:", 8) == 0) {
                uint8_t vol = atoi(hmi_input + 8);
                hid_set_volume(vol);
            }
            else if (strncmp(hmi_input, "BRIGHT_VAL:", 11) == 0) {
                uint8_t bright = atoi(hmi_input + 11);
                hid_set_brightness(bright);
            }
            // --- BẮT ĐẦU CHẾ ĐỘ NẠP FIRMWARE CỨU HỘ CHO ESP32-C5 ---
            else if (strncmp(hmi_input, "CMD_FLASH_MODE", 14) == 0) {
                dwin_write_text(0x0098, "\n[!] ENTERING USB-TO-UART FLASH MODE...\n");
                dwin_write_text(0x0098, "[!] Connect PC to RP2350 USB and run esptool.\n");
                esp_enter_bootloader();
                esp_uart_bridge_task();
            }
            // --- BẮT ĐẦU CHẾ ĐỘ NẠP OTA KHÔNG DÂY QUA WI-FI ---
            else if (strncmp(hmi_input, "CMD_START_OTA", 13) == 0) {
                push_config_to_c5("start_ota=1");
                dwin_write_text(0x0098, "[+] Requested ESP32-C5 to launch OTA Web Server...\n");
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
        
        // LUỒNG 4: Nhận dữ liệu phản hồi từ ESP32-C5 qua đường truyền SPI
        if (gpio_get(C5_PIN_HANDSHAKE) == 0) { // ESP32-C5 kéo chân Handshake xuống LOW để báo có tin nhắn
            uint8_t spi_rx_buf[128] = {0};
            
            // Kéo CS xuống LOW để bắt đầu đọc giao tiếp SPI
            gpio_put(C5_PIN_CS, 0);
            spi_read_blocking(C5_SPI_PORT, 0x00, spi_rx_buf, sizeof(spi_rx_buf));
            gpio_put(C5_PIN_CS, 1);
            
            // Xử lý các lệnh nhận được từ ESP32-C5
            if (strncmp((char*)spi_rx_buf, "START_DWIN_FLASH", 16) == 0) {
                dwin_write_text(0x0098, "[!] Nhận lệnh cập nhật giao diện HMI qua Wi-Fi...\n");
                // RP2350 tạm thời dừng lại để nhận dữ liệu ảnh từ ESP32 và ghi xuống Flash của DWIN
            }
            else if (strncmp((char*)spi_rx_buf, "APP:", 4) == 0) {
                // Nhận lệnh chuyển đổi giao diện Smart Profile qua BLE
                dwin_write_text(0x0098, "[+] BLE Smart Profile Event: ");
                dwin_write_text(0x0098, (char*)spi_rx_buf + 4);
                dwin_write_text(0x0098, "\n");
            }
        }
        
        sleep_ms(5);
    }
}
