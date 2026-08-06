#ifndef IBUTTON_H
#define IBUTTON_H

#include <stdint.h>
#include <stdbool.h>

#define IBUTTON_PIN 28

void ibutton_init(void);
bool ibutton_read_ds1990a(char* out_key_hex);
void ibutton_emulate_ds1990a(const char* key_hex);
uint8_t ibutton_crc8(const uint8_t *data, uint8_t len);

#endif // IBUTTON_H
