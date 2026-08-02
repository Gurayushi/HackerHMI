#ifndef DEAUTHER_TERMINAL_H
#define DEAUTHER_TERMINAL_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "esp_heap_caps.h"

// Cấp phát 60KB trong PSRAM (hoặc SRAM) của ESP32-C5 cho Terminal Log
#define ESP_TERMINAL_BUFFER_SIZE (60 * 1024)

typedef struct {
    char *buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} EspRingBuffer;

static EspRingBuffer esp_terminal_buffer;

void esp_term_init(void) {
    // Ưu tiên cấp phát trong PSRAM (MALLOC_CAP_SPIRAM) nếu có, nếu không thì dùng SRAM thường
    esp_terminal_buffer.buffer = (char *)heap_caps_malloc(ESP_TERMINAL_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (esp_terminal_buffer.buffer == NULL) {
        esp_terminal_buffer.buffer = (char *)malloc(ESP_TERMINAL_BUFFER_SIZE);
    }
    esp_terminal_buffer.head = 0;
    esp_terminal_buffer.tail = 0;
    esp_terminal_buffer.count = 0;
    if (esp_terminal_buffer.buffer) {
        memset(esp_terminal_buffer.buffer, 0, ESP_TERMINAL_BUFFER_SIZE);
    }
}

void esp_term_push_char(char c) {
    if (!esp_terminal_buffer.buffer) return;
    esp_terminal_buffer.buffer[esp_terminal_buffer.head] = c;
    esp_terminal_buffer.head = (esp_terminal_buffer.head + 1) % ESP_TERMINAL_BUFFER_SIZE;
    if (esp_terminal_buffer.count < ESP_TERMINAL_BUFFER_SIZE) {
        esp_terminal_buffer.count++;
    } else {
        esp_terminal_buffer.tail = (esp_terminal_buffer.tail + 1) % ESP_TERMINAL_BUFFER_SIZE;
    }
}

void esp_term_push_string(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        esp_term_push_char(str[i]);
    }
}

#endif // DEAUTHER_TERMINAL_H
