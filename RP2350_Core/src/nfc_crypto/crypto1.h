#ifndef CRYPTO1_H
#define CRYPTO1_H

#include <stdint.h>
#include <stdbool.h>

struct Crypto1State {
    uint32_t odd;
    uint32_t even;
};

void crypto1_init(struct Crypto1State *state, uint64_t key);
uint8_t crypto1_bit(struct Crypto1State *state, uint8_t in, bool is_encrypted);
uint8_t crypto1_byte(struct Crypto1State *state, uint8_t in, bool is_encrypted);
uint32_t crypto1_word(struct Crypto1State *state, uint32_t in, bool is_encrypted);
void crypto1_prng_successor(uint32_t *prng, uint32_t steps);

#endif // CRYPTO1_H
