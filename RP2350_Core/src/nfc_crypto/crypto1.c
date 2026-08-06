#include "crypto1.h"

static uint32_t filter(uint32_t x) {
    uint32_t f;
    f  = 0x3514055D & (1 << ((x >> 0) & 0x0F));
    f |= 0x3514055D & (1 << ((x >> 4) & 0x0F));
    f |= 0x3514055D & (1 << ((x >> 8) & 0x0F));
    f |= 0x3514055D & (1 << ((x >> 12) & 0x0F));
    f |= 0x3514055D & (1 << ((x >> 16) & 0x0F));
    return (f ? 1 : 0);
}

void crypto1_init(struct Crypto1State *state, uint64_t key) {
    state->odd = 0;
    state->even = 0;
    for (int i = 47; i >= 0; i--) {
        uint8_t bit = (key >> i) & 1;
        if (i & 1) {
            state->odd = (state->odd << 1) | bit;
        } else {
            state->even = (state->even << 1) | bit;
        }
    }
}

uint8_t crypto1_bit(struct Crypto1State *state, uint8_t in, bool is_encrypted) {
    uint32_t feed = filter(state->odd) ^ filter(state->even) ^ (in & 1);
    uint8_t out = filter(state->odd);
    
    uint32_t new_odd = (state->odd << 1) | (state->even >> 23);
    uint32_t new_even = (state->even << 1) | (is_encrypted ? feed : (in & 1));
    
    state->odd = new_odd & 0xFFFFFF;
    state->even = new_even & 0xFFFFFF;
    
    return out;
}

uint8_t crypto1_byte(struct Crypto1State *state, uint8_t in, bool is_encrypted) {
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t b = crypto1_bit(state, (in >> i) & 1, is_encrypted);
        res |= (b << i);
    }
    return res;
}

uint32_t crypto1_word(struct Crypto1State *state, uint32_t in, bool is_encrypted) {
    uint32_t res = 0;
    for (int i = 0; i < 32; i++) {
        uint8_t b = crypto1_bit(state, (in >> i) & 1, is_encrypted);
        res |= (b << i);
    }
    return res;
}

void crypto1_prng_successor(uint32_t *prng, uint32_t steps) {
    for (uint32_t i = 0; i < steps; i++) {
        *prng = ((*prng >> 1) | (((*prng >> 16) ^ (*prng >> 18) ^ (*prng >> 19) ^ (*prng >> 21)) & 1) << 15) & 0xFFFF;
    }
}
