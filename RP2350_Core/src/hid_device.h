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

// Biến trạng thái lưu Tọa độ cũ để tính toán Delta
static int16_t prev_x1 = -1, prev_y1 = -1;
static int16_t prev_x2 = -1, prev_y2 = -1;

// Hàm gửi tọa độ di chuyển chuột (1 ngón - Di chuyển trỏ chuột)
void hid_move_mouse_1_finger(int16_t curr_x, int16_t curr_y) {
    if (prev_x1 != -1 && prev_y1 != -1) {
        int8_t delta_x = (int8_t)(curr_x - prev_x1);
        int8_t delta_y = (int8_t)(curr_y - prev_y1);
        
        // tud_hid_mouse_report(0, 0, delta_x, delta_y, 0, 0);
    }
    prev_x1 = curr_x;
    prev_y1 = curr_y;
}

// Hàm gửi thao tác cuộn trang (2 ngón - Scrolling giống MacOS)
void hid_scroll_2_fingers(int16_t curr_x1, int16_t curr_y1, int16_t curr_x2, int16_t curr_y2) {
    if (prev_y1 != -1 && prev_y2 != -1) {
        // Tính trung bình cộng độ dời Y của cả 2 ngón tay
        int8_t delta_y = (int8_t)(((curr_y1 - prev_y1) + (curr_y2 - prev_y2)) / 2);
        
        // Đảo ngược dấu để giống "Natural Scrolling" của Mac (Vuốt lên thì trang kéo xuống)
        int8_t scroll_val = -delta_y; 
        
        // Gửi qua USB Report thông số Wheel (Cuộn dọc)
        // tud_hid_mouse_report(0, 0, 0, 0, scroll_val, 0);
    }
    prev_y1 = curr_y1;
    prev_y2 = curr_y2;
}

// Reset trạng thái khi nhấc tay khỏi màn hình
void hid_touchpad_release() {
    prev_x1 = -1; prev_y1 = -1;
    prev_x2 = -1; prev_y2 = -1;
}

// Hàm gửi thao tác nhấn chuột
void hid_click_mouse(uint8_t button_mask) {
    // Nhấn xuống
    // tud_hid_mouse_report(0, button_mask, 0, 0, 0, 0);
    // Nhả ra
    // tud_hid_mouse_report(0, 0, 0, 0, 0, 0);
}

#endif // HID_DEVICE_H
