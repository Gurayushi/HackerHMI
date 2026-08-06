#ifndef WIFI_DATABASE_H
#define WIFI_DATABASE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char ssid[33];
    char password[65];
    uint32_t conn_count;
} wifi_profile_t;

typedef struct {
    uint32_t count;
    wifi_profile_t profiles[100];
} wifi_db_t;

// Khởi tạo và đọc dữ liệu NVS
void wifi_db_init(void);

// Thêm mạng mới hoặc cập nhật mật khẩu mạng cũ (giải thuật LFU nếu danh sách đầy)
bool wifi_db_add_or_update(const char* ssid, const char* password);

// Tìm mật khẩu theo SSID
bool wifi_db_find(const char* ssid, char* out_password);

// Quên/Xóa mạng khỏi cơ sở dữ liệu
bool wifi_db_forget(const char* ssid);

// Tăng tần suất kết nối thành công (bảo vệ chống tràn số nguyên UINT32_MAX)
void wifi_db_increment_conn(const char* ssid);

// Lấy con trỏ đến cơ sở dữ liệu hiện tại
wifi_db_t* wifi_db_get(void);

#endif // WIFI_DATABASE_H
