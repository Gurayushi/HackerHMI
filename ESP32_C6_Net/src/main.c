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

// Hàm giả lập khởi tạo Web Server cấu hình (Captive Portal)
void start_web_config_portal() {
    ESP_LOGI(TAG, "Bật Web Portal ẩn để người dùng cấu hình IP, Username, Mật khẩu...");
    // TODO: Khởi tạo HTTP Server (Cổng 80) giao diện HTML form.
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
        // Nếu chưa cấu hình, bật chế độ Captive Portal cho người dùng nhập liệu
        start_web_config_portal();
    } else {
        // Nếu đã cấu hình, kết nối Wi-Fi và tiến hành SSH
        ESP_LOGI(TAG, "Đã có cấu hình. Đang kết nối SSH tới %s@%s...", target_user, target_ip);
        // wifi_init_sta();
        // start_ssh_client(target_ip, target_user, target_pass);
    }
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
