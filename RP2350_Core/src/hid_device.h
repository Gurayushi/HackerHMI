#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

// Các mã chuột cơ bản
#define MOUSE_BUTTON_LEFT   1
#define MOUSE_BUTTON_RIGHT  2
#define MOUSE_BUTTON_MIDDLE 4

// Hàm gửi 1 phím nhấn (Bàn phím)
// Sử dụng thư viện TinyUSB: tud_hid_keyboard_report(...)
void hid_send_key(char ascii_char) {
    // TODO: Chuyển đổi ký tự ASCII sang chuẩn USB HID Keycode
    // Ví dụ: 'a' -> HID_KEY_A
    uint8_t keycode[6] = {0};
    // Giả lập ánh xạ mã phím...
    
    // tud_hid_keyboard_report(0, 0, keycode);
    // Bỏ nhấn phím:
    // tud_hid_keyboard_report(0, 0, NULL);
}

// Hàm gửi tọa độ di chuyển chuột (Touchpad)
// Sử dụng thư viện TinyUSB: tud_hid_mouse_report(...)
void hid_move_mouse(int8_t delta_x, int8_t delta_y) {
    // delta_x, delta_y nằm trong khoảng -127 đến 127
    // tud_hid_mouse_report(0, 0, delta_x, delta_y, 0, 0);
}

// Hàm gửi thao tác nhấn chuột
void hid_click_mouse(uint8_t button_mask) {
    // Nhấn xuống
    // tud_hid_mouse_report(0, button_mask, 0, 0, 0, 0);
    // Nhả ra
    // tud_hid_mouse_report(0, 0, 0, 0, 0, 0);
}

#endif // HID_DEVICE_H
