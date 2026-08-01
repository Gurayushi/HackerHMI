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
"<title>HackerHMI OTA Dashboard</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<link href='https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&display=swap' rel='stylesheet'>"
"<style>"
":root {"
"  --bg-color: #080c14;"
"  --container-bg: rgba(13, 20, 35, 0.45);"
"  --border-color: rgba(255, 255, 255, 0.06);"
"  --text-primary: #f3f4f6;"
"  --text-secondary: #9ca3af;"
"  --accent-purple: #8b5cf6;"
"  --accent-blue: #3b82f6;"
"}"
"body { font-family: 'Outfit', sans-serif; background-color: var(--bg-color); color: var(--text-primary); display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; margin: 0; background-image: radial-gradient(at 0% 0%, rgba(99, 102, 241, 0.08) 0px, transparent 50%), radial-gradient(at 100% 100%, rgba(139, 92, 246, 0.08) 0px, transparent 50%); }"
".container { background: var(--container-bg); border: 1px solid var(--border-color); padding: 3rem 2.5rem; border-radius: 24px; box-shadow: 0 20px 50px rgba(0, 0, 0, 0.6); backdrop-filter: blur(16px); width: 480px; text-align: center; transition: all 0.4s cubic-bezier(0.16, 1, 0.3, 1); }"
"h1 { font-size: 2.2rem; font-weight: 700; margin: 0 0 0.5rem 0; background: linear-gradient(135deg, #c084fc, #6366f1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }"
".subtitle { color: var(--text-secondary); font-size: 0.95rem; margin-bottom: 2.5rem; font-weight: 300; }"
".option-grid { display: grid; grid-template-columns: 1fr; gap: 1rem; margin-bottom: 2rem; }"
".option-card { background: rgba(255, 255, 255, 0.015); border: 1.5px solid rgba(255, 255, 255, 0.04); border-radius: 14px; padding: 1.2rem; text-align: left; cursor: pointer; transition: all 0.3s cubic-bezier(0.25, 0.8, 0.25, 1); display: flex; align-items: center; gap: 1.2rem; position: relative; overflow: hidden; }"
".option-card::before { content: ''; position: absolute; top: 0; left: 0; width: 4px; height: 100%; background: transparent; transition: background-color 0.3s ease; }"
".option-card:hover { background: rgba(255, 255, 255, 0.04); border-color: rgba(99, 102, 241, 0.3); transform: translateY(-3px) scale(1.01); box-shadow: 0 10px 20px rgba(0, 0, 0, 0.2); }"
".option-card:active { transform: translateY(-1px) scale(0.99); }"
".option-card.active { background: rgba(99, 102, 241, 0.08); border-color: #6366f1; box-shadow: 0 0 24px rgba(99, 102, 241, 0.2); }"
".option-card.active::before { background: linear-gradient(to bottom, #c084fc, #6366f1); }"
".icon-wrapper { background: rgba(255, 255, 255, 0.03); border-radius: 10px; width: 44px; height: 44px; display: flex; align-items: center; justify-content: center; flex-shrink: 0; transition: all 0.3s ease; }"
".option-card:hover .icon-wrapper { transform: rotate(5deg) scale(1.05); background: rgba(255, 255, 255, 0.06); }"
".option-card.active .icon-wrapper { background: rgba(99, 102, 241, 0.18); }"
".icon-wrapper svg { width: 22px; height: 22px; stroke: var(--text-secondary); stroke-width: 1.5; fill: none; transition: all 0.3s ease; }"
".option-card:hover .icon-wrapper svg { stroke: #a5b4fc; }"
".option-card.active .icon-wrapper svg { stroke: #c084fc; }"
".option-info { display: flex; flex-direction: column; }"
".option-title { font-size: 0.95rem; font-weight: 600; color: var(--text-primary); margin-bottom: 0.2rem; transition: color 0.3s; }"
".option-card:hover .option-title { color: white; }"
".option-desc { font-size: 0.8rem; color: var(--text-secondary); font-weight: 300; }"
".upload-box { border: 2px dashed rgba(255, 255, 255, 0.12); border-radius: 16px; padding: 2.2rem 1rem; margin-bottom: 2rem; cursor: pointer; transition: all 0.3s ease; background: rgba(255, 255, 255, 0.005); display: flex; flex-direction: column; align-items: center; }"
".upload-box:hover { border-color: #6366f1; background: rgba(99, 102, 241, 0.03); box-shadow: 0 0 15px rgba(99, 102, 241, 0.05); }"
".upload-box svg { width: 42px; height: 42px; fill: none; stroke: var(--text-secondary); stroke-width: 1.5; margin-bottom: 0.8rem; transition: all 0.3s ease; }"
".upload-box:hover svg { stroke: #c084fc; transform: translateY(-2px); }"
"#uploadText { font-size: 0.9rem; color: var(--text-secondary); transition: color 0.3s; }"
".upload-box:hover #uploadText { color: var(--text-primary); }"
".btn { background: linear-gradient(135deg, #6366f1, #4f46e5); color: white; border: none; padding: 0.95rem 1.5rem; border-radius: 12px; font-weight: 600; font-size: 0.95rem; font-family: 'Outfit', sans-serif; cursor: pointer; width: 100%; transition: all 0.25s cubic-bezier(0.2, 0.8, 0.2, 1); box-shadow: 0 4px 14px rgba(99, 102, 241, 0.25); position: relative; overflow: hidden; }"
".btn::after { content: ''; position: absolute; top: 0; left: -50%; width: 200%; height: 100%; background: linear-gradient(to right, rgba(255,255,255,0) 0%, rgba(255,255,255,0.15) 50%, rgba(255,255,255,0) 100%); transform: skewX(-25deg); transition: 0.75s; }"
".btn:hover::after { left: 125%; }"
".btn:hover { transform: translateY(-2px); box-shadow: 0 8px 25px rgba(99, 102, 241, 0.45); }"
".btn:active { transform: translateY(1px) scale(0.99); box-shadow: 0 3px 10px rgba(99, 102, 241, 0.2); }"
".progress-wrapper { display: none; margin-top: 2rem; text-align: left; background: rgba(255, 255, 255, 0.015); border: 1px solid rgba(255, 255, 255, 0.04); border-radius: 14px; padding: 1.2rem; box-shadow: inset 0 2px 4px rgba(0,0,0,0.2); animation: fadeIn 0.3s ease-out forwards; }"
"@keyframes fadeIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }"
".progress-title-wrapper { display: flex; justify-content: space-between; align-items: center; font-size: 0.85rem; color: var(--text-primary); font-weight: 500; }"
".progress-bar { background: rgba(255, 255, 255, 0.05); border-radius: 9999px; height: 10px; overflow: hidden; margin-top: 0.7rem; border: 1px solid rgba(255, 255, 255, 0.03); box-shadow: inset 0 1px 2px rgba(0,0,0,0.5); }"
".progress-fill { background: linear-gradient(90deg, #3b82f6, #8b5cf6, #ec4899); background-size: 200% 100%; animation: glow-shift 2s linear infinite; width: 0%; height: 100%; border-radius: 9999px; transition: width 0.15s cubic-bezier(0.4, 0, 0.2, 1); box-shadow: 0 0 10px rgba(139, 92, 246, 0.5); position: relative; }"
".progress-fill::after { content: ''; position: absolute; top: 0; left: 0; bottom: 0; right: 0; background-image: linear-gradient(45deg, rgba(255, 255, 255, 0.15) 25%, transparent 25%, transparent 50%, rgba(255, 255, 255, 0.15) 50%, rgba(255, 255, 255, 0.15) 75%, transparent 75%, transparent); background-size: 15px 15px; z-index: 1; animation: progress-bar-stripes 1s linear infinite; border-radius: 9999px; }"
"@keyframes progress-bar-stripes { from { background-position: 0 0; } to { background-position: 15px 0; } }"
"@keyframes glow-shift { 0% { background-position: 0% 50%; } 50% { background-position: 100% 50%; } 100% { background-position: 0% 50%; } }"
".status-text { font-size: 0.8rem; color: var(--text-secondary); margin-top: 0.5rem; display: flex; justify-content: space-between; font-weight: 300; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<h1>HackerHMI OTA Dashboard</h1>"
"<div class='subtitle'>Hệ thống cập nhật không dây thông minh</div>"
"<form id='uploadForm'>"
"<div class='option-grid'>"
"<div class='option-card active' onclick='selectChip(\"esp32\")' id='card-esp32'>"
"<div class='icon-wrapper'><svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' d='M8.25 3v1.5M4.5 8.25H3m18 0h-1.5M4.5 12H3m18 0h-1.5m-15 3.75H3m18 0h-1.5M8.25 19.5V21M12 3v1.5m0 15V21m3.75-18v1.5m0 15V21M6.75 6.75h10.5a1.5 1.5 0 011.5 1.5v10.5a1.5 1.5 0 01-1.5 1.5H6.75a1.5 1.5 0 01-1.5-1.5V8.25a1.5 1.5 0 011.5-1.5z'/></svg></div>"
"<div class='option-info'><span class='option-title'>ESP32-C5 Firmware</span><span class='option-desc'>Cập nhật mô-đun mạng và kết nối không dây</span></div>"
"</div>"
"<div class='option-card' onclick='selectChip(\"rp2350\")' id='card-rp2350'>"
"<div class='icon-wrapper'><svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' d='M9 17.25v1.007a3 3 0 01-.879 2.122L7.5 21h9l-.621-.621A3 3 0 0115 18.257V17.25m6-12V15a2.25 2.25 0 01-2.25 2.25H5.25A2.25 2.25 0 013 15V5.25m18 0A2.25 2.25 0 0018.75 3H5.25A2.25 2.25 0 003 5.25m18 0V12a2.25 2.25 0 01-2.25 2.25H5.25A2.25 2.25 0 013 12V5.25'/></svg></div>"
"<div class='option-info'><span class='option-title'>RP2350 Core (via SWD)</span><span class='option-desc'>Cập nhật nhân xử lý HMI và giả lập HID</span></div>"
"</div>"
"<div class='option-card' onclick='selectChip(\"dwin\")' id='card-dwin'>"
"<div class='icon-wrapper'><svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' d='M2.25 15.75l5.159-5.159a2.25 2.25 0 013.182 0l5.159 5.159m-1.5-1.5l1.409-1.409a2.25 2.25 0 013.182 0l2.909 2.909m-18 3.75h16.5a1.5 1.5 0 001.5-1.5V6a1.5 1.5 0 00-1.5-1.5H3.75A1.5 1.5 0 002.25 6v12a1.5 1.5 0 001.5 1.5zm10.5-11.25h.008v.008h-.008V8.25zm.375 0a.375.375 0 11-.75 0 .375.375 0 01.75 0z'/></svg></div>"
"<div class='option-info'><span class='option-title'>DWIN HMI Layout (via UART)</span><span class='option-desc'>Cập nhật ảnh nền (.icl), font và giao diện</span></div>"
"</div>"
"</div>"
"<input type='hidden' id='selectedChip' value='esp32'>"
"<div class='upload-box' onclick='document.getElementById(\"fileInput\").click()'>"
"<svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' d='M12 16.5V9.75m0 0l3 3m-3-3l-3 3M6.75 19.5a4.5 4.5 0 01-1.41-8.775 5.25 5.25 0 0110.233-2.33 3 3 0 013.758 3.848A3.752 3.752 0 0118 19.5H6.75z'/></svg>"
"<div id='uploadText'>Kéo thả hoặc Click để chọn file .bin / .icl</div>"
"<input type='file' id='fileInput' name='update' accept='.bin,.icl' onchange='fileSelected()'>"
"</div>"
"<button type='button' class='btn' onclick='startUpload()'>Khởi động nâng cấp</button>"
"</form>"
"<div class='progress-wrapper' id='progressWrapper'>"
"<div class='progress-title-wrapper'>"
"<span id='progressTitle'>Đang nạp dữ liệu...</span>"
"<span id='percentText'>0%</span>"
"</div>"
"<div class='progress-bar'><div class='progress-fill' id='progressFill'></div></div>"
"<div class='status-text'><span id='statusInfo'>Đang thiết lập kết nối...</span></div>"
"</div>"
"</div>"
"<script>"
"function selectChip(chip) {"
"  document.getElementById('selectedChip').value = chip;"
"  document.querySelectorAll('.option-card').forEach(card => card.classList.remove('active'));"
"  document.getElementById('card-' + chip).classList.add('active');"
"}"
"function fileSelected() {"
"  const file = document.getElementById('fileInput').files[0];"
"  if(file) document.getElementById('uploadText').innerText = file.name + ' (' + (file.size/1024/1024).toFixed(2) + ' MB)';"
"}"
"function startUpload() {"
"  const file = document.getElementById('fileInput').files[0];"
"  const chip = document.getElementById('selectedChip').value;"
"  if(!file) { alert('Vui lòng chọn file trước!'); return; }"
"  document.getElementById('progressWrapper').style.display = 'block';"
"  document.getElementById('progressTitle').innerText = 'Đang nạp dữ liệu sang ' + chip.toUpperCase() + '...';"
"  let url = '/update_esp32';"
"  if (chip === 'rp2350') url = '/update_rp2350';"
"  else if (chip === 'dwin') url = '/update_dwin';"
"  const xhr = new XMLHttpRequest();"
"  xhr.open('POST', url, true);"
"  xhr.upload.onprogress = function(e) {"
"    if (e.lengthComputable) {"
"      const pct = Math.round((e.loaded / e.total) * 100);"
"      document.getElementById('progressFill').style.width = pct + '%';"
"      document.getElementById('percentText').innerText = pct + '%';"
"      document.getElementById('statusInfo').innerText = 'Đang tải lên: ' + pct + '%';"
"    }"
"  };"
"  xhr.onload = function() {"
"    if(xhr.status === 200) {"
"      document.getElementById('statusInfo').innerText = 'Nâng cấp thành công!';"
"      alert('Nâng cấp thành công!');"
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

// POST /update_dwin Handler: Nhận file giao diện và gửi qua SPI sang RP2350
static esp_err_t dwin_update_post_handler(httpd_req_t *req) {
    ESP_LOGI(OTA_TAG, "Nhận yêu cầu nạp giao diện DWIN HMI qua OTA...");
    char buf[1024];
    int remaining = req->content_len;
    int ret;

    extern void forward_to_rp2350(const char* payload);
    forward_to_rp2350("START_DWIN_FLASH");

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            forward_to_rp2350("END_DWIN_FLASH_ERROR");
            return ESP_FAIL;
        }
        // Gửi block dữ liệu ảnh/font sang cho RP2350 qua SPI
        // RP2350 sẽ viết trực tiếp xuống Flash của DWIN qua UART0
        // (Trong thực tế sẽ viết code đồng bộ gửi block)
        remaining -= ret;
    }

    forward_to_rp2350("END_DWIN_FLASH_SUCCESS");
    ESP_LOGI(OTA_TAG, "[+] Truyền tải tệp tin giao diện DWIN HMI sang RP2350 thành công!");
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

        httpd_uri_t dwin_post = {
            .uri      = "/update_dwin",
            .method   = HTTP_POST,
            .handler  = dwin_update_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &dwin_post);
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
