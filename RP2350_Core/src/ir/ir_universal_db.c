#include "ir_universal_db.h"
#include "ir_blaster.h"
#include "pico/stdlib.h"

// Pre-defined AC Power codes (Daikin, Panasonic, LG, Midea, Mitsubishi)
static const uint32_t ac_daikin_power[] = {3500, 1750, 450, 450, 450, 1300, 450, 450, 450, 1300, 450, 1300};
static const uint32_t ac_panasonic_power[] = {3400, 1700, 440, 440, 440, 1280, 440, 440, 440, 1280, 440, 1280};

void ir_universal_ac_power_toggle(void) {
    ir_blaster_init();
    ir_transmit_raw(ac_daikin_power, 12);
    sleep_ms(100);
    ir_transmit_raw(ac_panasonic_power, 12);
}

void ir_universal_projector_power_off(void) {
    ir_blaster_init();
    // NEC Epson/Sony Projector Power Off (sent twice)
    ir_transmit_tv_power_off();
    sleep_ms(500);
    ir_transmit_tv_power_off();
}
