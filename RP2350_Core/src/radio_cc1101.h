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
    
    // Cấu hình cơ bản (Basic Configuration) - Không chứa cấu hình tấn công
}

// Hàm ghi 1 byte vào thanh ghi của CC1101 qua SPI
void cc1101_write_reg(uint8_t regAddr, uint8_t value) {
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &regAddr, 1);
    spi_write_blocking(CC1101_SPI_PORT, &value, 1);
    gpio_put(CC1101_PIN_CS, 1);
}

// Hàm đọc 1 byte từ thanh ghi của CC1101 qua SPI
uint8_t cc1101_read_reg(uint8_t regAddr) {
    uint8_t value;
    // Đặt bit Đọc (Read bit)
    regAddr |= 0x80; 
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &regAddr, 1);
    spi_read_blocking(CC1101_SPI_PORT, 0x00, &value, 1);
    gpio_put(CC1101_PIN_CS, 1);
    return value;
}

// Bắn chuỗi tín hiệu thô (Raw Signal) ra ngoài không khí
void cc1101_transmit_signal(uint8_t* payload, uint8_t length) {
    // Ghi dữ liệu vào TX FIFO (Thanh ghi 0x3F)
    gpio_put(CC1101_PIN_CS, 0);
    uint8_t tx_fifo_addr = 0x3F;
    spi_write_blocking(CC1101_SPI_PORT, &tx_fifo_addr, 1);
    spi_write_blocking(CC1101_SPI_PORT, payload, length);
    gpio_put(CC1101_PIN_CS, 1);
    
    // Kích hoạt TX (Lệnh STX)
    uint8_t stx_cmd = 0x35;
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &stx_cmd, 1);
    gpio_put(CC1101_PIN_CS, 1);
}

#endif // RADIO_CC1101_H
