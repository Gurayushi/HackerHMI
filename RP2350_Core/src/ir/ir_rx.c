#include "ir_rx.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

extern volatile bool g_core1_run;

void ir_rx_init(void) {
    gpio_init(IR_RX_PIN);
    gpio_set_dir(IR_RX_PIN, GPIO_IN);
    gpio_pull_up(IR_RX_PIN); // VS1838B output is active LOW
}

bool ir_rx_capture_signal(uint32_t *out_durations, uint16_t *out_len, uint32_t timeout_ms) {
    ir_rx_init();
    
    // Wait for initial pulse falling edge
    uint32_t start_wait = time_us_32();
    while (gpio_get(IR_RX_PIN) == 1) {
        if (!g_core1_run) return false;
        if ((time_us_32() - start_wait) > (timeout_ms * 1000)) return false;
        sleep_us(10);
    }
    
    uint16_t idx = 0;
    uint32_t last_time = time_us_32();
    bool last_state = gpio_get(IR_RX_PIN);
    
    while (idx < 500 && g_core1_run) {
        bool current_state = gpio_get(IR_RX_PIN);
        uint32_t now = time_us_32();
        
        if (current_state != last_state) {
            out_durations[idx++] = now - last_time;
            last_time = now;
            last_state = current_state;
        } else if ((now - last_time) > 15000) { // 15ms silence -> End of frame
            out_durations[idx++] = now - last_time;
            break;
        }
    }
    
    *out_len = idx;
    return (idx >= 10);
}
