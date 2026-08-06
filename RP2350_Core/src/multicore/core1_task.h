#ifndef CORE1_TASK_H
#define CORE1_TASK_H

#include <stdbool.h>
#include <stdint.h>

extern volatile bool g_core1_run;
extern volatile uint32_t g_core1_cmd;
extern char g_core1_uid_arg[64];

extern volatile bool g_core1_result_ready;
extern char g_core1_result_buf[128];

enum Core1Cmd {
    CORE1_CMD_IDLE = 0,
    CORE1_CMD_RFID_READ,
    CORE1_CMD_RFID_WRITE,
    CORE1_CMD_RFID_EMULATE,
    CORE1_CMD_HID_READ,
    CORE1_CMD_HID_EMULATE,
    CORE1_CMD_NFC_READ,
    CORE1_CMD_NFC_EMULATE,
    CORE1_CMD_RF_DECODE,
    CORE1_CMD_IBUTTON_READ,
    CORE1_CMD_IBUTTON_EMULATE,
    CORE1_CMD_RF_ANALYZE,
    CORE1_CMD_RF_RAW_CAPTURE,
    CORE1_CMD_RF_BRUTEFORCE,
    CORE1_CMD_IR_LEARN,
    CORE1_CMD_RFID_INDALA_READ,
    CORE1_CMD_RFID_AWID_READ,
    CORE1_CMD_RFID_FDXB_READ,
    CORE1_CMD_IBUTTON_CYFRAL_READ,
    CORE1_CMD_IBUTTON_METAKOM_READ,
    CORE1_CMD_RF_SOMFY_DECODE,
    CORE1_CMD_IR_AC_UNIVERSAL
};

void core1_entry(void);
void start_core1_task(uint32_t cmd, const char* arg);

#endif // CORE1_TASK_H
