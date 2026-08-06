#ifndef IR_PROTOCOLS_H
#define IR_PROTOCOLS_H

#include <stdint.h>

typedef enum {
    IR_TYPE_PARSED,
    IR_TYPE_RAW
} ir_signal_type_t;

// Bộ mã hóa giao thức chuẩn thành chuỗi xung định thời (microseconds)
// Trả về số lượng phần tử timing đã ghi vào out_timings
uint16_t ir_encode_nec(uint32_t address, uint32_t command, uint32_t *out_timings);
uint16_t ir_encode_sirc_12(uint32_t address, uint32_t command, uint32_t *out_timings);
uint16_t ir_encode_rc5(uint32_t address, uint32_t command, uint32_t *out_timings);

#endif // IR_PROTOCOLS_H
