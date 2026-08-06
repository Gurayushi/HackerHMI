#ifndef IBUTTO_EXT_H
#define IBUTTO_EXT_H

#include <stdint.h>
#include <stdbool.h>

bool ibutton_read_cyfral(char* out_key_hex);
bool ibutton_read_metakom(char* out_key_hex);
void ibutton_emulate_cyfral(const char* key_hex);

#endif // IBUTTO_EXT_H
