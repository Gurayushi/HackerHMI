#ifndef RFID_NFC_H
#define RFID_NFC_H

#include "hardware/i2c.h"
#include "hardware/uart.h"

// Cấu hình PN532 (NFC 13.56MHz) qua I2C
#define PN532_I2C_PORT i2c0
#define PN532_SDA_PIN 4
#define PN532_SCL_PIN 5

// Cấu hình RDM6300 (RFID 125kHz) qua UART1
#define RDM6300_UART_ID uart1
#define RDM6300_TX_PIN 8
#define RDM6300_RX_PIN 9

void rfid_nfc_init() {
    // 1. Khởi tạo I2C cho PN532
    i2c_init(PN532_I2C_PORT, 400 * 1000);
    gpio_set_function(PN532_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PN532_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PN532_SDA_PIN);
    gpio_pull_up(PN532_SCL_PIN);
    
    // 2. Khởi tạo UART1 cho RDM6300
    uart_init(RDM6300_UART_ID, 9600); // RDM6300 mặc định chạy ở 9600 baud
    gpio_set_function(RDM6300_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RDM6300_RX_PIN, GPIO_FUNC_UART);
}

// Kiểm tra xem có thẻ từ 125kHz (Thẻ thang máy cũ) đưa vào không
bool rdm6300_read_card(char* out_uid) {
    if (uart_is_readable(RDM6300_UART_ID)) {
        // RDM6300 sẽ nhả ra 14 byte: [0x02] [10 byte ASCII DATA] [2 byte Checksum] [0x03]
        // TODO: Đọc UART và tính Checksum.
        return true;
    }
    return false;
}

// Hàm gửi khung dữ liệu thô qua I2C cho PN532
void pn532_i2c_write(uint8_t* data, uint8_t length) {
    i2c_write_blocking(PN532_I2C_PORT, 0x24, data, length, false);
}

// Hàm đọc khung dữ liệu thô từ PN532 qua I2C
void pn532_i2c_read(uint8_t* buffer, uint8_t length) {
    // Đọc trạng thái Ready (Cần chờ thanh ghi trạng thái báo sẵn sàng)
    uint8_t status = 0;
    while (status != 0x01) {
        i2c_read_blocking(PN532_I2C_PORT, 0x24, &status, 1, false);
        sleep_ms(2);
    }
    // Đọc khung dữ liệu
    i2c_read_blocking(PN532_I2C_PORT, 0x24, buffer, length, false);
}

// Hàm kiểm tra cơ bản sự tồn tại của thẻ (Không chứa logic sao chép)
bool pn532_read_nfc(uint8_t* out_uid, uint8_t* uid_len) {
    // Lệnh InListPassiveTarget (0x4A) để quét thẻ
    uint8_t cmd_scan[] = {0x00, 0x00, 0xFF, 0x04, 0xFC, 0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00};
    pn532_i2c_write(cmd_scan, sizeof(cmd_scan));
    
    // Đọc ACK và Response (Chỉ minh họa giao thức cơ bản)
    uint8_t response[20];
    pn532_i2c_read(response, 20);
    
    return false; // Cần phân tích Response để lấy UID hợp lệ
}

#endif // RFID_NFC_H
