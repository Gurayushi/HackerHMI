#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// RP2350 không còn cấp phát tĩnh mảng 30KB buffer thô nữa.
// Mọi dữ liệu log từ ESP32-C5 sẽ được gửi thẳng qua SPI và hiển thị lên DWIN HMI
// hoặc ghi log nhẹ trên các biến ngắn của RP2350.
// RingBuffer này sẽ chỉ được định nghĩa cấu trúc tối giản không có mảng đệm tĩnh lớn.

typedef struct {
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} RingBuffer;

void term_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

#endif // TERMINAL_H
