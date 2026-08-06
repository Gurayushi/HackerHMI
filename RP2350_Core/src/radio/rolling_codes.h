#ifndef ROLLING_CODES_H
#define ROLLING_CODES_H

#include <stdint.h>
#include <stdbool.h>

bool cc1101_decode_somfy(const uint32_t* durations, uint16_t count, uint32_t* out_remote_id, uint16_t* out_rolling_cnt);
bool cc1101_decode_secplus(const uint32_t* durations, uint16_t count, uint32_t* out_id);

#endif // ROLLING_CODES_H
