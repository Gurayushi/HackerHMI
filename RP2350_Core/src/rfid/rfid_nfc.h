#ifndef RFID_NFC_H
#define RFID_NFC_H

#include <stdint.h>
#include <stdbool.h>

void rfid_nfc_init(void);
void pn532_i2c_write(uint8_t* data, uint8_t length);
void pn532_i2c_read(uint8_t* buffer, uint8_t length);
bool pn532_read_nfc(uint8_t* out_uid, uint8_t* uid_len);
bool pn532_emulate_uid(uint8_t* uid, uint8_t uid_len);
bool em4095_decode_em4100_frame(uint64_t frame, char* out_uid);
bool em4095_read_em4100(char* out_uid);
bool em4095_write_t5577_block(uint8_t block, uint32_t data);
bool em4095_write_em4100(const char* uid_hex);
void em4095_emulate_em4100(const char* uid_hex);
bool em4095_read_hid_prox(char* out_uid);
void em4095_emulate_hid_prox(const char* uid_str);

#endif // RFID_NFC_H
