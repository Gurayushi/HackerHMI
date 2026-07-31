#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Dành riêng 30KB RAM cho Terminal Ring-Buffer (Khoảng ~500 dòng text)
#define TERMINAL_BUFFER_SIZE (30 * 1024)

typedef struct {
    char buffer[TERMINAL_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} RingBuffer;

// Khởi tạo bộ đệm
void term_init(RingBuffer *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    memset(rb->buffer, 0, TERMINAL_BUFFER_SIZE);
}

// Ghi 1 ký tự vào bộ đệm (Tự động ghi đè dữ liệu cũ nhất nếu đầy)
void term_push_char(RingBuffer *rb, char c) {
    rb->buffer[rb->head] = c;
    rb->head = (rb->head + 1) % TERMINAL_BUFFER_SIZE;
    if (rb->count < TERMINAL_BUFFER_SIZE) {
        rb->count++;
    } else {
        rb->tail = (rb->tail + 1) % TERMINAL_BUFFER_SIZE;
    }
}

// Giả lập thuật toán bóc tách màu sắc ANSI (Vd: \x1b[31m -> Đỏ)
void term_parse_ansi_and_push(RingBuffer *rb, const char* str) {
    // TODO: Viết thuật toán State Machine để loại bỏ ký tự \x1b
    // và tính toán mã màu tương ứng với hệ màu 16-bit của DWIN.
    // Tạm thời đẩy chuỗi thô vào buffer:
    for (int i = 0; str[i] != '\0'; i++) {
        term_push_char(rb, str[i]);
    }
}

#endif // TERMINAL_H
