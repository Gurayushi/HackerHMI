#include "debruijn_gen.h"
#include "radio_cc1101.h"
#include "pico/stdlib.h"

extern volatile bool g_core1_run;

uint16_t debruijn_generate_12bit_sequence(uint32_t *out_buffer, uint16_t max_words) {
    uint16_t count = 0;
    uint32_t a[16] = {0};
    int k = 2;
    int n = 12;
    int t = 1;
    int p = 1;
    
    while (p > 0 && count < max_words) {
        if (t > n) {
            if (n % p == 0) {
                uint32_t val = 0;
                for (int i = 1; i <= n; i++) {
                    val = (val << 1) | a[i];
                }
                out_buffer[count++] = val;
            }
        } else {
            a[t] = a[t - p];
            if (a[t] == (k - 1)) {
                p = t;
            } else {
                a[t]++;
                p = t;
            }
        }
        t++;
    }
    return count;
}

void debruijn_transmit_bruteforce_12bit(void) {
    cc1101_set_tx_mode();
    gpio_set_dir(CC1101_PIN_GDO0, GPIO_OUT);
    
    // De Bruijn brute-force pulse sequence (350us TE)
    for (uint32_t code = 0; code < 4096; code++) {
        uint32_t timings[50];
        uint16_t len = cc1101_encode_princeton(code, timings);
        
        for (uint16_t i = 0; i < len; i++) {
            gpio_put(CC1101_PIN_GDO0, (i % 2 == 0) ? 1 : 0);
            sleep_us(timings[i]);
        }
        
        if (!g_core1_run) break;
    }
    
    gpio_put(CC1101_PIN_GDO0, 0);
    gpio_set_dir(CC1101_PIN_GDO0, GPIO_IN);
    cc1101_cmd_strobe(CC1101_SIDLE);
}
