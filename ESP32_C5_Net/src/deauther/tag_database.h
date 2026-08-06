#ifndef TAG_DATABASE_H
#define TAG_DATABASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_TAG_NAME_LEN 16
#define MAX_RFID_UID_LEN 8
#define MAX_NFC_UID_LEN  10
#define MAX_TAG_COUNT    20

typedef struct {
    char name[MAX_TAG_NAME_LEN];
    uint8_t uid[MAX_RFID_UID_LEN];
    uint8_t uid_len;
    uint8_t protocol; // 0 = EM4100, 1 = FDX-B
} rfid_profile_t;

typedef struct {
    char name[MAX_TAG_NAME_LEN];
    uint8_t uid[MAX_NFC_UID_LEN];
    uint8_t uid_len;
    uint8_t type;     // 0 = Mifare Classic, 1 = Mifare Ultralight
} nfc_profile_t;

typedef struct {
    rfid_profile_t rfid_tags[MAX_TAG_COUNT];
    uint32_t rfid_count;
    nfc_profile_t nfc_tags[MAX_TAG_COUNT];
    uint32_t nfc_count;
} tag_db_t;

void tag_db_init(void);
bool tag_db_add_rfid(const char* name, const uint8_t* uid, uint8_t uid_len, uint8_t protocol);
bool tag_db_add_nfc(const char* name, const uint8_t* uid, uint8_t uid_len, uint8_t type);
bool tag_db_get_rfid(uint32_t idx, rfid_profile_t* out_profile);
bool tag_db_get_nfc(uint32_t idx, nfc_profile_t* out_profile);
bool tag_db_delete_rfid(const char* name);
bool tag_db_delete_nfc(const char* name);
void tag_db_list(char* out_buf, size_t max_len);

#endif // TAG_DATABASE_H
