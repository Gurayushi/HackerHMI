#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

// Thư viện SSH sẽ được compile từ idf_component.yml
#include "libssh2.h"

static const char *TAG = "HACKER_NET";

// Biến trạng thái Wi-Fi
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Các biến cấu hình động (Sẽ được nạp từ bộ nhớ NVS)
char wifi_ssid[32] = "";
char wifi_pass[64] = "";
char target_ip[32] = "";
char target_user[32] = "";
char target_pass[64] = "";

// Hàm giả lập giao tiếp với RP2350 (HMI master)
void listen_to_rp2350_for_config() {
    ESP_LOGI(TAG, "Đang chờ RP2350 gửi cấu hình từ HMI Keyboard...");
    // TODO: Thiết lập ngắt UART hoặc SPI Slave
    // Khi người dùng gõ phím trên màn hình DWIN và bấm Save:
    // 1. DWIN gửi chuỗi UART đến RP2350
    // 2. RP2350 gửi gói dữ liệu (SSID, PASS, IP, USER, PASS) sang ESP32-C5
    // 3. ESP32-C5 gọi lệnh nvs_set_str() để lưu vào bộ nhớ.
    // 4. ESP32-C5 tự khởi động lại (esp_restart) để áp dụng cấu hình mới.
}

// Hàm đọc NVS
void load_config_from_nvs() {
    ESP_LOGI(TAG, "Đang tải cấu hình từ NVS...");
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Lỗi mở NVS (%s)", esp_err_to_name(err));
        return;
    }

    size_t required_size = 0;
    
    // Đọc Target IP
    required_size = sizeof(target_ip);
    nvs_get_str(my_handle, "ssh_ip", target_ip, &required_size);
    
    // Đọc Target Username
    required_size = sizeof(target_user);
    nvs_get_str(my_handle, "ssh_user", target_user, &required_size);

    // Đọc Target Password
    required_size = sizeof(target_pass);
    nvs_get_str(my_handle, "ssh_pass", target_pass, &required_size);
    
    // Đọc Wi-Fi SSID
    required_size = sizeof(wifi_ssid);
    nvs_get_str(my_handle, "wifi_ssid", wifi_ssid, &required_size);

    // Đọc Wi-Fi Password
    required_size = sizeof(wifi_pass);
    nvs_get_str(my_handle, "wifi_pass", wifi_pass, &required_size);

    nvs_close(my_handle);
}

// Xử lý sự kiện Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Khởi động lại kết nối Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Đã lấy được IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Khởi tạo Wi-Fi Station
void wifi_init_sta(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold = {
                .authmode = WIFI_AUTH_WPA2_PSK,
            },
        },
    };
    
    // Copy cấu hình từ biến toàn cục (NVS)
    strcpy((char*)wifi_config.sta.ssid, wifi_ssid);
    strcpy((char*)wifi_config.sta.password, wifi_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi khởi tạo hoàn tất.");
}

// Luồng thực thi SSH (FreeRTOS Task)
void ssh_task(void *pvParameters) {
    ESP_LOGI(TAG, "Đang khởi tạo phiên SSH...");

    // Chờ Wi-Fi kết nối thành công mới chạy
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    
    ESP_LOGI(TAG, "Đang kết nối SSH tới %s@%s", target_user, target_ip);
    
    // 1. Khởi tạo TCP Socket tới target_ip cổng 22
    // 2. Khởi tạo libssh2 session: libssh2_session_init()
    // 3. Bắt tay (Handshake)
    // 4. Xác thực: libssh2_userauth_password()
    // 5. Mở kênh truyền: libssh2_channel_open_session()
    
    // Vòng lặp liên tục gửi lệnh (1 giây/lần) để lấy dữ liệu CPU/RAM
    while(1) {
        // Gửi lệnh: Get-Process | Select-Object -First 10
        // Đọc Output thô
        // (Tương lai) Phân tích Output và gửi về RP2350 qua SPI
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    // Đóng kênh và session khi kết thúc
    // libssh2_channel_free()
    // libssh2_session_disconnect()
    // libssh2_session_free()
    
    vTaskDelete(NULL);
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

    ESP_LOGI(TAG, "Bắt đầu khởi động ESP32-C5 HackerHMI");
    
    // 2. Tải cấu hình động
    load_config_from_nvs();

    if (strlen(target_ip) == 0 || strlen(wifi_ssid) == 0) {
        // HMI trống, chờ người dùng bấm dấu [+] trên màn hình và gõ phím
        listen_to_rp2350_for_config();
    } else {
        // Đã có cấu hình trong NVS, kết nối Wi-Fi
        wifi_init_sta();
        
        // Mở luồng đa nhiệm chạy ngầm phiên SSH
        xTaskCreate(&ssh_task, "ssh_task", 8192, NULL, 5, NULL);
    }
    
    // Vòng lặp rảnh rỗi của hàm main
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
