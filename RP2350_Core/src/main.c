#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "ui/dwin_ui.h"
#include "terminal/terminal.h"
#include "usb/hid_device.h"

// Vũ khí Hacker
#include "usb/badusb.h"
#include "radio/radio_cc1101.h"
#include "rfid/rfid_nfc.h"
#include "ir/ir_blaster.h"
#include "flasher/esp32_flasher.h"

// Khai báo SPI0 kết nối với ESP32-C5
#define C5_SPI_PORT      spi0
#define C5_PIN_MISO      16
#define C5_PIN_CS        17
#define C5_PIN_SCK       18
#define C5_PIN_MOSI      19
#define C5_PIN_HANDSHAKE 22

// Biến toàn cục chứa bộ đệm Terminal 30KB
RingBuffer terminal_buffer;

// Các cờ trạng thái quản lý đa nhiệm (Multitasking state flags)
volatile bool g_badusb_active   = false;
volatile bool g_rfid_active     = true; // Mặc định chạy scan nền
volatile bool g_radio_active    = false;
volatile bool g_ir_active       = false;
volatile bool g_flasher_active  = false;

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
        if (g_badusb_active) {
            badusb_task();
        }

        // LUỒNG 2: Xử lý thao tác cảm ứng từ người dùng (UART)
        char hmi_input[128] = {0};
        dwin_listen_keyboard_input(hmi_input);
        
        // --- XỬ LÝ LỆNH TỪ MÀN HÌNH CẢM ỨNG ---
        if (strlen(hmi_input) > 0) {
            if (strncmp(hmi_input, "CMD_IR_FIRE", 11) == 0) {
                // Nút "Tắt Tivi" được bấm
                g_ir_active = true;
                ir_transmit_code(0xA90, 12); 
            }
            else if (strncmp(hmi_input, "CMD_RF_OPEN", 11) == 0) {
                // Nút "Mở cổng" được bấm
                g_radio_active = true;
                uint8_t payload[] = {0x01, 0x02, 0x03}; // Mã giả lập
                cc1101_transmit_signal(payload, 3);
            }
            // --- CÁC NÚT BẤM ĐỔI HỆ ĐIỀU HÀNH CHO TOUCHPAD ---
            else if (strncmp(hmi_input, "CMD_OS_WIN", 10) == 0) {
                g_badusb_active = true;
                hid_set_os(0); dwin_write_text(0x0098, "[+] Switched to Windows Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_MAC", 10) == 0) {
                g_badusb_active = true;
                hid_set_os(1); dwin_write_text(0x0098, "[+] Switched to MacOS Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_LINUX", 12) == 0) {
                g_badusb_active = true;
                hid_set_os(2); dwin_write_text(0x0098, "[+] Switched to Linux Mode\n");
            }
            else if (strncmp(hmi_input, "CMD_OS_ANDROID", 14) == 0) {
                g_badusb_active = true;
                hid_set_os(3); dwin_write_text(0x0098, "[+] Switched to Android Mode\n");
            }
            // --- STREAM DECK: CÁC PHÍM TẮT ĐA NHIỆM (MACRO MULTI-ACTION) ---
            else if (strncmp(hmi_input, "CMD_MACRO_1", 11) == 0) {
                g_badusb_active = true;
                hid_run_macro(1); dwin_write_text(0x0098, "[+] Executing Macro 1 (Work Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_2", 11) == 0) {
                g_badusb_active = true;
                hid_run_macro(2); dwin_write_text(0x0098, "[+] Executing Macro 2 (Gaming Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_3", 11) == 0) {
                g_badusb_active = true;
                hid_run_macro(3); dwin_write_text(0x0098, "[+] Executing Macro 3 (Dev Mode)\n");
            }
            else if (strncmp(hmi_input, "CMD_MACRO_4", 11) == 0) {
                g_badusb_active = true;
                hid_run_macro(4); dwin_write_text(0x0098, "[+] Executing Custom Macro\n");
            }
            // --- STREAM DECK: ĐIỀU CHỈNH ÂM LƯỢNG / ĐỘ SÁNG ---
            else if (strncmp(hmi_input, "VOL_VAL:", 8) == 0) {
                g_badusb_active = true;
                uint8_t vol = atoi(hmi_input + 8);
                hid_set_volume(vol);
            }
            else if (strncmp(hmi_input, "BRIGHT_VAL:", 11) == 0) {
                g_badusb_active = true;
                uint8_t bright = atoi(hmi_input + 11);
                hid_set_brightness(bright);
            }
            // --- BẮT ĐẦU CHẾ ĐỘ NẠP FIRMWARE CỨU HỘ CHO ESP32-C5 ---
            else if (strncmp(hmi_input, "CMD_FLASH_MODE", 14) == 0) {
                g_flasher_active = true;
                dwin_write_text(0x0098, "\n[!] ENTERING USB-TO-UART FLASH MODE...\n");
                dwin_write_text(0x0098, "[!] Connect PC to RP2350 USB and run esptool.\n");
                esp32_enter_bootloader();
                esp32_uart_bridge_task();
            }
            // --- BẮT ĐẦU CHẾ ĐỘ NẠP OTA KHÔNG DÂY QUA WI-FI ---
            else if (strncmp(hmi_input, "CMD_START_OTA", 13) == 0) {
                push_config_to_c5("start_ota=1");
                dwin_write_text(0x0098, "[+] Requested ESP32-C5 to launch OTA Web Server...\n");
            }
            // --- CÁC PHÍM ĐIỀU KHIỂN WI-FI DEAUTHER ---
            else if (strncmp(hmi_input, "CMD_DEAUTH_SCAN", 15) == 0) {
                push_config_to_c5("scan");
                dwin_write_text(0x0470, "Scanning...");
            }
            else if (strncmp(hmi_input, "CMD_DEAUTH_START", 16) == 0) {
                push_config_to_c5("start 0");
                dwin_write_text(0x0470, "RUNNING - Targeted Mode");
            }
            else if (strncmp(hmi_input, "CMD_DEAUTH_STOP", 15) == 0) {
                push_config_to_c5("stop");
                dwin_write_text(0x0470, "STOPPED");
            }
            else if (strncmp(hmi_input, "CMD_DEAUTH_NUKE", 15) == 0) {
                push_config_to_c5("nuke 30");
                dwin_write_text(0x0470, "RUNNING - Nuke Mode (30s)");
            }
            else if (strncmp(hmi_input, "DEAUTH_SEL:", 11) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "sel %s", hmi_input + 11);
                push_config_to_c5(cmd);
            }
            else if (strncmp(hmi_input, "DEAUTH_MODE:", 12) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "mode %s", hmi_input + 12);
                push_config_to_c5(cmd);
                dwin_write_text(0x0490, hmi_input + 12);
            }
            else if (strncmp(hmi_input, "CMD_DEAUTH_HIDE", 15) == 0) {
                // Quay về Trang chủ (Trang 0) nhưng để tác vụ chạy nền
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Wi-Fi Deauther đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_DEAUTH_KILL", 15) == 0) {
                // Tắt hẳn cuộc tấn công và quay về Trang chủ
                push_config_to_c5("stop");
                dwin_write_text(0x0470, "STOPPED");
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Wi-Fi Deauther đã được tắt hoàn toàn.\n");
            }
            // --- HIDE / KILL cho BadUSB ---
            else if (strncmp(hmi_input, "CMD_BADUSB_HIDE", 15) == 0) {
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Tác vụ BadUSB đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_BADUSB_KILL", 15) == 0) {
                g_badusb_active = false;
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Tác vụ BadUSB đã dừng hoạt động.\n");
            }
            // --- HIDE / KILL cho RFID / NFC ---
            else if (strncmp(hmi_input, "CMD_RFID_HIDE", 13) == 0) {
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Tính năng quét RFID/NFC đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_RFID_KILL", 13) == 0) {
                g_rfid_active = false;
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Đã tắt đầu đọc RFID/NFC hoàn toàn.\n");
            }
            // --- HIDE / KILL cho Radio CC1101 ---
            else if (strncmp(hmi_input, "CMD_RADIO_HIDE", 14) == 0) {
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Tác vụ Radio CC1101 đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_RADIO_KILL", 14) == 0) {
                g_radio_active = false;
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Đã tắt phát sóng Radio CC1101.\n");
            }
            // --- HIDE / KILL cho IR Blaster ---
            else if (strncmp(hmi_input, "CMD_IR_HIDE", 11) == 0) {
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Phát tín hiệu IR đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_IR_KILL", 11) == 0) {
                g_ir_active = false;
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Đã tắt hồng ngoại IR hoàn toàn.\n");
            }
            // --- HIDE / KILL cho C5 Flasher ---
            else if (strncmp(hmi_input, "CMD_FLASH_HIDE", 14) == 0) {
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[*] Cầu nối nạp ESP32-C5 đang chạy ẩn dưới nền...\n");
            }
            else if (strncmp(hmi_input, "CMD_FLASH_KILL", 14) == 0) {
                g_flasher_active = false;
                dwin_switch_page(0);
                dwin_write_text(0x0098, "[!] Đã tắt chế độ nạp cứu hộ UART.\n");
            }
            // --- CÁC PHÍM ĐIỀU KHIỂN ĐA PHIÊN SSH ---
            else if (strcmp(hmi_input, "CMD_SSH_LIST") == 0) {
                push_config_to_c5("CMD_GET_TASKS 0");
            }
            else if (strncmp(hmi_input, "CMD_SSH_START:", 14) == 0) {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "CMD_START_TASK 0 %s", hmi_input + 14);
                push_config_to_c5(cmd);
            }
            else if (strncmp(hmi_input, "CMD_SSH_HIDE:", 13) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_HIDE_TASK %s", hmi_input + 13);
                push_config_to_c5(cmd);
                dwin_switch_page(0);
            }
            else if (strncmp(hmi_input, "CMD_SSH_RESUME:", 15) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_RESUME_TASK %s", hmi_input + 15);
                push_config_to_c5(cmd);
            }
            else if (strncmp(hmi_input, "CMD_SSH_KILL:", 13) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_KILL_TASK %s", hmi_input + 13);
                push_config_to_c5(cmd);
            }
            // --- CÁC PHÍM ĐIỀU KHIỂN ĐA PHIÊN DASHBOARD MONITOR ---
            else if (strcmp(hmi_input, "CMD_MONITOR_LIST") == 0) {
                push_config_to_c5("CMD_GET_TASKS 1");
            }
            else if (strncmp(hmi_input, "CMD_MONITOR_START:", 18) == 0) {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "CMD_START_TASK 1 %s", hmi_input + 18);
                push_config_to_c5(cmd);
            }
            else if (strncmp(hmi_input, "CMD_MONITOR_HIDE:", 17) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_HIDE_TASK %s", hmi_input + 17);
                push_config_to_c5(cmd);
                dwin_switch_page(0);
            }
            else if (strncmp(hmi_input, "CMD_MONITOR_RESUME:", 19) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_RESUME_TASK %s", hmi_input + 19);
                push_config_to_c5(cmd);
            }
            else if (strncmp(hmi_input, "CMD_MONITOR_KILL:", 17) == 0) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CMD_KILL_TASK %s", hmi_input + 17);
                push_config_to_c5(cmd);
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
        if (g_rfid_active && rdm6300_read_card(rfid_uid)) {
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
            else if (strncmp((char*)spi_rx_buf, "DEAUTH_LOG:", 11) == 0) {
                // Ghi log / AP list của Deauther vào vùng hiển thị HMI 0x0400
                dwin_write_text(0x0400, (char*)spi_rx_buf + 11);
                
                // Nếu log kết quả chứa thông tin thống kê gói tin (Ví dụ: "attack: stopping" hoặc pps stats)
                // Ta có thể trích xuất ra để ghi lên vùng nhớ status/stats
                if (strstr((char*)spi_rx_buf, "pps=") || strstr((char*)spi_rx_buf, "pkts=")) {
                    char *pkts_ptr = strstr((char*)spi_rx_buf + 11, "pkts=");
                    if (pkts_ptr) {
                        dwin_write_text(0x0480, pkts_ptr);
                    }
                }
            }
            else if (strncmp((char*)spi_rx_buf, "LOG:", 4) == 0) {
                // Ghi log hệ thống vào vùng nhớ HMI 0x0098
                dwin_write_text(0x0098, (char*)spi_rx_buf + 4);
            }
            else if (strncmp((char*)spi_rx_buf, "APP:", 4) == 0) {
                // Nhận lệnh chuyển đổi giao diện Smart Profile qua BLE
                dwin_write_text(0x0098, "[+] BLE Smart Profile Event: ");
                dwin_write_text(0x0098, (char*)spi_rx_buf + 4);
                dwin_write_text(0x0098, "\n");
            }
            else if (strncmp((char*)spi_rx_buf, "TASK_LIST:", 10) == 0) {
                char *list = strchr((char*)spi_rx_buf + 10, ':');
                if (list) {
                    list++; // Bỏ qua ký tự ':'
                    // Thay thế tất cả ';' bằng '\n' để hiển thị danh sách dòng mới trên DWIN
                    for (int i = 0; list[i] != '\0'; i++) {
                        if (list[i] == ';') list[i] = '\n';
                    }
                    dwin_write_text(0x0400, list);
                }
            }
            else if (strncmp((char*)spi_rx_buf, "MON_STAT:", 9) == 0) {
                int task_id, cpu, ram, disk;
                if (sscanf((char*)spi_rx_buf + 9, "%d:%d,%d,%d", &task_id, &cpu, &ram, &disk) == 4) {
                    char cpu_str[32], ram_str[32], disk_str[32];
                    snprintf(cpu_str, sizeof(cpu_str), "CPU: %d%%", cpu);
                    snprintf(ram_str, sizeof(ram_str), "RAM: %d%%", ram);
                    snprintf(disk_str, sizeof(disk_str), "Disk: %d%%", disk);
                    dwin_write_text(0x0480, cpu_str);
                    dwin_write_text(0x0482, ram_str);
                    dwin_write_text(0x0484, disk_str);
                }
            }
            else if (strncmp((char*)spi_rx_buf, "MON_HIST:", 9) == 0) {
                // Hiển thị lịch sử lấy thông số CPU/RAM dưới dạng Text trên vùng hiển thị 0x0400
                dwin_write_text(0x0400, "History Data Chunks:\n");
                dwin_write_text(0x0400, (char*)spi_rx_buf + 9);
            }
            else if (strncmp((char*)spi_rx_buf, "WTH:", 4) == 0) {
                int temp, humidity, icon_id;
                if (sscanf((char*)spi_rx_buf + 4, "%d:%d:%d", &temp, &humidity, &icon_id) == 3) {
                    dwin_write_val(0x0160, (uint16_t)temp);
                    dwin_write_val(0x0162, (uint16_t)humidity);
                    dwin_write_val(0x0164, (uint16_t)icon_id);
                }
            }
        }
        
        sleep_ms(5);
    }
}
