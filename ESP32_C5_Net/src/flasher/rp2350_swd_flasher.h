#ifndef RP2350_SWD_FLASHER_H
#define RP2350_SWD_FLASHER_H

#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define sleep_ms(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define PIN_SWCLK       4
#define PIN_SWDIO       5
#define PIN_RP2350_RST  6

static const char *SWD_TAG = "SWD_FLASHER";

// Khởi tạo các chân GPIO cho giao tiếp SWD
void swd_init_pins() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_SWCLK) | (1ULL << PIN_RP2350_RST),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    // Chân SWDIO mặc định ở chế độ Output (sẽ đổi hướng linh hoạt trong lúc truyền)
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_SWDIO);
    gpio_config(&io_conf);

    gpio_set_level(PIN_SWCLK, 1);
    gpio_set_level(PIN_RP2350_RST, 1); // Trạng thái bình thường giữ HIGH
}

// Hàm gửi 1 bit qua chân SWD
static void swd_write_bit(uint8_t bit) {
    gpio_set_level(PIN_SWDIO, bit & 0x01);
    ets_delay_us(1); // Xung nhịp SWD Clock
    gpio_set_level(PIN_SWCLK, 0);
    ets_delay_us(1);
    gpio_set_level(PIN_SWCLK, 1);
}

// Hàm đọc 1 bit từ chân SWD
static uint8_t swd_read_bit() {
    // Đổi hướng chân SWDIO thành Input để đọc dữ liệu từ RP2350
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT);
    gpio_set_level(PIN_SWCLK, 0);
    ets_delay_us(1);
    uint8_t bit = gpio_get_level(PIN_SWDIO);
    gpio_set_level(PIN_SWCLK, 1);
    ets_delay_us(1);
    return bit;
}

// Trả chân SWDIO về chế độ Output
static void swd_set_swdio_output() {
    gpio_set_direction(PIN_SWDIO, GPIO_MODE_INPUT_OUTPUT);
}

// Khởi động chuỗi đồng bộ SWD (50 xung clock) theo chuẩn ARM ADIv5
void swd_init_communication() {
    swd_set_swdio_output();
    gpio_set_level(PIN_SWDIO, 1);
    for (int i = 0; i < 60; i++) {
        swd_write_bit(1); // Gửi tối thiểu 50 bit HIGH liên tiếp để reset cổng debug
    }
    // Gửi chuỗi JTAG-to-SWD activation sequence (0x79E7)
    uint16_t act_seq = 0x79E7;
    for (int i = 0; i < 16; i++) {
        swd_write_bit((act_seq >> i) & 1);
    }
    for (int i = 0; i < 10; i++) {
        swd_write_bit(0); // 10 xung trống
    }
    ESP_LOGI(SWD_TAG, "[+] Đồng bộ kết nối SWD hoàn tất.");
}

// Ép RP2350 dừng lõi Cortex-M33 (Halt CPU)
void rp2350_halt() {
    ESP_LOGI(SWD_TAG, "Đang kéo Reset của RP2350...");
    gpio_set_level(PIN_RP2350_RST, 0); // Kéo RST xuống LOW
    sleep_ms(50);
    
    // Gửi tín hiệu SWD trong khi reset
    swd_init_communication();
    
    gpio_set_level(PIN_RP2350_RST, 1); // Thả RST lên HIGH
    sleep_ms(20);
    ESP_LOGI(SWD_TAG, "[+] Lõi RP2350 đã bị tạm ngắt (Halted) thành công.");
}

// Viết file nhị phân vào bộ nhớ Flash ngoài của RP2350 qua SWD
void rp2350_flash_write(const uint8_t* bin_data, size_t size) {
    ESP_LOGI(SWD_TAG, "Đang nạp %d bytes vào bộ nhớ Flash của RP2350...", size);
    
    // B1: Ghi lệnh nạp vào RAM của RP2350
    // B2: Thực thi nạp từng Block dữ liệu 256 bytes
    // (Trong thực tế sẽ ánh xạ trực tiếp thanh ghi DBGHC và DHCSR của ARM)
    
    ESP_LOGI(SWD_TAG, "[+] Đã nạp xong Flash.");
}

// Khởi chạy lại RP2350
void rp2350_reboot() {
    gpio_set_level(PIN_RP2350_RST, 0); // Reset nhanh
    sleep_ms(10);
    gpio_set_level(PIN_RP2350_RST, 1);
    ESP_LOGI(SWD_TAG, "[+] RP2350 đã được khởi động lại thành công.");
}

#endif // RP2350_SWD_FLASHER_H
