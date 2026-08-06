#include "tag_database.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "TAG_DB";
static tag_db_t g_tag_db = {0};

void tag_db_init(void) {
    memset(&g_tag_db, 0, sizeof(tag_db_t));
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("tag_db", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        size_t required_size = sizeof(tag_db_t);
        err = nvs_get_blob(my_handle, "tags", &g_tag_db, &required_size);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Loi doc tag blob NVS (%s), khoi tao rong", esp_err_to_name(err));
            g_tag_db.rfid_count = 0;
            g_tag_db.nfc_count = 0;
        } else {
            ESP_LOGI(TAG, "Da doc %lu RFID tags va %lu NFC tags tu NVS", g_tag_db.rfid_count, g_tag_db.nfc_count);
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGI(TAG, "Chua co CSDL tags luu tren NVS. Tao moi.");
    }
}

static void tag_db_save(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("tag_db", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(my_handle, "tags", &g_tag_db, sizeof(tag_db_t));
        if (err == ESP_OK) {
            nvs_commit(my_handle);
            ESP_LOGI(TAG, "Da luu CSDL tags vao NVS");
        } else {
            ESP_LOGE(TAG, "Loi ghi tag blob NVS (%s)", esp_err_to_name(err));
        }
        nvs_close(my_handle);
    }
}

bool tag_db_add_rfid(const char* name, const uint8_t* uid, uint8_t uid_len, uint8_t protocol) {
    if (!name || strlen(name) == 0 || !uid || uid_len > MAX_RFID_UID_LEN) return false;
    
    // Check duplication
    for (uint32_t i = 0; i < g_tag_db.rfid_count; i++) {
        if (strcmp(g_tag_db.rfid_tags[i].name, name) == 0) {
            memcpy(g_tag_db.rfid_tags[i].uid, uid, uid_len);
            g_tag_db.rfid_tags[i].uid_len = uid_len;
            g_tag_db.rfid_tags[i].protocol = protocol;
            tag_db_save();
            return true;
        }
    }
    
    if (g_tag_db.rfid_count < MAX_TAG_COUNT) {
        rfid_profile_t *p = &g_tag_db.rfid_tags[g_tag_db.rfid_count++];
        strncpy(p->name, name, MAX_TAG_NAME_LEN - 1);
        p->name[MAX_TAG_NAME_LEN - 1] = '\0';
        memcpy(p->uid, uid, uid_len);
        p->uid_len = uid_len;
        p->protocol = protocol;
        tag_db_save();
        return true;
    }
    
    return false; // Full
}

bool tag_db_add_nfc(const char* name, const uint8_t* uid, uint8_t uid_len, uint8_t type) {
    if (!name || strlen(name) == 0 || !uid || uid_len > MAX_NFC_UID_LEN) return false;
    
    // Check duplication
    for (uint32_t i = 0; i < g_tag_db.nfc_count; i++) {
        if (strcmp(g_tag_db.nfc_tags[i].name, name) == 0) {
            memcpy(g_tag_db.nfc_tags[i].uid, uid, uid_len);
            g_tag_db.nfc_tags[i].uid_len = uid_len;
            g_tag_db.nfc_tags[i].type = type;
            tag_db_save();
            return true;
        }
    }
    
    if (g_tag_db.nfc_count < MAX_TAG_COUNT) {
        nfc_profile_t *p = &g_tag_db.nfc_tags[g_tag_db.nfc_count++];
        strncpy(p->name, name, MAX_TAG_NAME_LEN - 1);
        p->name[MAX_TAG_NAME_LEN - 1] = '\0';
        memcpy(p->uid, uid, uid_len);
        p->uid_len = uid_len;
        p->type = type;
        tag_db_save();
        return true;
    }
    
    return false; // Full
}

bool tag_db_get_rfid(uint32_t idx, rfid_profile_t* out_profile) {
    if (idx >= g_tag_db.rfid_count || !out_profile) return false;
    *out_profile = g_tag_db.rfid_tags[idx];
    return true;
}

bool tag_db_get_nfc(uint32_t idx, nfc_profile_t* out_profile) {
    if (idx >= g_tag_db.nfc_count || !out_profile) return false;
    *out_profile = g_tag_db.nfc_tags[idx];
    return true;
}

bool tag_db_delete_rfid(const char* name) {
    if (!name) return false;
    for (uint32_t i = 0; i < g_tag_db.rfid_count; i++) {
        if (strcmp(g_tag_db.rfid_tags[i].name, name) == 0) {
            for (uint32_t j = i; j < g_tag_db.rfid_count - 1; j++) {
                g_tag_db.rfid_tags[j] = g_tag_db.rfid_tags[j + 1];
            }
            g_tag_db.rfid_count--;
            tag_db_save();
            return true;
        }
    }
    return false;
}

bool tag_db_delete_nfc(const char* name) {
    if (!name) return false;
    for (uint32_t i = 0; i < g_tag_db.nfc_count; i++) {
        if (strcmp(g_tag_db.nfc_tags[i].name, name) == 0) {
            for (uint32_t j = i; j < g_tag_db.nfc_count - 1; j++) {
                g_tag_db.nfc_tags[j] = g_tag_db.nfc_tags[j + 1];
            }
            g_tag_db.nfc_count--;
            tag_db_save();
            return true;
        }
    }
    return false;
}

void tag_db_list(char* out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return;
    out_buf[0] = '\0';
    size_t offset = 0;
    
    offset += snprintf(out_buf + offset, max_len - offset, "--- RFID TAGS ---\n");
    for (uint32_t i = 0; i < g_tag_db.rfid_count; i++) {
        rfid_profile_t *p = &g_tag_db.rfid_tags[i];
        offset += snprintf(out_buf + offset, max_len - offset, "%ld. %s [", i + 1, p->name);
        for (uint8_t j = 0; j < p->uid_len; j++) {
            offset += snprintf(out_buf + offset, max_len - offset, "%02X", p->uid[j]);
        }
        offset += snprintf(out_buf + offset, max_len - offset, "] (%s)\n", p->protocol == 0 ? "EM4100" : "FDX-B");
    }
    
    offset += snprintf(out_buf + offset, max_len - offset, "\n--- NFC TAGS ---\n");
    for (uint32_t i = 0; i < g_tag_db.nfc_count; i++) {
        nfc_profile_t *p = &g_tag_db.nfc_tags[i];
        offset += snprintf(out_buf + offset, max_len - offset, "%ld. %s [", i + 1, p->name);
        for (uint8_t j = 0; j < p->uid_len; j++) {
            offset += snprintf(out_buf + offset, max_len - offset, "%02X", p->uid[j]);
        }
        offset += snprintf(out_buf + offset, max_len - offset, "] (%s)\n", p->type == 0 ? "Classic" : "Ultralight");
    }
}
