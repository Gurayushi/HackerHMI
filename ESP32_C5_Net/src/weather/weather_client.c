#include "weather_client.h"
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char *TAG = "WEATHER";

// Getter cho wifi_event_group từ main.c
extern EventGroupHandle_t get_wifi_event_group(void);
#define WIFI_CONNECTED_BIT BIT0

extern void send_to_rp2350(const char *msg);

// Buffer lưu kết quả HTTP response
static char *response_buffer = NULL;
static int response_len = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (response_buffer == NULL) {
                    response_buffer = malloc(evt->data_len + 1);
                    if (response_buffer) {
                        memcpy(response_buffer, evt->data, evt->data_len);
                        response_len = evt->data_len;
                        response_buffer[response_len] = '\0';
                    }
                } else {
                    char *new_buf = realloc(response_buffer, response_len + evt->data_len + 1);
                    if (new_buf) {
                        response_buffer = new_buf;
                        memcpy(response_buffer + response_len, evt->data, evt->data_len);
                        response_len += evt->data_len;
                        response_buffer[response_len] = '\0';
                    }
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Lấy tọa độ kinh/vĩ độ bằng ip-api.com
static bool get_location(double *lat, double *lon) {
    if (response_buffer) {
        free(response_buffer);
        response_buffer = NULL;
    }
    response_len = 0;

    esp_http_client_config_t config = {
        .url = "http://ip-api.com/json/",
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for location");
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK && response_buffer) {
        cJSON *json = cJSON_Parse(response_buffer);
        if (json) {
            cJSON *status = cJSON_GetObjectItem(json, "status");
            if (status && strcmp(status->valuestring, "success") == 0) {
                cJSON *lat_item = cJSON_GetObjectItem(json, "lat");
                cJSON *lon_item = cJSON_GetObjectItem(json, "lon");
                if (lat_item && lon_item) {
                    *lat = lat_item->valuedouble;
                    *lon = lon_item->valuedouble;
                    ESP_LOGI(TAG, "Detected location lat: %f, lon: %f", *lat, *lon);
                    cJSON_Delete(json);
                    esp_http_client_cleanup(client);
                    return true;
                }
            }
            cJSON_Delete(json);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Location request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return false;
}

// Lấy dữ liệu thời tiết bằng Open-Meteo
static bool get_weather(double lat, double lon, int *temp, int *humidity, int *icon_id) {
    if (response_buffer) {
        free(response_buffer);
        response_buffer = NULL;
    }
    response_len = 0;

    char url[256];
    snprintf(url, sizeof(url), "http://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current=temperature_2m,relative_humidity_2m,weather_code", lat, lon);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for weather");
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK && response_buffer) {
        cJSON *json = cJSON_Parse(response_buffer);
        if (json) {
            cJSON *current = cJSON_GetObjectItem(json, "current");
            if (current) {
                cJSON *t_item = cJSON_GetObjectItem(current, "temperature_2m");
                cJSON *h_item = cJSON_GetObjectItem(current, "relative_humidity_2m");
                cJSON *code_item = cJSON_GetObjectItem(current, "weather_code");
                if (t_item && h_item && code_item) {
                    *temp = (int)(t_item->valuedouble + 0.5);
                    *humidity = h_item->valueint;
                    int wcode = code_item->valueint;
                    
                    // Ánh xạ mã thời tiết WMO sang Icon HMI DWIN
                    // 0 = Nắng, 1 = Mây, 2 = Mưa, 3 = Dông sét
                    if (wcode == 0) {
                        *icon_id = 0; // Trời quang (Nắng)
                    } else if (wcode >= 1 && wcode <= 3) {
                        *icon_id = 1; // Nhiều mây
                    } else if ((wcode >= 51 && wcode <= 67) || (wcode >= 80 && wcode <= 82)) {
                        *icon_id = 2; // Mưa dông nhẹ / Mưa rào
                    } else if (wcode >= 95) {
                        *icon_id = 3; // Dông sét bão
                    } else {
                        *icon_id = 1; // Mặc định nhiều mây
                    }
                    
                    ESP_LOGI(TAG, "Weather: Temp=%dC, Humid=%d%%, Code=%d (Icon=%d)", *temp, *humidity, wcode, *icon_id);
                    cJSON_Delete(json);
                    esp_http_client_cleanup(client);
                    return true;
                }
            }
            cJSON_Delete(json);
        }
    } else {
        ESP_LOGE(TAG, "HTTP Weather request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return false;
}

static bool get_air_quality(double lat, double lon, int *aqi) {
    if (response_buffer) {
        free(response_buffer);
        response_buffer = NULL;
    }
    response_len = 0;

    char url[256];
    snprintf(url, sizeof(url), "http://air-quality-api.open-meteo.com/v1/air-quality?latitude=%f&longitude=%f&current=us_aqi", lat, lon);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for AQI");
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK && response_buffer) {
        cJSON *json = cJSON_Parse(response_buffer);
        if (json) {
            cJSON *current = cJSON_GetObjectItem(json, "current");
            if (current) {
                cJSON *aqi_item = cJSON_GetObjectItem(current, "us_aqi");
                if (aqi_item) {
                    *aqi = aqi_item->valueint;
                    ESP_LOGI(TAG, "Air Quality: AQI=%d", *aqi);
                    cJSON_Delete(json);
                    esp_http_client_cleanup(client);
                    return true;
                }
            }
            cJSON_Delete(json);
        }
    } else {
        ESP_LOGE(TAG, "HTTP AQI request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return false;
}

static void weather_task(void *pvParameters) {
    ESP_LOGI(TAG, "Weather Task started. Waiting for Wi-Fi...");
    EventGroupHandle_t wifi_eg = get_wifi_event_group();
    if (!wifi_eg) {
        ESP_LOGE(TAG, "Wi-Fi Event Group is NULL. Delaying...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        wifi_eg = get_wifi_event_group();
    }
    
    if (wifi_eg) {
        xEventGroupWaitBits(wifi_eg, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    } else {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Tránh crash
    }
    
    ESP_LOGI(TAG, "Wi-Fi Connected. Fetching weather...");
    
    double lat = 21.0285; // Mặc định Hà Nội
    double lon = 105.8542;
    
    // Thử dò IP
    if (get_location(&lat, &lon)) {
        ESP_LOGI(TAG, "Successfully detected location via IP.");
    } else {
        ESP_LOGW(TAG, "Failed to detect location, using default (Hanoi).");
    }

    while (1) {
        int temp = 25;
        int humidity = 70;
        int icon_id = 0;
        int aqi = 50; // Mặc định AQI trung bình/tốt
        
        bool w_ok = get_weather(lat, lon, &temp, &humidity, &icon_id);
        bool aq_ok = get_air_quality(lat, lon, &aqi);
        
        if (w_ok || aq_ok) {
            // Định dạng chuỗi gửi sang RP2350: WTH:temp:humid:icon:aqi
            char spi_msg[64];
            snprintf(spi_msg, sizeof(spi_msg), "WTH:%d:%d:%d:%d", temp, humidity, icon_id, aqi);
            send_to_rp2350(spi_msg);
        } else {
            ESP_LOGE(TAG, "Failed to update weather or air quality data.");
        }
        
        // Chờ 30 phút (30 * 60 * 1000 ms)
        vTaskDelay(pdMS_TO_TICKS(30 * 60 * 1000));
    }
    vTaskDelete(NULL);
}

void weather_init(void) {
    xTaskCreate(weather_task, "weather_task", 8192, NULL, 4, NULL);
}
