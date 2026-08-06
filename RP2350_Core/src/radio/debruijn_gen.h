#ifndef DEBRUIJN_GEN_H
#define DEBRUIJN_GEN_H

#include <stdint.h>
#include <stdbool.h>

uint16_t debruijn_generate_12bit_sequence(uint32_t *out_buffer, uint16_t max_words);
void debruijn_transmit_bruteforce_12bit(void);

#endif // DEBRUIJN_GEN_H
