#include "hmi_cmd_parser.h"
#include "comm/c5_spi.h"
#include "multicore/core1_task.h"
#include "ui/dwin_ui.h"
#include "usb/badusb.h"
#include "usb/hid_device.h"
#include "ir/ir_blaster.h"
#include "flasher/esp32_flasher.h"
#include "radio/radio_cc1101.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile bool g_badusb_active = false;
volatile bool g_radio_active = false;
volatile bool g_ir_active = false;
volatile bool g_flasher_active = false;

// Dummy rfid status flag for backward compatibility
volatile bool g_rfid_active = false;

void process_hmi_command(const char* hmi_input) {
    if (strncmp(hmi_input, "CMD_IR_FIRE", 11) == 0) {
        g_ir_active = true;
        ir_tv_buster_fire(); 
    }
    else if (strncmp(hmi_input, "CMD_IR_WEB_START", 16) == 0) {
        g_ir_active = true;
        dwin_write_text(0x0550, "Starting IR Web Server...\n");
        push_config_to_c5("start_ir_web=1");
    }
    else if (strncmp(hmi_input, "CMD_IR_WEB_STOP", 15) == 0) {
        push_config_to_c5("start_ir_web=0");
        dwin_write_text(0x0550, "IR Web Server Stopped.\n");
    }
    else if (strncmp(hmi_input, "CMD_GOTO_WIFI", 13) == 0) {
        dwin_switch_page(1); 
    }
    else if (strncmp(hmi_input, "CMD_WIFI_FORGET:", 16) == 0) {
        push_config_to_c5(hmi_input);
        dwin_write_text(0x0098, "Forgetting Wi-Fi network...\n");
    }
    else if (strncmp(hmi_input, "CMD_RF_OPEN", 11) == 0) {
        g_radio_active = true;
        uint8_t payload[] = {0x01, 0x02, 0x03}; 
        cc1101_transmit_signal(payload, 3);
    }
    else if (strncmp(hmi_input, "CMD_OS_WIN", 10) == 0) {
        g_badusb_active = true;
        hid_set_os(0); 
        dwin_write_text(0x0098, "[+] Switched to Windows Mode\n");
    }
    else if (strncmp(hmi_input, "CMD_OS_MAC", 10) == 0) {
        g_badusb_active = true;
        hid_set_os(1); 
        dwin_write_text(0x0098, "[+] Switched to MacOS Mode\n");
    }
    else if (strncmp(hmi_input, "CMD_OS_LINUX", 12) == 0) {
        g_badusb_active = true;
        hid_set_os(2); 
        dwin_write_text(0x0098, "[+] Switched to Linux Mode\n");
    }
    else if (strncmp(hmi_input, "CMD_OS_ANDROID", 14) == 0) {
        g_badusb_active = true;
        hid_set_os(3); 
        dwin_write_text(0x0098, "[+] Switched to Android Mode\n");
    }
    else if (strncmp(hmi_input, "CMD_MACRO_1", 11) == 0) {
        g_badusb_active = true;
        hid_run_macro(1); 
        dwin_write_text(0x0098, "[+] Executing Macro 1 (Work Mode)\n");
    }
    else if (strncmp(hmi_input, "CMD_MACRO_2", 11) == 0) {
        g_badusb_active = true;
        hid_run_macro(2); 
        dwin_write_text(0x0098, "[+] Executing Macro 2 (Gaming Mode)\n");
    }
    else if (strncmp(hmi_input, "CMD_MACRO_3", 11) == 0) {
        g_badusb_active = true;
        hid_run_macro(3); 
        dwin_write_text(0x0098, "[+] Executing Macro 3 (Dev Mode)\n");
    }
    else if (strncmp(hmi_input, "CMD_MACRO_4", 11) == 0) {
        g_badusb_active = true;
        hid_run_macro(4); 
        dwin_write_text(0x0098, "[+] Executing Custom Macro\n");
    }
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
    else if (strncmp(hmi_input, "CMD_FLASH_MODE", 14) == 0) {
        g_flasher_active = true;
        dwin_write_text(0x0098, "\n[!] ENTERING USB-TO-UART FLASH MODE...\n");
        dwin_write_text(0x0098, "[!] Connect PC to RP2350 USB and run esptool.\n");
        esp32_enter_bootloader();
        esp32_uart_bridge_task();
    }
    else if (strncmp(hmi_input, "CMD_START_OTA", 13) == 0) {
        push_config_to_c5("start_ota=1");
        dwin_write_text(0x0098, "[+] Requested ESP32-C5 to launch OTA Web Server...\n");
    }
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
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Wi-Fi Deauther dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_DEAUTH_KILL", 15) == 0) {
        push_config_to_c5("stop");
        dwin_write_text(0x0470, "STOPPED");
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Wi-Fi Deauther da tat hoan toan.\n");
    }
    else if (strncmp(hmi_input, "CMD_BADUSB_HIDE", 15) == 0) {
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Tac vu BadUSB dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_BADUSB_KILL", 15) == 0) {
        g_badusb_active = false;
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Tac vu BadUSB da dung hoat dong.\n");
    }
    else if (strncmp(hmi_input, "CMD_RFID_HIDE", 13) == 0) {
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Tinh nang RFID/NFC dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_RFID_KILL", 13) == 0) {
        g_rfid_active = false;
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Da tat dau doc RFID/NFC hoan toan.\n");
    }
    else if (strcmp(hmi_input, "CMD_RFID_READ") == 0) {
        dwin_write_text(0x0098, "[*] Dang doc the RFID 125kHz (EM4100)...\n");
        start_core1_task(CORE1_CMD_RFID_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_HID_READ") == 0) {
        dwin_write_text(0x0098, "[*] Dang doc the HID Prox 125kHz (FSK)...\n");
        start_core1_task(CORE1_CMD_HID_READ, NULL);
    }
    else if (strncmp(hmi_input, "CMD_HID_EMULATE:", 16) == 0) {
        char emu_uid[32] = {0};
        strncpy(emu_uid, hmi_input + 16, sizeof(emu_uid) - 1);
        start_core1_task(CORE1_CMD_HID_EMULATE, emu_uid);
    }
    else if (strncmp(hmi_input, "CMD_RFID_WRITE:", 15) == 0) {
        char target_uid[32] = {0};
        strncpy(target_uid, hmi_input + 15, sizeof(target_uid) - 1);
        dwin_write_text(0x0098, "[*] Dang ghi the T5577...\n");
        start_core1_task(CORE1_CMD_RFID_WRITE, target_uid);
    }
    else if (strncmp(hmi_input, "CMD_RFID_EMULATE:", 17) == 0) {
        char emu_uid[32] = {0};
        strncpy(emu_uid, hmi_input + 17, sizeof(emu_uid) - 1);
        start_core1_task(CORE1_CMD_RFID_EMULATE, emu_uid);
    }
    else if (strcmp(hmi_input, "CMD_NFC_READ") == 0) {
        dwin_write_text(0x0098, "[*] Dang doc the NFC (ISO14443-A)...\n");
        start_core1_task(CORE1_CMD_NFC_READ, NULL);
    }
    else if (strncmp(hmi_input, "CMD_NFC_EMULATE:", 16) == 0) {
        char emu_uid_hex[32] = {0};
        strncpy(emu_uid_hex, hmi_input + 16, sizeof(emu_uid_hex) - 1);
        start_core1_task(CORE1_CMD_NFC_EMULATE, emu_uid_hex);
    }
    else if (strcmp(hmi_input, "CMD_RF_DECODE") == 0) {
        dwin_write_text(0x0098, "[*] Dang quet song RF 433.92MHz...\n");
        start_core1_task(CORE1_CMD_RF_DECODE, NULL);
    }
    else if (strncmp(hmi_input, "CMD_RADIO_HIDE", 14) == 0) {
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Tac vu Radio CC1101 dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_RADIO_KILL", 14) == 0) {
        g_radio_active = false;
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Da tat phat song Radio CC1101.\n");
    }
    else if (strncmp(hmi_input, "CMD_IR_HIDE", 11) == 0) {
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Phat tin hieu IR dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_IR_KILL", 11) == 0) {
        g_ir_active = false;
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Da tat hong ngoai IR hoan toan.\n");
    }
    else if (strncmp(hmi_input, "CMD_FLASH_HIDE", 14) == 0) {
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[*] Cau noi nap ESP32-C5 dang chay an duoi nen...\n");
    }
    else if (strncmp(hmi_input, "CMD_FLASH_KILL", 14) == 0) {
        g_flasher_active = false;
        dwin_switch_page(0);
        dwin_write_text(0x0098, "[!] Da tat che do nuoc cuu ho UART.\n");
    }
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
    else if (strcmp(hmi_input, "CMD_IBUTTON_READ") == 0) {
        start_core1_task(CORE1_CMD_IBUTTON_READ, NULL);
    }
    else if (strncmp(hmi_input, "CMD_IBUTTON_EMULATE:", 20) == 0) {
        start_core1_task(CORE1_CMD_IBUTTON_EMULATE, hmi_input + 20);
    }
    else if (strcmp(hmi_input, "CMD_RF_BRUTEFORCE") == 0) {
        start_core1_task(CORE1_CMD_RF_BRUTEFORCE, NULL);
    }
    else if (strcmp(hmi_input, "CMD_IR_LEARN") == 0) {
        start_core1_task(CORE1_CMD_IR_LEARN, NULL);
    }
    else if (strcmp(hmi_input, "CMD_RFID_INDALA_READ") == 0) {
        start_core1_task(CORE1_CMD_RFID_INDALA_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_RFID_AWID_READ") == 0) {
        start_core1_task(CORE1_CMD_RFID_AWID_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_RFID_FDXB_READ") == 0) {
        start_core1_task(CORE1_CMD_RFID_FDXB_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_IBUTTON_CYFRAL_READ") == 0) {
        start_core1_task(CORE1_CMD_IBUTTON_CYFRAL_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_IBUTTON_METAKOM_READ") == 0) {
        start_core1_task(CORE1_CMD_IBUTTON_METAKOM_READ, NULL);
    }
    else if (strcmp(hmi_input, "CMD_IR_AC_UNIVERSAL") == 0) {
        start_core1_task(CORE1_CMD_IR_AC_UNIVERSAL, NULL);
    }
    else {
        push_config_to_c5(hmi_input);
        dwin_write_text(0x0098, "Config sent to ESP32-C5 for NVS Storage.\n");
    }
}
