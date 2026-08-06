#include "core1_task.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "ui/dwin_ui.h"
#include "rfid/rfid_nfc.h"
#include "radio/radio_cc1101.h"
#include "ibutton/ibutton.h"
#include "radio/keeloq.h"
#include "radio/debruijn_gen.h"
#include "ir/ir_rx.h"
#include "rfid/rfid_protocols_ext.h"
#include "ibutton/ibutton_ext.h"
#include "radio/rolling_codes.h"
#include "ir/ir_universal_db.h"

volatile bool g_core1_run = false;
volatile uint32_t g_core1_cmd = CORE1_CMD_IDLE;
char g_core1_uid_arg[64] = {0};

volatile bool g_core1_result_ready = false;
char g_core1_result_buf[128] = {0};

void core1_entry(void) {
    while (1) {
        uint32_t cmd = multicore_fifo_pop_blocking();
        g_core1_cmd = cmd;
        g_core1_run = true;
        
        if (cmd == CORE1_CMD_RFID_READ) {
            char card_uid[32] = {0};
            if (em4095_read_em4100(card_uid)) {
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "[+] Quet thanh cong RFID: %s\n", card_uid);
                dwin_write_text(0x0098, log_msg);
                dwin_write_text(0x0400, card_uid);
                
                snprintf(g_core1_result_buf, sizeof(g_core1_result_buf), "CMD_TAG_ADD_RFID:RFID_%s:%s:0", card_uid + 6, card_uid);
                g_core1_result_ready = true;
            } else {
                dwin_write_text(0x0098, "[!] That bai: Khong phat hien the RFID.\n");
            }
        }
        else if (cmd == CORE1_CMD_IBUTTON_READ) {
            char key_str[32] = {0};
            dwin_write_text(0x0098, "[*] Dang doc khoa iButton 1-Wire (DS1990A)...\n");
            if (ibutton_read_ds1990a(key_str)) {
                char log_msg[128];
                snprintf(log_msg, sizeof(log_msg), "[+] Quet thanh cong iButton: %s\n", key_str);
                dwin_write_text(0x0098, log_msg);
                dwin_write_text(0x0400, key_str);
                
                snprintf(g_core1_result_buf, sizeof(g_core1_result_buf), "CMD_TAG_ADD_IBUTTON:%s:%s:0", key_str, key_str);
                g_core1_result_ready = true;
            } else {
                dwin_write_text(0x0098, "[!] That bai: Khong phat hien khoa iButton.\n");
            }
        }
        else if (cmd == CORE1_CMD_IBUTTON_EMULATE) {
            char log_msg[128];
            snprintf(log_msg, sizeof(log_msg), "[*] Dang gia lap iButton 1-Wire: %s\n[!] Bam nut bat ky de dung.\n", g_core1_uid_arg);
            dwin_write_text(0x0098, log_msg);
            ibutton_emulate_ds1990a(g_core1_uid_arg);
            dwin_write_text(0x0098, "[+] Da dung gia lap iButton.\n");
        }
        else if (cmd == CORE1_CMD_RF_BRUTEFORCE) {
            dwin_write_text(0x0098, "[!] DANG CHAY VET CAN DE BRUIJN 12-BIT (4096 CODES)...\n");
            debruijn_transmit_bruteforce_12bit();
            dwin_write_text(0x0098, "[+] Hoan thanh vet can De Bruijn!\n");
        }
        else if (cmd == CORE1_CMD_IR_LEARN) {
            dwin_write_text(0x0098, "[*] Dang cho phat tin hieu Hong ngoai (VS1838B)...\n");
            uint32_t raw_timings[500];
            uint16_t raw_len = 0;
            if (ir_rx_capture_signal(raw_timings, &raw_len, 5000)) {
                char log_msg[128];
                snprintf(log_msg, sizeof(log_msg), "[+] Hoc thanh cong IR (%d pulses)!\n", raw_len);
                dwin_write_text(0x0098, log_msg);
            } else {
                dwin_write_text(0x0098, "[!] That bai: Khong nhan duoc tin hieu IR.\n");
            }
        }
        else if (cmd == CORE1_CMD_RFID_INDALA_READ) {
            char uid[32] = {0};
            if (em4095_read_indala(uid)) {
                char log[128];
                snprintf(log, sizeof(log), "[+] Quet Indala PSK: %s\n", uid);
                dwin_write_text(0x0098, log);
            } else {
                dwin_write_text(0x0098, "[!] Khong tim thay the Indala.\n");
            }
        }
        else if (cmd == CORE1_CMD_RFID_AWID_READ) {
            char uid[32] = {0};
            if (em4095_read_awid(uid)) {
                char log[128];
                snprintf(log, sizeof(log), "[+] Quet AWID: %s\n", uid);
                dwin_write_text(0x0098, log);
            } else {
                dwin_write_text(0x0098, "[!] Khong tim thay the AWID.\n");
            }
        }
        else if (cmd == CORE1_CMD_RFID_FDXB_READ) {
            char uid[32] = {0};
            if (em4095_read_fdxb(uid)) {
                char log[128];
                snprintf(log, sizeof(log), "[+] Quet Chip FDX-B Thu Cung: %s\n", uid);
                dwin_write_text(0x0098, log);
            } else {
                dwin_write_text(0x0098, "[!] Khong tim thay chip FDX-B (134.2kHz).\n");
            }
        }
        else if (cmd == CORE1_CMD_IBUTTON_CYFRAL_READ) {
            char key[32] = {0};
            if (ibutton_read_cyfral(key)) {
                char log[128];
                snprintf(log, sizeof(log), "[+] Quet Cyfral: %s\n", key);
                dwin_write_text(0x0098, log);
            } else {
                dwin_write_text(0x0098, "[!] Khong phat hien chia Cyfral.\n");
            }
        }
        else if (cmd == CORE1_CMD_IBUTTON_METAKOM_READ) {
            char key[32] = {0};
            if (ibutton_read_metakom(key)) {
                char log[128];
                snprintf(log, sizeof(log), "[+] Quet Metakom: %s\n", key);
                dwin_write_text(0x0098, log);
            } else {
                dwin_write_text(0x0098, "[!] Khong phat hien chia Metakom.\n");
            }
        }
        else if (cmd == CORE1_CMD_IR_AC_UNIVERSAL) {
            dwin_write_text(0x0098, "[*] Dang phat chuoi xung Dieu hoa Van nang (Daikin/Panasonic)...\n");
            ir_universal_ac_power_toggle();
            dwin_write_text(0x0098, "[+] Da phat xong tin hieu Dieu hoa Van nang!\n");
        }
        else if (cmd == CORE1_CMD_RFID_WRITE) {
            if (em4095_write_em4100(g_core1_uid_arg)) {
                dwin_write_text(0x0098, "[+] Ghi the T5577 thanh cong!\n");
            } else {
                dwin_write_text(0x0098, "[!] Ghi the T5577 that bai.\n");
            }
        }
        else if (cmd == CORE1_CMD_RFID_EMULATE) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "[*] Dang gia lap RFID: %s\n[!] Bam nut bat ky tren DWIN de dung.\n", g_core1_uid_arg);
            dwin_write_text(0x0098, log_msg);
            em4095_emulate_em4100(g_core1_uid_arg);
            dwin_write_text(0x0098, "[+] Da dung gia lap RFID.\n");
        }
        else if (cmd == CORE1_CMD_HID_READ) {
            char card_uid[32] = {0};
            if (em4095_read_hid_prox(card_uid)) {
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "[+] Quet thanh cong HID: %s\n", card_uid);
                dwin_write_text(0x0098, log_msg);
                dwin_write_text(0x0400, card_uid);
                
                snprintf(g_core1_result_buf, sizeof(g_core1_result_buf), "CMD_TAG_ADD_RFID:HID_%s:%s:2", card_uid + 4, card_uid);
                g_core1_result_ready = true;
            } else {
                dwin_write_text(0x0098, "[!] That bai: Khong phat hien the HID Prox.\n");
            }
        }
        else if (cmd == CORE1_CMD_HID_EMULATE) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "[*] Dang gia lap HID Prox: %s\n[!] Bam nut bat ky tren DWIN de dung.\n", g_core1_uid_arg);
            dwin_write_text(0x0098, log_msg);
            em4095_emulate_hid_prox(g_core1_uid_arg);
            dwin_write_text(0x0098, "[+] Da dung gia lap HID Prox.\n");
        }
        else if (cmd == CORE1_CMD_NFC_READ) {
            uint8_t nfc_uid[10] = {0};
            uint8_t nfc_len = 0;
            if (pn532_read_nfc(nfc_uid, &nfc_len)) {
                char hex_str[32] = {0};
                for (uint8_t i = 0; i < nfc_len; i++) {
                    snprintf(hex_str + i * 2, 3, "%02X", nfc_uid[i]);
                }
                char log_msg[256];
                snprintf(log_msg, sizeof(log_msg), "[+] Quet thanh cong NFC UID: %s\n", hex_str);
                dwin_write_text(0x0098, log_msg);
                dwin_write_text(0x0400, hex_str);
                
                snprintf(g_core1_result_buf, sizeof(g_core1_result_buf), "CMD_TAG_ADD_NFC:NFC_%s:%s:0", hex_str, hex_str);
                g_core1_result_ready = true;
            } else {
                dwin_write_text(0x0098, "[!] That bai: Khong phat hien the NFC.\n");
            }
        }
        else if (cmd == CORE1_CMD_NFC_EMULATE) {
            uint8_t uid[10] = {0};
            uint8_t len = 0;
            for (int i = 0; i < strlen(g_core1_uid_arg); i += 2) {
                unsigned int val;
                sscanf(g_core1_uid_arg + i, "%2x", &val);
                uid[len++] = (uint8_t)val;
            }
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "[*] Dang gia lap NFC UID: %s\n[!] Cho ket noi dau doc...\n", g_core1_uid_arg);
            dwin_write_text(0x0098, log_msg);
            if (pn532_emulate_uid(uid, len)) {
                dwin_write_text(0x0098, "[+] Dau doc da xac thuc gia lap NFC thanh cong!\n");
            } else {
                dwin_write_text(0x0098, "[!] Da dung gia lap NFC.\n");
            }
        }
        else if (cmd == CORE1_CMD_RF_DECODE) {
            cc1101_set_rx_mode();
            uint32_t capture_buf[200] = {0};
            uint16_t idx = 0;
            uint32_t start_time = time_us_32();
            bool last_state = gpio_get(CC1101_PIN_GDO0);
            
            while ((time_us_32() - start_time) < 1000000 && idx < 200 && g_core1_run) {
                bool state = gpio_get(CC1101_PIN_GDO0);
                if (state != last_state) {
                    uint32_t now = time_us_32();
                    capture_buf[idx++] = now - start_time;
                    start_time = now;
                    last_state = state;
                }
            }
            cc1101_cmd_strobe(CC1101_SIDLE);
            
            if (idx > 10) {
                uint32_t princeton_code = 0;
                uint32_t came_code = 0;
                if (cc1101_decode_princeton(capture_buf, idx, &princeton_code)) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "[+] Giai ma Princeton: 0x%06lX\n", princeton_code);
                    dwin_write_text(0x0098, log_msg);
                    dwin_write_text(0x0400, log_msg);
                } else if (cc1101_decode_came(capture_buf, idx, &came_code)) {
                    char log_msg[256];
                    snprintf(log_msg, sizeof(log_msg), "[+] Giai ma Came: 0x%03lX\n", came_code);
                    dwin_write_text(0x0098, log_msg);
                    dwin_write_text(0x0400, log_msg);
                } else {
                    dwin_write_text(0x0098, "[!] Thu song tho thanh cong nhung khong khop giao thuc.\n");
                }
            } else {
                dwin_write_text(0x0098, "[!] Khong phat hien tin hieu RF hop le.\n");
            }
        }
        
        g_core1_run = false;
        g_core1_cmd = CORE1_CMD_IDLE;
    }
}

void start_core1_task(uint32_t cmd, const char* arg) {
    if (g_core1_run) {
        g_core1_run = false;
        sleep_ms(50); // wait for Core 1 task cleanup
    }
    if (arg) {
        strncpy(g_core1_uid_arg, arg, sizeof(g_core1_uid_arg) - 1);
        g_core1_uid_arg[sizeof(g_core1_uid_arg) - 1] = '\0';
    } else {
        g_core1_uid_arg[0] = '\0';
    }
    multicore_fifo_push_blocking(cmd);
}
