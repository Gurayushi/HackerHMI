#ifndef IR_RX_H
#define IR_RX_H

#include <stdint.h>
#include <stdbool.h>

#define IR_RX_PIN 23

void ir_rx_init(void);
bool ir_rx_capture_signal(uint32_t *out_durations, uint16_t *out_len, uint32_t timeout_ms);

#endif // IR_RX_H
