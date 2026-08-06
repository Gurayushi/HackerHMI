#include "wifi_auto_connect.h"
#include "wifi_database.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char* TAG = "WIFI_AUTO";
static TaskHandle_t s_auto_connect_task_hdl = NULL;
static bool s_auto_connect_enabled = false;

extern EventGroupHandle_t get_wifi_event_group(void);
#define WIFI_CONNECTED_BIT BIT0

static void wifi_auto_connect_task(void *pvParameters) {
    ESP_LOGI(TAG, "Khoi dong Wifi Auto Connect Task...");
    wifi_db_t *db = wifi_db_get();
    
    while (s_auto_connect_enabled) {
        // Kiem tra neu da connected thi dung lai va delay 15s truoc khi check tiep
        EventGroupHandle_t ev_grp = get_wifi_event_group();
        if (ev_grp && (xEventGroupGetBits(ev_grp) & WIFI_CONNECTED_BIT)) {
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }
        
        if (db->count == 0) {
            vTaskDelay(pdMS_TO_TICKS(15000));
            continue;
        }
        
        ESP_LOGI(TAG, "Mat ket noi WiFi. Dang quet tim mang cu da luu...");
        
        wifi_scan_config_t scan_config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false
        };
        
        // Scan chan (blocking)
        esp_err_t err = esp_wifi_scan_start(&scan_config, true);
        if (err == ESP_OK) {
            uint16_t ap_count = 0;
            esp_wifi_scan_get_ap_num(&ap_count);
            if (ap_count > 0) {
                wifi_ap_record_t *ap_list = malloc(sizeof(wifi_ap_record_t) * ap_count);
                if (ap_list && esp_wifi_scan_get_ap_records(&ap_count, ap_list) == ESP_OK) {
                    bool matched = false;
                    for (int i = 0; i < ap_count; i++) {
                        char saved_pass[65] = {0};
                        if (wifi_db_find((const char*)ap_list[i].ssid, saved_pass)) {
                            ESP_LOGI(TAG, "Phat hien SSID hop le: %s. Dang ket noi tu dong...", ap_list[i].ssid);
                            
                            // Ngat cac phien dang co
                            esp_wifi_disconnect();
                            
                            wifi_config_t wifi_config = {0};
                            strcpy((char*)wifi_config.sta.ssid, (const char*)ap_list[i].ssid);
                            strcpy((char*)wifi_config.sta.password, saved_pass);
                            
                            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
                            esp_wifi_connect();
                            matched = true;
                            break;
                        }
                    }
                    if (!matched) {
                        ESP_LOGI(TAG, "Khong tim thay mang nao trong danh sach SSID da luu.");
                    }
                }
                if (ap_list) free(ap_list);
            }
        } else {
            ESP_LOGE(TAG, "Loi quet Wi-Fi: %s", esp_err_to_name(err));
        }
        
        // Nghi 15 giay nhu order cua user truoc khi quet tiep
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
    s_auto_connect_task_hdl = NULL;
    vTaskDelete(NULL);
}

void wifi_auto_connect_start(void) {
    if (s_auto_connect_enabled) return;
    s_auto_connect_enabled = true;
    xTaskCreate(wifi_auto_connect_task, "wifi_auto", 4096, NULL, 5, &s_auto_connect_task_hdl);
}

void wifi_auto_connect_stop(void) {
    s_auto_connect_enabled = false;
}
