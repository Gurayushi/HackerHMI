#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

// System Modules
#include "ui/dwin_ui.h"
#include "terminal/terminal.h"
#include "comm/c5_spi.h"
#include "multicore/core1_task.h"
#include "ui/hmi_cmd_parser.h"

// Hacker Modules
#include "usb/badusb.h"
#include "usb/hid_device.h"
#include "radio/radio_cc1101.h"
#include "rfid/rfid_nfc.h"
#include "ir/ir_blaster.h"
#include "flasher/esp32_flasher.h"

// Global Terminal buffer
RingBuffer terminal_buffer;

// Safe HMI UART write mutex
mutex_t dwin_uart_mutex;

// Raw IR capture buffer
static uint32_t g_ir_raw_buf[1024];
static uint16_t g_ir_raw_idx = 0;

int main() {
    // --- OVERCLOCK TO 300MHz ---
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(2);
    set_sys_clock_khz(300000, true);
    
    stdio_init_all();
    
    // --- BASIC SYSTEM INIT ---
    term_init(&terminal_buffer);
    
    // Initialize Mutex for DWIN UART thread safety
    mutex_init(&dwin_uart_mutex);
    
    dwin_init();
    init_spi_for_c5();
    
    // --- HACKER MODULES INIT ---
    badusb_init();
    cc1101_init(433.92); // Default to 433.92MHz
    rfid_nfc_init();
    ir_blaster_init();
    esp32_flasher_init();
    
    // Launch Core 1 execution task
    multicore_launch_core1(core1_entry);
    
    dwin_write_text(0x0098, "HackerHMI - Booting RP2350 Core...\n");
    dwin_write_text(0x0098, "[+] All Hacker Modules Loaded.\n");
    dwin_write_text(0x0098, "[+] Core 1 Multiprocessing Active.\n");

    while (true) {
        // Sync result buffer from Core 1 to ESP32-C5 over SPI0 (safe on Core 0)
        if (g_core1_result_ready) {
            g_core1_result_ready = false;
            push_config_to_c5(g_core1_result_buf);
        }

        // LUỒNG 1: Giữ kết nối BadUSB luôn sống
        if (g_badusb_active) {
            badusb_task();
        }

        // LUỒNG 2: Xử lý thao tác cảm ứng từ người dùng (UART)
        char hmi_input[128] = {0};
        dwin_listen_keyboard_input(hmi_input);
        
        // --- XỬ LÝ LỆNH TỪ MÀN HÌNH CẢM ỨNG ---
        if (strlen(hmi_input) > 0) {
            // Signal Core 1 to abort any background task immediately on user input
            if (g_core1_run) {
                g_core1_run = false;
                sleep_ms(50); // wait for Core 1 task cleanup
            }
            process_hmi_command(hmi_input);
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
                dwin_write_text(0x0098, "[!] Recv DWIN update command over Wi-Fi...\n");
            }
            else if (strncmp((char*)spi_rx_buf, "DEAUTH_LOG:", 11) == 0) {
                dwin_write_text(0x0400, (char*)spi_rx_buf + 11);
                
                if (strstr((char*)spi_rx_buf, "pps=") || strstr((char*)spi_rx_buf, "pkts=")) {
                    char *pkts_ptr = strstr((char*)spi_rx_buf + 11, "pkts=");
                    if (pkts_ptr) {
                        dwin_write_text(0x0480, pkts_ptr);
                    }
                }
            }
            else if (strncmp((char*)spi_rx_buf, "LOG:", 4) == 0) {
                dwin_write_text(0x0098, (char*)spi_rx_buf + 4);
            }
            else if (strncmp((char*)spi_rx_buf, "APP:", 4) == 0) {
                dwin_write_text(0x0098, "[+] BLE Smart Profile Event: ");
                dwin_write_text(0x0098, (char*)spi_rx_buf + 4);
            }
            else if (strncmp((char*)spi_rx_buf, "IR_FILE_SIGNAL:", 15) == 0) {
                char proto[16];
                uint32_t address = 0, command = 0;
                if (sscanf((char*)spi_rx_buf + 15, "%[^:]:%lu:%lu", proto, &address, &command) == 3) {
                    uint32_t timings[80];
                    uint16_t len = 0;
                    if (strcmp(proto, "NEC") == 0) {
                        len = ir_encode_nec(address, command, timings);
                    } else if (strcmp(proto, "SIRC") == 0) {
                        len = ir_encode_sirc_12(address, command, timings);
                    } else if (strcmp(proto, "RC5") == 0) {
                        len = ir_encode_rc5(address, command, timings);
                    }
                    
                    if (len > 0) {
                        dwin_write_text(0x0550, "[+] Transmitting IR signal...\n");
                        ir_transmit_raw(timings, len);
                    }
                }
            }
            else if (strncmp((char*)spi_rx_buf, "IR_FILE_RAW_START:", 18) == 0) {
                char name[32];
                uint32_t freq = 38000;
                sscanf((char*)spi_rx_buf + 18, "%[^:]:%lu", name, &freq);
                
                char log_msg[128];
                snprintf(log_msg, sizeof(log_msg), "[+] Recv Raw Key: %s\nFrequency: %luHz\n", name, freq);
                dwin_write_text(0x0550, log_msg);
                
                g_ir_raw_idx = 0;
            }
            else if (strncmp((char*)spi_rx_buf, "IR_FILE_RAW_DATA:", 17) == 0) {
                char *token = strtok((char*)spi_rx_buf + 17, ",");
                while (token != NULL && g_ir_raw_idx < 1024) {
                    g_ir_raw_buf[g_ir_raw_idx++] = strtoul(token, NULL, 10);
                    token = strtok(NULL, ",");
                }
            }
            else if (strncmp((char*)spi_rx_buf, "IR_FILE_RAW_END", 15) == 0) {
                char log_msg[64];
                snprintf(log_msg, sizeof(log_msg), "[+] Transmitting RAW (%d pulses)...\n", g_ir_raw_idx);
                dwin_write_text(0x0550, log_msg);
                
                ir_transmit_raw(g_ir_raw_buf, g_ir_raw_idx);
            }
            else if (strcmp((char*)spi_rx_buf, "IR_UPLOAD_OK") == 0) {
                dwin_write_text(0x0550, "[+] Upload Successful!\n[!] Web Server Closed.\n");
            }
            else if (strncmp((char*)spi_rx_buf, "WTH:", 4) == 0) {
                int temp, humidity, icon_id, aqi;
                int parsed = sscanf((char*)spi_rx_buf + 4, "%d:%d:%d:%d", &temp, &humidity, &icon_id, &aqi);
                if (parsed >= 3) {
                    dwin_write_val(0x0160, (uint16_t)temp);
                    dwin_write_val(0x0162, (uint16_t)humidity);
                    dwin_write_val(0x0164, (uint16_t)icon_id);
                    if (parsed == 4) {
                        dwin_write_val(0x0166, (uint16_t)aqi);
                    }
                }
            }
        }
        
        sleep_ms(5);
    }
}
