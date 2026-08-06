#ifndef TV_BUSTER_DATABASE_H
#define TV_BUSTER_DATABASE_H

#include <stdint.h>

typedef enum {
    IR_PROTO_NEC,
    IR_PROTO_SIRC_12,
    IR_PROTO_RC5
} ir_protocol_t;

typedef struct {
    const char *brand;
    ir_protocol_t protocol;
    uint32_t address;
    uint32_t command;
} tv_code_t;

// Danh sách mã tắt nguồn (Power Off/Toggle) phổ biến nhất của các hãng TV lớn toàn cầu
static const tv_code_t tv_buster_database[] = {
    {"Sony",        IR_PROTO_SIRC_12, 0x01,   0x15},    // Command 0x15, Addr 0x01
    {"Samsung",     IR_PROTO_NEC,     0x0707, 0x02},    // Addr 0x0707, Cmd 0x02 (Samsung32)
    {"LG",          IR_PROTO_NEC,     0x0404, 0x08},    // Addr 0x0404, Cmd 0x08
    {"Panasonic",   IR_PROTO_NEC,     0x0100, 0x3D},    // Addr 0x0100, Cmd 0x3D
    {"Toshiba",     IR_PROTO_NEC,     0x0202, 0x02},    // Addr 0x0202, Cmd 0x02
    {"Sharp",       IR_PROTO_NEC,     0x555A, 0xF1},    // Addr 0x555A, Cmd 0xF1
    {"Sanyo",       IR_PROTO_NEC,     0x00FF, 0x18},    // Addr 0x00FF, Cmd 0x18
    {"Philips",     IR_PROTO_RC5,     0x00,   0x0C},    // Addr 0x00, Cmd 0x0C (Toggle)
    {"JVC",         IR_PROTO_NEC,     0x0303, 0x08},    // Addr 0x0303, Cmd 0x08
    {"Pioneer",     IR_PROTO_NEC,     0xA23D, 0x3C},    // Addr A23D, Cmd 0x3C
    {"Hitachi",     IR_PROTO_NEC,     0x50AF, 0x3C},    // Addr 50AF, Cmd 0x3C
    {"NEC_TV",      IR_PROTO_NEC,     0x1818, 0x18},    // Addr 1818, Cmd 0x18
};

#define TV_BUSTER_DB_SIZE (sizeof(tv_buster_database) / sizeof(tv_code_t))

#endif // TV_BUSTER_DATABASE_H
