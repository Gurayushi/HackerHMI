#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

// Thư viện SSH
#include "libssh2.h"
#include "driver/spi_slave.h"
#include "driver/gpio.h"
#include "flasher/rp2350_swd_flasher.h"
#include "ota/ota_server.h"
#include "ota/ir_web_server.h"
#include "deauther/wifi_database.h"
#include "deauther/wifi_auto_connect.h"
#include "deauther/task_manager.h"
#include "deauther/tag_database.h"
#include "weather/weather_client.h"

// Cấu hình các chân SPI2
#define GPIO_MOSI 19
#define GPIO_MISO 16
#define GPIO_SCLK 18
#define GPIO_CS   17
#define GPIO_HANDSHAKE 22

// Cấu hình Deauther Core
#include "deauther/io.h"
#include "deauther/wifi_ctrl.h"
#include "deauther/targets.h"
#include "deauther/attack.h"
#include "deauther/cli.h"

#define SPI_QUEUE_SIZE 32
#define SPI_MSG_LEN 128

static const char *TAG = "HACKER_NET";
static QueueHandle_t s_spi_tx_queue = NULL;

// Biến kiểm soát khởi động Wi-Fi một lần duy nhất cho SSH/OTA
static bool wifi_started = false;

// Biến trạng thái Wi-Fi
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

EventGroupHandle_t get_wifi_event_group(void) {
    return wifi_event_group;
}

// Các biến cấu hình động (Sẽ được nạp từ bộ nhớ NVS)
char wifi_ssid[32] = "";
char wifi_pass[64] = "";
char target_ip[32] = "";
char target_user[32] = "";
char target_pass[64] = "";

// Hàm gửi tin nhắn qua SPI về RP2350
void send_to_rp2350(const char *msg) {
    if (s_spi_tx_queue) {
        char buf[SPI_MSG_LEN] = {0};
        strncpy(buf, msg, SPI_MSG_LEN - 1);
        xQueueSend(s_spi_tx_queue, buf, 0); // Đẩy vào hàng đợi không chặn
    }
}

void forward_to_rp2350(const char* payload) {
    send_to_rp2350(payload);
}

// Các ký hiệu giả lập để tránh lỗi liên kết thư viện Wi-Fi cũ với IDF mới
uint32_t g_offchan_packet_lifetime = 1000;
int wifi_nvs_get_low_rate_enable(void) {
    return 0;
}
bool esp_wifi_use_supp_pmk_cache(void) {
    return false;
}

// Sink xuất log ra SPI gửi về RP2350
static void spi_log_sink(const char *buf, size_t len, void *ctx) {
    (void)ctx;
    char msg[SPI_MSG_LEN];
    size_t offset = 0;
    while (offset < len) {
        memset(msg, 0, sizeof(msg));
        strcpy(msg, "LOG:");
        size_t chunk = len - offset;
        if (chunk > (SPI_MSG_LEN - 5)) chunk = SPI_MSG_LEN - 5;
        memcpy(msg + 4, buf + offset, chunk);
        send_to_rp2350(msg);
        offset += chunk;
    }
}

// Xử lý sự kiện Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG, "Wi-Fi mat ket noi, ly do: %d. Thu ket noi lai...", event->reason);
        
        if (event->reason == WIFI_REASON_AUTH_FAIL || event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
            char wrong_pass_msg[64];
            snprintf(wrong_pass_msg, sizeof(wrong_pass_msg), "WIFI_ERR_WRONG_PASS:%s", wifi_ssid);
            send_to_rp2350(wrong_pass_msg);
        } else {
            send_to_rp2350("WIFI_DISCONNECTED");
        }
        
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Đã lấy được IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        
        // Cap nhat vao CSDL LFU va tang luot ket noi thanh cong
        wifi_config_t wifi_cfg;
        if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK) {
            wifi_db_add_or_update((const char*)wifi_cfg.sta.ssid, (const char*)wifi_cfg.sta.password);
            wifi_db_increment_conn((const char*)wifi_cfg.sta.ssid);
        }
        
        char ip_msg[64];
        snprintf(ip_msg, sizeof(ip_msg), "IR_WEB_URL:http://" IPSTR "/", IP2STR(&event->ip_info.ip));
        send_to_rp2350(ip_msg);
        
        char conn_msg[64];
        snprintf(conn_msg, sizeof(conn_msg), "WIFI_CONNECTED:%s", wifi_ssid);
        send_to_rp2350(conn_msg);
    }
}

// Khởi tạo Wi-Fi Station cho OTA/SSH
void wifi_init_sta(void) {
    if (wifi_started) {
        ESP_LOGI(TAG, "Wi-Fi đã được khởi chạy từ trước.");
        return;
    }
    wifi_started = true;
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
    
    strcpy((char*)wifi_config.sta.ssid, wifi_ssid);
    strcpy((char*)wifi_config.sta.password, wifi_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi khởi tạo hoàn tất.");
}

// Luồng thực thi SSH
void ssh_task(void *pvParameters) {
    ESP_LOGI(TAG, "Đang khởi tạo phiên SSH...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Đang kết nối SSH tới %s@%s", target_user, target_ip);
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

// Hàm nạp flash cho RP2350 qua SWD
void ota_update_rp2350(const uint8_t* bin_data, size_t size) {
    ESP_LOGI(TAG, "Bắt đầu cập nhật OTA cho chip RP2350...");
    rp2350_halt();
    rp2350_flash_write(bin_data, size);
    rp2350_reboot();
}

// Task giao tiếp SPI Slave với RP2350
void spi_slave_task(void *pvParameters) {
    ESP_LOGI(TAG, "Khởi động SPI Slave Task...");

    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 3,
        .flags = 0,
    };

    ESP_ERROR_CHECK(spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO));

    // Cấu hình chân Handshake
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << GPIO_HANDSHAKE),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_HANDSHAKE, 1); // Mặc định HIGH

    WORD_ALIGNED_ATTR char recvbuf[SPI_MSG_LEN];
    WORD_ALIGNED_ATTR char sendbuf[SPI_MSG_LEN];

    spi_slave_transaction_t t = {
        .length = SPI_MSG_LEN * 8,
        .rx_buffer = recvbuf,
        .tx_buffer = sendbuf,
    };

    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        memset(sendbuf, 0, sizeof(sendbuf));

        // Kiểm tra xem có tin nhắn gửi đi trong queue không
        bool has_msg = false;
        if (xQueueReceive(s_spi_tx_queue, sendbuf, 0) == pdTRUE) {
            has_msg = true;
            gpio_set_level(GPIO_HANDSHAKE, 0); // Kéo xuống LOW báo tin nhắn
        } else {
            strcpy(sendbuf, "IDLE");
        }

        // Chờ nhận / gửi qua SPI
        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
        
        if (has_msg) {
            gpio_set_level(GPIO_HANDSHAKE, 1); // Trả về HIGH
        }

        if (ret == ESP_OK) {
            if (strlen(recvbuf) > 0 && strcmp(recvbuf, "IDLE") != 0) {
                // --- BỘ XỬ LÝ LỆNH ĐA NHIỆM TỪ RP2350 HMI ---
                if (strncmp(recvbuf, "CMD_", 4) == 0) {
                    if (strncmp(recvbuf, "CMD_GET_TASKS ", 14) == 0) {
                        int type = atoi(recvbuf + 14);
                        char list_buf[128];
                        task_get_list(type, list_buf, sizeof(list_buf));
                        send_to_rp2350(list_buf);
                    }
                    else if (strncmp(recvbuf, "CMD_START_TASK ", 15) == 0) {
                        int type;
                        char host[32], user[32], pass[32];
                        int port = 22;
                        if (sscanf(recvbuf + 15, "%d %s %s %s %d", &type, host, user, pass, &port) >= 4) {
                            task_instance_t* task = task_create(type, host, user, pass, port);
                            if (task) {
                                char reply[32];
                                snprintf(reply, sizeof(reply), "TASK_STARTED:%lu", task->task_id);
                                send_to_rp2350(reply);
                            } else {
                                send_to_rp2350("TASK_START_ERR");
                            }
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_HIDE_TASK ", 14) == 0) {
                        uint32_t task_id = strtoul(recvbuf + 14, NULL, 10);
                        if (task_hide(task_id)) {
                            char reply[32];
                            snprintf(reply, sizeof(reply), "TASK_HIDDEN:%lu", task_id);
                            send_to_rp2350(reply);
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_RESUME_TASK ", 16) == 0) {
                        uint32_t task_id = strtoul(recvbuf + 16, NULL, 10);
                        task_instance_t* task = task_resume(task_id);
                        if (task) {
                            char reply[32];
                            snprintf(reply, sizeof(reply), "TASK_RESUMED:%lu", task_id);
                            send_to_rp2350(reply);
                            
                            if (task->type == TASK_TYPE_MONITOR) {
                                send_monitor_history_to_hmi(task);
                            }
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_KILL_TASK ", 14) == 0) {
                        uint32_t task_id = strtoul(recvbuf + 14, NULL, 10);
                        if (task_kill(task_id)) {
                            char reply[32];
                            snprintf(reply, sizeof(reply), "TASK_KILLED:%lu", task_id);
                            send_to_rp2350(reply);
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_WIFI_FORGET:", 16) == 0) {
                        char forget_ssid[33] = {0};
                        strncpy(forget_ssid, recvbuf + 16, sizeof(forget_ssid) - 1);
                        if (wifi_db_forget(forget_ssid)) {
                            send_to_rp2350("WIFI_FORGOTTEN");
                            if (strcmp(wifi_ssid, forget_ssid) == 0) {
                                esp_wifi_disconnect();
                                memset(wifi_ssid, 0, sizeof(wifi_ssid));
                                memset(wifi_pass, 0, sizeof(wifi_pass));
                            }
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_TAG_ADD_RFID:", 17) == 0) {
                        char name[16] = {0};
                        char uid_hex[32] = {0};
                        int protocol = 0;
                        if (sscanf(recvbuf + 17, "%[^:]:%[^:]:%d", name, uid_hex, &protocol) == 3) {
                            uint8_t uid[8];
                            uint8_t len = 0;
                            for (int i = 0; i < strlen(uid_hex) && len < 8; i += 2) {
                                unsigned int val;
                                sscanf(uid_hex + i, "%2x", &val);
                                uid[len++] = (uint8_t)val;
                            }
                            if (tag_db_add_rfid(name, uid, len, protocol)) {
                                send_to_rp2350("TAG_ADD_OK");
                            } else {
                                send_to_rp2350("TAG_ADD_ERR");
                            }
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_TAG_ADD_NFC:", 16) == 0) {
                        char name[16] = {0};
                        char uid_hex[32] = {0};
                        int type = 0;
                        if (sscanf(recvbuf + 16, "%[^:]:%[^:]:%d", name, uid_hex, &type) == 3) {
                            uint8_t uid[10];
                            uint8_t len = 0;
                            for (int i = 0; i < strlen(uid_hex) && len < 10; i += 2) {
                                unsigned int val;
                                sscanf(uid_hex + i, "%2x", &val);
                                uid[len++] = (uint8_t)val;
                            }
                            if (tag_db_add_nfc(name, uid, len, type)) {
                                send_to_rp2350("TAG_ADD_OK");
                            } else {
                                send_to_rp2350("TAG_ADD_ERR");
                            }
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_TAG_DEL_RFID:", 17) == 0) {
                        if (tag_db_delete_rfid(recvbuf + 17)) {
                            send_to_rp2350("TAG_DEL_OK");
                        } else {
                            send_to_rp2350("TAG_DEL_ERR");
                        }
                    }
                    else if (strncmp(recvbuf, "CMD_TAG_DEL_NFC:", 16) == 0) {
                        if (tag_db_delete_nfc(recvbuf + 16)) {
                            send_to_rp2350("TAG_DEL_OK");
                        } else {
                            send_to_rp2350("TAG_DEL_ERR");
                        }
                    }
                    else if (strcmp(recvbuf, "CMD_TAG_LIST") == 0) {
                        char list_buf[256];
                        tag_db_list(list_buf, sizeof(list_buf));
                        send_to_rp2350(list_buf);
                    }
                }
                // Kiểm tra xem có phải lệnh cấu hình hệ thống không
                else if (strncmp(recvbuf, "wifi_ssid=", 10) == 0 || strncmp(recvbuf, "wifi_pass=", 10) == 0 ||
                    strncmp(recvbuf, "ssh_ip=", 7) == 0 || strncmp(recvbuf, "ssh_user=", 9) == 0 ||
                    strncmp(recvbuf, "ssh_pass=", 9) == 0 || strncmp(recvbuf, "start_ota=", 10) == 0 ||
                    strncmp(recvbuf, "start_ir_web=", 13) == 0) {
                    
                    char temp[SPI_MSG_LEN];
                    strcpy(temp, recvbuf);
                    char *equal_sign = strchr(temp, '=');
                    if (equal_sign != NULL) {
                        *equal_sign = '\0';
                        char *key = temp;
                        char *value = equal_sign + 1;
                        
                        if (strcmp(key, "start_ota") == 0) {
                            if (strcmp(value, "1") == 0) {
                                wifi_init_sta();
                                start_ota_webserver();
                            } else {
                                stop_ota_webserver();
                            }
                        } else if (strcmp(key, "start_ir_web") == 0) {
                            if (strcmp(value, "1") == 0) {
                                esp_netif_ip_info_t ip_info;
                                esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
                                bool connected = false;
                                if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
                                    connected = true;
                                }
                                
                                if (!connected) {
                                    send_to_rp2350("IR_WEB_ERR_WIFI");
                                    if (strlen(wifi_ssid) != 0 && !wifi_started) {
                                        wifi_init_sta();
                                    }
                                } else {
                                    start_ir_webserver();
                                    char ip_msg[64];
                                    snprintf(ip_msg, sizeof(ip_msg), "IR_WEB_URL:http://" IPSTR "/", IP2STR(&ip_info.ip));
                                    send_to_rp2350(ip_msg);
                                }
                            } else {
                                stop_ir_webserver();
                            }
                        } else {
                            if (strcmp(key, "wifi_ssid") == 0) {
                                strncpy(wifi_ssid, value, sizeof(wifi_ssid) - 1);
                                wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
                            }
                            else if (strcmp(key, "wifi_pass") == 0) {
                                strncpy(wifi_pass, value, sizeof(wifi_pass) - 1);
                                wifi_pass[sizeof(wifi_pass) - 1] = '\0';
                                
                                if (wifi_started) {
                                    wifi_config_t wifi_config = {0};
                                    strcpy((char*)wifi_config.sta.ssid, wifi_ssid);
                                    strcpy((char*)wifi_config.sta.password, wifi_pass);
                                    esp_wifi_disconnect();
                                    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
                                    esp_wifi_connect();
                                } else {
                                    wifi_init_sta();
                                }
                            }
                            
                            nvs_handle_t my_handle;
                            if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
                                nvs_set_str(my_handle, key, value);
                                nvs_commit(my_handle);
                                nvs_close(my_handle);
                            }
                        }
                    }
                } else {
                    // Chuyển thẳng lệnh nhận được vào CLI của Deauther
                    cli_feed(recvbuf, strlen(recvbuf));
                    cli_feed("\r\n", 2);
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// Đọc cấu hình từ NVS
void load_config_from_nvs() {
    ESP_LOGI(TAG, "Đang tải cấu hình từ NVS...");
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Lỗi mở NVS (%s)", esp_err_to_name(err));
        return;
    }

    size_t required_size = 0;
    required_size = sizeof(target_ip);
    nvs_get_str(my_handle, "ssh_ip", target_ip, &required_size);
    required_size = sizeof(target_user);
    nvs_get_str(my_handle, "ssh_user", target_user, &required_size);
    required_size = sizeof(target_pass);
    nvs_get_str(my_handle, "ssh_pass", target_pass, &required_size);
    required_size = sizeof(wifi_ssid);
    nvs_get_str(my_handle, "wifi_ssid", wifi_ssid, &required_size);
    required_size = sizeof(wifi_pass);
    nvs_get_str(my_handle, "wifi_pass", wifi_pass, &required_size);

    nvs_close(my_handle);
}

static void monitor_tick_task(void *pvParameters) {
    (void)pvParameters;
    while (1) {
        monitor_update_tick();
        send_monitor_updates_to_hmi();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Bắt đầu khởi động ESP32-C5 HackerHMI");
    
    // Tải cấu hình
    load_config_from_nvs();

    // Khởi tạo CSDL Wi-Fi LFU và khởi chạy quét ngầm kết nối tự động
    wifi_db_init();
    tag_db_init();
    wifi_auto_connect_start();

    // Khởi tạo trình quản lý đa nhiệm
    task_manager_init();

    // Khởi tạo các module của Deauther
    io_init();
    io_register_sink(spi_log_sink, NULL);
    wifi_ctrl_init();
    targets_init();
    attack_init();

    // Khởi tạo các chân GPIO cho giao tiếp SWD
    swd_init_pins();

    // Tạo Queue và khởi động SPI Slave Task
    s_spi_tx_queue = xQueueCreate(SPI_QUEUE_SIZE, SPI_MSG_LEN);
    xTaskCreate(spi_slave_task, "spi_slave_task", 8192, NULL, 12, NULL);
    
    // Khởi chạy tác vụ cập nhật tài nguyên Dashboard định kỳ 1s
    xTaskCreate(monitor_tick_task, "monitor_tick", 4096, NULL, 5, NULL);

    if (strlen(wifi_ssid) != 0) {
        wifi_init_sta();
        weather_init();
        if (strlen(target_ip) != 0) {
            xTaskCreate(&ssh_task, "ssh_task", 8192, NULL, 5, NULL);
        }
    }
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
