#ifndef KEELOQ_H
#define KEELOQ_H

#include <stdint.h>
#include <stdbool.h>

uint32_t keeloq_decrypt(uint32_t data, uint64_t key);
uint32_t keeloq_encrypt(uint32_t data, uint64_t key);
bool keeloq_decode_buffer(const uint32_t* durations, uint16_t count, uint64_t manuf_key, uint32_t* out_serial, uint16_t* out_btn, uint16_t* out_cnt);

#endif // KEELOQ_H
