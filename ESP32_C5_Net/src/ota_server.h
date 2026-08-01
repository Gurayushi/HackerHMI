#ifndef OTA_SERVER_H
#define OTA_SERVER_H

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

static const char *OTA_TAG = "OTA_SERVER";
static httpd_handle_t server = NULL;

// File HTML giao diện nạp OTA được nhúng trực tiếp dưới dạng chuỗi (Sleek Dark Mode UI)
static const char* ota_html_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='utf-8'>"
"<title>HackerHMI OTA Panel</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>"
"body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background-color: #0b0f19; color: #f3f4f6; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; margin: 0; }"
".container { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.05); padding: 2.5rem; border-radius: 16px; box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37); backdrop-filter: blur(8px); width: 420px; text-align: center; }"
"h1 { font-size: 1.8rem; margin-bottom: 0.5rem; background: linear-gradient(135deg, #a78bfa, #818cf8); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }"
"p { color: #9ca3af; font-size: 0.9rem; margin-bottom: 2rem; }"
".upload-box { border: 2px dashed rgba(255, 255, 255, 0.15); border-radius: 12px; padding: 2rem 1rem; margin-bottom: 1.5rem; cursor: pointer; transition: all 0.3s ease; background: rgba(255, 255, 255, 0.01); }"
".upload-box:hover { border-color: #818cf8; background: rgba(129, 140, 248, 0.05); }"
".upload-box svg { width: 48px; height: 48px; fill: none; stroke: #818cf8; stroke-width: 1.5; margin-bottom: 1rem; }"
"input[type='file'] { display: none; }"
".btn { background: linear-gradient(135deg, #6366f1, #4f46e5); color: white; border: none; padding: 0.75rem 1.5rem; border-radius: 8px; font-weight: 600; cursor: pointer; width: 100%; transition: opacity 0.2s; }"
".btn:hover { opacity: 0.9; }"
".progress-wrapper { display: none; margin-top: 1.5rem; text-align: left; }"
".progress-bar { background: rgba(255, 255, 255, 0.08); border-radius: 9999px; height: 8px; overflow: hidden; margin-top: 0.5rem; }"
".progress-fill { background: linear-gradient(90deg, #3b82f6, #8b5cf6); width: 0%; height: 100%; transition: width 0.1s ease; }"
".status-text { font-size: 0.8rem; color: #9ca3af; margin-top: 0.25rem; display: flex; justify-content: space-between; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<h1>HackerHMI OTA Control</h1>"
"<p>Chọn hệ chip đích và tải lên file Firmware để tiến hành nâng cấp không dây.</p>"
"<form id='uploadForm'>"
"<div class='upload-box' onclick='document.getElementById(\"fileInput\").click()'>"
"<svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' d='M12 16.5V9.75m0 0l3 3m-3-3l-3 3M6.75 19.5a4.5 4.5 0 01-1.41-8.775 5.25 5.25 0 0110.233-2.33 3 3 0 013.758 3.848A3.752 3.752 0 0118 19.5H6.75z'/></svg>"
"<div id='uploadText'>Kéo thả hoặc Click để chọn file .bin</div>"
"<input type='file' id='fileInput' name='update' accept='.bin' onchange='fileSelected()'>"
"</div>"
"<select id='chipSelect' class='btn' style='background: rgba(255,255,255,0.06); border: 1px solid rgba(255,255,255,0.1); margin-bottom: 1.5rem; outline: none;'>"
"<option value='esp32' style='background:#0b0f19;'>Nâng cấp ESP32-C5 (Wi-Fi SoC)</option>"
"<option value='rp2350' style='background:#0b0f19;'>Nâng cấp RP2350 (HMI Core via SWD)</option>"
"</select>"
"<button type='button' class='btn' onclick='startUpload()'>Khởi động Nâng cấp (Upload)</button>"
"</form>"
"<div class='progress-wrapper' id='progressWrapper'>"
"<div>Đang truyền và flash dữ liệu...</div>"
"<div class='progress-bar'><div class='progress-fill' id='progressFill'></div></div>"
"<div class='status-text'><span id='percentText'>0%</span><span id='statusInfo'>Đang xử lý...</span></div>"
"</div>"
"</div>"
"<script>"
"function fileSelected() {"
"  const file = document.getElementById('fileInput').files[0];"
"  if(file) document.getElementById('uploadText').innerText = file.name + ' (' + (file.size/1024/1024).toFixed(2) + ' MB)';"
"}"
"function startUpload() {"
"  const file = document.getElementById('fileInput').files[0];"
"  const chip = document.getElementById('chipSelect').value;"
"  if(!file) { alert('Vui lòng chọn file .bin trước!'); return; }"
"  document.getElementById('progressWrapper').style.display = 'block';"
"  const xhr = new XMLHttpRequest();"
"  const url = chip === 'esp32' ? '/update_esp32' : '/update_rp2350';"
"  xhr.open('POST', url, true);"
"  xhr.upload.onprogress = function(e) {"
"    if (e.lengthComputable) {"
"      const pct = Math.round((e.loaded / e.total) * 100);"
"      document.getElementById('progressFill').style.width = pct + '%';"
"      document.getElementById('percentText').innerText = pct + '%';"
"    }"
"  };"
"  xhr.onload = function() {"
"    if(xhr.status === 200) {"
"      document.getElementById('statusInfo').innerText = 'Hoàn tất! Đang khởi động lại mạch...';"
"      alert('Nâng cấp thành công! Hệ thống đang tự động Reboot.');"
"    } else {"
"      alert('Nâng cấp thất bại: ' + xhr.responseText);"
"    }"
"  };"
"  const formData = new FormData();"
"  formData.append('update', file);"
"  xhr.send(formData);"
"}"
"</script>"
"</body>"
"</html>";

// GET / Handler: Trả về trang HTML
static esp_err_t ota_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, ota_html_page, HTTPD_RESP_USE_ILLEGAL_LEN);
}

// POST /update_esp32 Handler: Nhận và flash OTA phân vùng ESP32
static esp_err_t esp32_update_post_handler(httpd_req_t *req) {
    ESP_LOGI(OTA_TAG, "Nhận yêu cầu nạp firmware cho ESP32-C5...");
    char buf[1024];
    int remaining = req->content_len;
    int ret;
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    if (update_partition == NULL) {
        ESP_LOGE(OTA_TAG, "Không tìm thấy phân vùng nâng cấp OTA!");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Partition missing");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Lỗi khởi chạy OTA: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA Begin failed");
        return ESP_FAIL;
    }

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(OTA_TAG, "Lỗi kết nối Socket nhận file!");
            esp_ota_end(update_handle);
            return ESP_FAIL;
        }

        err = esp_ota_write(update_handle, (const void *)buf, ret);
        if (err != ESP_OK) {
            ESP_LOGE(OTA_TAG, "Lỗi ghi flash OTA: %s", esp_err_to_name(err));
            esp_ota_end(update_handle);
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Lỗi đóng tiến trình OTA: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "Lỗi gán phân vùng khởi động mới: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }

    ESP_LOGI(OTA_TAG, "[+] Flash firmware ESP32-C5 hoàn tất! Hệ thống sẽ reboot sau 1 giây.");
    httpd_resp_sendstr(req, "Success");
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

// POST /update_rp2350 Handler: Nhận file và nạp trực tiếp cho RP2350 qua SWD
static esp_err_t rp2350_update_post_handler(httpd_req_t *req) {
    ESP_LOGI(OTA_TAG, "Nhận yêu cầu nạp firmware cho RP2350 qua SWD...");
    char buf[1024];
    int remaining = req->content_len;
    int ret;
    
    // Halt lõi của RP2350 để chiếm quyền ghi Flash ngoài
    extern void rp2350_halt();
    extern void rp2350_flash_write(const uint8_t* bin_data, size_t size);
    extern void rp2350_reboot();
    
    rp2350_halt();

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            rp2350_reboot();
            return ESP_FAIL;
        }
        // Ghi tuần tự từng khối đệm vào flash ngoài của RP2350 qua SWD
        rp2350_flash_write((const uint8_t*)buf, ret);
        remaining -= ret;
    }

    // Khởi chạy lại RP2350 để hoàn tất nạp
    rp2350_reboot();
    
    ESP_LOGI(OTA_TAG, "[+] Nạp cứu hộ/nâng cấp RP2350 qua SWD hoàn tất!");
    httpd_resp_sendstr(req, "Success");
    return ESP_OK;
}

// Khởi chạy Web Server nạp OTA (Cổng 80)
void start_ota_webserver() {
    if (server != NULL) return;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 4;
    config.stack_size = 8192;

    ESP_LOGI(OTA_TAG, "Đang khởi động Web Server nạp OTA...");
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ota_get = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = ota_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &ota_get);

        httpd_uri_t esp32_post = {
            .uri      = "/update_esp32",
            .method   = HTTP_POST,
            .handler  = esp32_update_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &esp32_post);

        httpd_uri_t rp2350_post = {
            .uri      = "/update_rp2350",
            .method   = HTTP_POST,
            .handler  = rp2350_update_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &rp2350_post);
        ESP_LOGI(OTA_TAG, "[+] OTA Web Server đang chạy tại cổng 80.");
    }
}

// Dừng Web Server nạp OTA
void stop_ota_webserver() {
    if (server != NULL) {
        ESP_LOGI(OTA_TAG, "Đang tắt OTA Web Server để tiết kiệm năng lượng...");
        httpd_stop(server);
        server = NULL;
    }
}

#endif // OTA_SERVER_H
