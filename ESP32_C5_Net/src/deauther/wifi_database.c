#include "wifi_database.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "WIFI_DB";
static wifi_db_t g_wifi_db = {0};

void wifi_db_init(void) {
    memset(&g_wifi_db, 0, sizeof(wifi_db_t));
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_db", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        size_t required_size = sizeof(wifi_db_t);
        err = nvs_get_blob(my_handle, "db", &g_wifi_db, &required_size);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Loi doc blob NVS (%s), tao moi CSDL", esp_err_to_name(err));
            g_wifi_db.count = 0;
        } else {
            ESP_LOGI(TAG, "Da doc thanh cong %lu profile Wi-Fi tu NVS", g_wifi_db.count);
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGI(TAG, "Chua co CSDL Wi-Fi luu tren NVS. Tao moi.");
    }
}

static void wifi_db_save(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("wifi_db", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(my_handle, "db", &g_wifi_db, sizeof(wifi_db_t));
        if (err == ESP_OK) {
            nvs_commit(my_handle);
            ESP_LOGI(TAG, "Da luu CSDL Wi-Fi vao NVS");
        } else {
            ESP_LOGE(TAG, "Loi ghi blob NVS (%s)", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }
}

bool wifi_db_add_or_update(const char* ssid, const char* password) {
    if (!ssid || strlen(ssid) == 0) return false;
    
    // Tim xem mang da ton tai chua
    for (uint32_t i = 0; i < g_wifi_db.count; i++) {
        if (strcmp(g_wifi_db.profiles[i].ssid, ssid) == 0) {
            // Cap nhat mat khau moi
            strncpy(g_wifi_db.profiles[i].password, password ? password : "", sizeof(g_wifi_db.profiles[i].password) - 1);
            g_wifi_db.profiles[i].password[sizeof(g_wifi_db.profiles[i].password) - 1] = '\0';
            wifi_db_save();
            return true;
        }
    }
    
    // Neu chua ton tai, kiem tra xem CSDL da day chua (100)
    if (g_wifi_db.count < 100) {
        wifi_profile_t *p = &g_wifi_db.profiles[g_wifi_db.count++];
        strncpy(p->ssid, ssid, sizeof(p->ssid) - 1);
        p->ssid[sizeof(p->ssid) - 1] = '\0';
        strncpy(p->password, password ? password : "", sizeof(p->password) - 1);
        p->password[sizeof(p->password) - 1] = '\0';
        p->conn_count = 0;
    } else {
        // Thuc thi giai thuat don dep LFU: tim phan tu co conn_count nho nhat
        uint32_t min_idx = 0;
        uint32_t min_conn = g_wifi_db.profiles[0].conn_count;
        for (uint32_t i = 1; i < 100; i++) {
            if (g_wifi_db.profiles[i].conn_count < min_conn) {
                min_conn = g_wifi_db.profiles[i].conn_count;
                min_idx = i;
            }
        }
        ESP_LOGI(TAG, "CSDL day, giai phong mang it ket noi nhat (LFU): %s (%lu lan)", 
                 g_wifi_db.profiles[min_idx].ssid, g_wifi_db.profiles[min_idx].conn_count);
                 
        // Dich chuyen mang de de phan tu min_idx
        for (uint32_t i = min_idx; i < 99; i++) {
            g_wifi_db.profiles[i] = g_wifi_db.profiles[i+1];
        }
        
        // Them vao vi tri cuoi
        wifi_profile_t *p = &g_wifi_db.profiles[99];
        strncpy(p->ssid, ssid, sizeof(p->ssid) - 1);
        p->ssid[sizeof(p->ssid) - 1] = '\0';
        strncpy(p->password, password ? password : "", sizeof(p->password) - 1);
        p->password[sizeof(p->password) - 1] = '\0';
        p->conn_count = 0;
    }
    
    wifi_db_save();
    return true;
}

bool wifi_db_find(const char* ssid, char* out_password) {
    if (!ssid) return false;
    for (uint32_t i = 0; i < g_wifi_db.count; i++) {
        if (strcmp(g_wifi_db.profiles[i].ssid, ssid) == 0) {
            strcpy(out_password, g_wifi_db.profiles[i].password);
            return true;
        }
    }
    return false;
}

bool wifi_db_forget(const char* ssid) {
    if (!ssid) return false;
    for (uint32_t i = 0; i < g_wifi_db.count; i++) {
        if (strcmp(g_wifi_db.profiles[i].ssid, ssid) == 0) {
            // Dich chuyen cac phan tu sau len de phan tu nay
            for (uint32_t j = i; j < g_wifi_db.count - 1; j++) {
                g_wifi_db.profiles[j] = g_wifi_db.profiles[j+1];
            }
            g_wifi_db.count--;
            memset(&g_wifi_db.profiles[g_wifi_db.count], 0, sizeof(wifi_profile_t));
            wifi_db_save();
            return true;
        }
    }
    return false;
}

void wifi_db_increment_conn(const char* ssid) {
    if (!ssid) return;
    for (uint32_t i = 0; i < g_wifi_db.count; i++) {
        if (strcmp(g_wifi_db.profiles[i].ssid, ssid) == 0) {
            // Chong tran can tren UINT32_MAX
            if (g_wifi_db.profiles[i].conn_count < UINT32_MAX) {
                g_wifi_db.profiles[i].conn_count++;
            }
            wifi_db_save();
            break;
        }
    }
}

wifi_db_t* wifi_db_get(void) {
    return &g_wifi_db;
}
