#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "HACKER_NET";

// Các biến cấu hình động (Sẽ được nạp từ bộ nhớ NVS)
char target_ip[32] = "";
char target_user[32] = "";
char target_pass[64] = "";

// Hàm giả lập đọc cấu hình từ NVS Flash
void load_config_from_nvs() {
    ESP_LOGI(TAG, "Đang tải cấu hình từ bộ nhớ NVS...");
    // TODO: Viết logic mở NVS và đọc các key: "ssh_ip", "ssh_user", "ssh_pass"
    // Nếu NVS trống (chưa có cấu hình), sẽ bật chế độ Wi-Fi Access Point (Phát Wi-Fi)
    // để người dùng dùng điện thoại kết nối vào và điền thông tin qua Web Portal.
}

// Hàm giả lập giao tiếp với RP2350 (HMI master)
void listen_to_rp2350_for_config() {
    ESP_LOGI(TAG, "Đang chờ RP2350 gửi cấu hình từ HMI Keyboard...");
    // TODO: Thiết lập ngắt UART hoặc SPI Slave
    // Khi người dùng gõ phím trên màn hình DWIN và bấm Save:
    // 1. DWIN gửi chuỗi UART đến RP2350
    // 2. RP2350 gửi gói dữ liệu (IP, Username, Password) sang ESP32-C6
    // 3. ESP32-C6 gọi lệnh nvs_set_str() để lưu vào bộ nhớ.
    // 4. ESP32-C6 tự khởi động lại (esp_restart) để áp dụng cấu hình mới.
}

void app_main(void)
{
    // 1. Khởi tạo bộ nhớ NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Bắt đầu khởi động ESP32-C6 HackerHMI");
    
    // 2. Tải cấu hình động
    load_config_from_nvs();

    if (strlen(target_ip) == 0) {
        // HMI trống, chờ người dùng bấm dấu [+] trên màn hình và gõ phím
        listen_to_rp2350_for_config();
    } else {
        // Đã có cấu hình trong NVS, tiến hành SSH
        ESP_LOGI(TAG, "Đã có cấu hình. Đang kết nối SSH tới %s@%s...", target_user, target_ip);
        // wifi_init_sta();
        // start_ssh_client(target_ip, target_user, target_pass);
    }
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
