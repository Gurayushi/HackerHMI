#ifndef RFID_PROTOCOLS_EXT_H
#define RFID_PROTOCOLS_EXT_H

#include <stdint.h>
#include <stdbool.h>

bool em4095_read_indala(char* out_uid);
bool em4095_read_awid(char* out_uid);
bool em4095_read_fdxb(char* out_uid);
void em4095_emulate_psk(const char* uid_hex);

#endif // RFID_PROTOCOLS_EXT_H
