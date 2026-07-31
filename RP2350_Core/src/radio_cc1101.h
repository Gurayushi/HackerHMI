#ifndef RADIO_CC1101_H
#define RADIO_CC1101_H

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdint.h>

// Xung đột SPI0 (Dành cho C6), nên gán CC1101 vào SPI1
#define CC1101_SPI_PORT spi1
#define CC1101_PIN_MISO 12
#define CC1101_PIN_CS   13
#define CC1101_PIN_SCK  14
#define CC1101_PIN_MOSI 15
#define CC1101_PIN_GDO0 10 // Chân ngắt tín hiệu

// Hàm khởi tạo module Radio CC1101
void cc1101_init(float frequency_mhz) {
    spi_init(CC1101_SPI_PORT, 5000 * 1000); // 5MHz
    gpio_set_function(CC1101_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(CC1101_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CC1101_PIN_MOSI, GPIO_FUNC_SPI);
    
    gpio_init(CC1101_PIN_CS);
    gpio_set_dir(CC1101_PIN_CS, GPIO_OUT);
    gpio_put(CC1101_PIN_CS, 1);
    
    // TODO: Gửi mảng lệnh (Register Array) qua SPI để cấu hình tần số (VD: 433.92 MHz)
    // Thiết lập Modulation: ASK/OOK (Dùng để mở cổng, cửa cuốn)
}

// Bắn chuỗi tín hiệu thô (Raw Signal) ra ngoài không khí
void cc1101_transmit_signal(uint8_t* payload, uint8_t length) {
    gpio_put(CC1101_PIN_CS, 0);
    // Gửi byte Header ghi dữ liệu
    // spi_write_blocking(...)
    gpio_put(CC1101_PIN_CS, 1);
    // TODO: Kéo chân GDO0 để ra lệnh phát sóng (TX Strobe)
}

#endif // RADIO_CC1101_H
