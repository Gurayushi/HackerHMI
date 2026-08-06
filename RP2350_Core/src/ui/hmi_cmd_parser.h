#ifndef HMI_CMD_PARSER_H
#define HMI_CMD_PARSER_H

#include <stdbool.h>

extern volatile bool g_badusb_active;
extern volatile bool g_radio_active;
extern volatile bool g_ir_active;
extern volatile bool g_flasher_active;

void process_hmi_command(const char* hmi_input);

#endif // HMI_CMD_PARSER_H
