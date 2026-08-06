#include "ir_web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "IR_WEB_SERVER";
static httpd_handle_t ir_server = NULL;

extern void send_to_rp2350(const char *msg);
extern void stop_ir_webserver(void);

// Trang HTML giao diện tối giản Neon cho IR Uploader
static const char* ir_html_page = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<meta charset='utf-8'>"
"<title>HackerHMI IR Uploader</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no'>"
"<link href='https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&display=swap' rel='stylesheet'>"
"<style>"
":root {"
"  --bg-color: #030307;"
"  --container-bg: rgba(6, 6, 12, 0.85);"
"  --border-color: rgba(139, 92, 246, 0.15);"
"  --text-primary: #f3f4f6;"
"  --text-secondary: #a78bfa;"
"  --accent-neon: #bc39fa;"
"  --accent-glow: rgba(188, 57, 250, 0.5);"
"  --neon-blue: #3b82f6;"
"}"
"*, *::before, *::after { box-sizing: border-box; }"
"body { font-family: 'Outfit', sans-serif; background-color: var(--bg-color); color: var(--text-primary); display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; margin: 0; padding: 1rem; overflow-x: hidden; background-image: radial-gradient(circle at 10% 20%, rgba(139, 92, 246, 0.12) 0%, transparent 40%), radial-gradient(circle at 90% 80%, rgba(188, 57, 250, 0.12) 0%, transparent 40%); }"
".container { background: var(--container-bg); border: 2px solid var(--border-color); padding: 2.5rem 2rem; border-radius: 24px; box-shadow: 0 0 30px rgba(139, 92, 246, 0.15), 0 15px 50px rgba(0, 0, 0, 0.8), inset 0 0 15px rgba(139, 92, 246, 0.1); backdrop-filter: blur(20px); width: 100%; max-width: 500px; text-align: center; position: relative; z-index: 10; transition: all 0.4s ease; }"
".container:hover { border-color: rgba(188, 57, 250, 0.4); box-shadow: 0 0 40px rgba(188, 57, 250, 0.25), 0 15px 50px rgba(0, 0, 0, 0.9), inset 0 0 20px rgba(188, 57, 250, 0.15); }"
"h1 { font-size: 2rem; font-weight: 700; margin: 0 0 0.5rem 0; background: linear-gradient(135deg, #f472b6, #c084fc, #6366f1); -webkit-background-clip: text; -webkit-text-fill-color: transparent; letter-spacing: -0.03em; text-shadow: 0 0 20px rgba(188, 57, 250, 0.3); }"
".subtitle { color: var(--text-secondary); font-size: 0.9rem; margin-bottom: 2rem; font-weight: 400; text-transform: uppercase; letter-spacing: 0.08em; }"
".upload-box { border: 2px dashed rgba(188, 57, 250, 0.3); border-radius: 16px; padding: 2rem 1rem; margin-bottom: 1.5rem; cursor: pointer; transition: all 0.3s ease; background: rgba(139, 92, 246, 0.01); display: flex; flex-direction: column; align-items: center; }"
".upload-box:hover { border-color: var(--accent-neon); background: rgba(188, 57, 250, 0.04); box-shadow: 0 0 20px rgba(188, 57, 250, 0.2); }"
".upload-box svg { width: 40px; height: 40px; fill: none; stroke: #a78bfa; stroke-width: 1.5; margin-bottom: 0.8rem; transition: all 0.3s ease; }"
".upload-box:hover svg { stroke: var(--accent-neon); transform: translateY(-3px); }"
"#uploadText { font-size: 0.85rem; color: #9ca3af; word-break: break-all; }"
"textarea { width: 100%; height: 150px; background: rgba(6, 6, 12, 0.6); border: 1.5px solid rgba(139, 92, 246, 0.2); color: #fff; border-radius: 12px; padding: 12px; font-family: monospace; font-size: 0.8rem; resize: none; outline: none; transition: border-color 0.3s; }"
"textarea:focus { border-color: var(--accent-neon); box-shadow: 0 0 10px rgba(188, 57, 250, 0.2); }"
".btn { background: linear-gradient(135deg, #c084fc, #8b5cf6, #4f46e5); color: white; border: none; padding: 0.9rem 1.5rem; border-radius: 12px; font-weight: 600; font-size: 0.9rem; cursor: pointer; width: 100%; transition: all 0.25s ease; box-shadow: 0 0 15px rgba(139, 92, 246, 0.4); margin-top: 1.5rem; border: 1px solid rgba(255, 255, 255, 0.1); }"
".btn:hover { transform: translateY(-2px); box-shadow: 0 0 25px rgba(188, 57, 250, 0.6); }"
".btn:active { transform: translateY(1px); }"
"#status { margin-top: 1.2rem; font-size: 0.9rem; font-weight: 600; color: var(--accent-neon); min-height: 20px; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"  <h1>HackerHMI IR Uploader</h1>"
"  <div class='subtitle'>Tải tệp tin .ir lên RP2350</div>"
"  <div class='upload-box' id='dropZone'>"
"    <svg viewBox='0 0 24 24'><path stroke-linecap='round' stroke-linejoin='round' stroke='#a78bfa' stroke-width='1.5' d='M12 16.5V9.75m0 0l3 3m-3-3l-3 3M6.75 19.5a4.5 4.5 0 01-1.41-8.775 5.25 5.25 0 0110.233-2.33 3 3 0 013.758 3.848A3.752 3.752 0 0118 19.5H6.75z'/></svg>"
"    <div id='uploadText'>Kéo thả file .ir vào đây hoặc Click để chọn file</div>"
"    <input type='file' id='fileInput' style='display:none' accept='.ir'>"
"  </div>"
"  <div style='text-align: left; margin: 1rem 0;'>"
"    <span style='color: var(--text-secondary); font-size: 0.8rem; font-weight: 600; display: block; margin-bottom: 0.5rem;'>HOẶC DÁN NỘI DUNG TẠI ĐÂY</span>"
"    <textarea id='pasteArea' placeholder='Filetype: IR signals file\nVersion: 1\n...\nname: Power\ntype: parsed\nprotocol: NEC\naddress: 07 00 00 00\ncommand: 02 00 00 00'></textarea>"
"  </div>"
"  <button class='btn' onclick='submitIR()'>UPLOAD LÊN THIẾT BỊ</button>"
"  <div id='status'></div>"
"</div>"
"<script>"
"  const dropZone = document.getElementById('dropZone');"
"  const fileInput = document.getElementById('fileInput');"
"  const pasteArea = document.getElementById('pasteArea');"
"  const statusDiv = document.getElementById('status');"
"  let fileContent = '';"
"  dropZone.onclick = () => fileInput.click();"
"  fileInput.onchange = (e) => {"
"    const file = e.target.files[0];"
"    if (file) {"
"      document.getElementById('uploadText').innerText = file.name;"
"      const reader = new FileReader();"
"      reader.onload = (evt) => {"
"        fileContent = evt.target.result;"
"      };"
"      reader.readAsText(file);"
"    }"
"  };"
"  function submitIR() {"
"    let content = fileContent || pasteArea.value;"
"    if (!content.trim()) {"
"      statusDiv.innerText = 'Vui lòng chọn file hoặc dán nội dung!';"
"      statusDiv.style.color = '#ef4444';"
"      return;"
"    }"
"    statusDiv.innerText = 'Đang tải lên...';"
"    statusDiv.style.color = '#bc39fa';"
"    fetch('/upload', {"
"      method: 'POST',"
"      headers: { 'Content-Type': 'text/plain' },"
"      body: content"
"    })"
"    .then(res => res.text())"
"    .then(text => {"
"      statusDiv.innerText = text;"
"      if(text.includes('Thành công')) {"
"        statusDiv.style.color = '#10b981';"
"      } else {"
"        statusDiv.style.color = '#ef4444';"
"      }"
"    })"
"    .catch(err => {"
"      statusDiv.innerText = 'Lỗi kết nối!';"
"      statusDiv.style.color = '#ef4444';"
"    });"
"  }"
"</script>"
"</body>"
"</html>";

// GET / - Phục vụ trang HTML
static esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, ir_html_page, HTTPD_RESP_USE_STRLEN);
}

// Hàm phân tích 4-byte hex cách nhau khoảng trắng sang uint32_t (ví dụ: "07 00 00 00" -> 7)
static uint32_t parse_hex_bytes(const char* hex_str) {
    uint32_t b0 = 0, b1 = 0, b2 = 0, b3 = 0;
    sscanf(hex_str, "%x %x %x %x", &b0, &b1, &b2, &b3);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

// POST /upload - Tiếp nhận nội dung tệp .ir và parse trực tiếp gửi về RP2350
static esp_err_t upload_handler(httpd_req_t *req) {
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to receive post body");
        return ESP_FAIL;
    }
    buf[req->content_len] = '\0';

    ESP_LOGI(TAG, "Đã nhận được tệp .ir chứa %d ký tự.", req->content_len);

    // Bắt đầu phân tích cú pháp tệp tin .ir
    char name[32] = "IR_Key";
    char type[16] = "";
    char protocol[32] = "";
    uint32_t address = 0;
    uint32_t command = 0;
    uint32_t frequency = 38000;
    char raw_data[1024] = "";

    char *line = strtok(buf, "\n");
    while (line != NULL) {
        // Loại bỏ ký tự xuống dòng carriage return
        char *cr = strchr(line, '\r');
        if (cr) *cr = '\0';

        if (strncmp(line, "name:", 5) == 0) {
            strncpy(name, line + 5, sizeof(name) - 1);
            // Xóa dấu cách thừa ở đầu
            char *p = name;
            while(*p == ' ') p++;
            memmove(name, p, strlen(p) + 1);
        }
        else if (strncmp(line, "type:", 5) == 0) {
            sscanf(line + 5, "%s", type);
        }
        else if (strncmp(line, "protocol:", 9) == 0) {
            sscanf(line + 9, "%s", protocol);
        }
        else if (strncmp(line, "address:", 8) == 0) {
            address = parse_hex_bytes(line + 8);
        }
        else if (strncmp(line, "command:", 8) == 0) {
            command = parse_hex_bytes(line + 8);
        }
        else if (strncmp(line, "frequency:", 10) == 0) {
            sscanf(line + 10, "%lu", &frequency);
        }
        else if (strncmp(line, "data:", 5) == 0) {
            strncpy(raw_data, line + 5, sizeof(raw_data) - 1);
            char *p = raw_data;
            while(*p == ' ') p++;
            memmove(raw_data, p, strlen(p) + 1);
        }
        line = strtok(NULL, "\n");
    }
    free(buf);

    // Truyền dữ liệu về RP2350 qua SPI
    if (strcmp(type, "parsed") == 0) {
        char spi_msg[128];
        snprintf(spi_msg, sizeof(spi_msg), "IR_FILE_PARSED:%s:%s:%lu:%lu", name, protocol, address, command);
        send_to_rp2350(spi_msg);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (strcmp(type, "raw") == 0) {
        // Báo hiệu bắt đầu gói RAW
        char spi_msg[128];
        snprintf(spi_msg, sizeof(spi_msg), "IR_FILE_RAW_START:%s:%lu", name, frequency);
        send_to_rp2350(spi_msg);
        vTaskDelay(pdMS_TO_TICKS(50));

        // Tách nhỏ chuỗi data gửi làm nhiều gói 10 phần tử
        char *token = strtok(raw_data, " ");
        char packet_buf[128] = "IR_FILE_RAW_DATA:";
        int count = 0;
        while (token != NULL) {
            strcat(packet_buf, token);
            count++;
            token = strtok(NULL, " ");
            if (token != NULL && count < 10) {
                strcat(packet_buf, ",");
            } else {
                send_to_rp2350(packet_buf);
                vTaskDelay(pdMS_TO_TICKS(50));
                strcpy(packet_buf, "IR_FILE_RAW_DATA:");
                count = 0;
            }
        }
        
        send_to_rp2350("IR_FILE_RAW_END");
        vTaskDelay(pdMS_TO_TICKS(50));
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File khong hop le");
        return ESP_FAIL;
    }

    // Báo thành công
    send_to_rp2350("IR_UPLOAD_OK");
    httpd_resp_send(req, "Upload Thanh cong! Thiet bi dang phat tin hieu...", HTTPD_RESP_USE_STRLEN);

    // Trì hoãn 1.5 giây để hoàn thành truyền dữ liệu và gửi thông báo, sau đó tự tắt server để tiết kiệm tài nguyên
    vTaskDelay(pdMS_TO_TICKS(1500));
    stop_ir_webserver();

    return ESP_OK;
}

// Khởi chạy máy chủ web xử lý tải tệp tin .ir
void start_ir_webserver(void) {
    if (ir_server != NULL) {
        ESP_LOGI(TAG, "IR File Web Server dang chay.");
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32769; // Tránh xung đột port ctrl với OTA server (32768)

    ESP_LOGI(TAG, "Dang mo IR File Web Server tren Port: %d", config.server_port);

    if (httpd_start(&ir_server, &config) == ESP_OK) {
        httpd_uri_t get_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = get_handler,
            .user_ctx  = NULL
        };
        httpd_uri_t upload_uri = {
            .uri       = "/upload",
            .method    = HTTP_POST,
            .handler   = upload_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(ir_server, &get_uri);
        httpd_register_uri_handler(ir_server, &upload_uri);
    }
}

// Dừng máy chủ web để giải phóng tài nguyên
void stop_ir_webserver(void) {
    if (ir_server != NULL) {
        ESP_LOGI(TAG, "Dang tat IR File Web Server...");
        httpd_stop(ir_server);
        ir_server = NULL;
    }
}
