#ifndef HID_DEVICE_H
#define HID_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Các biến trạng thái lưu Tọa độ cũ
static int16_t prev_x[4] = {-1, -1, -1, -1};
static int16_t prev_y[4] = {-1, -1, -1, -1};
static float prev_distance = -1.0;

// Hàm gửi 1 phím nhấn / Tổ hợp phím (Bàn phím)
void hid_send_key(uint8_t modifier, uint8_t keycode) {
    // tud_hid_keyboard_report(0, modifier, &keycode);
    // Nhả phím: tud_hid_keyboard_report(0, 0, NULL);
}

// 1 NGÓN TAY: Di chuyển chuột
void hid_move_mouse_1_finger(int16_t curr_x, int16_t curr_y) {
    if (prev_x[0] != -1 && prev_y[0] != -1) {
        int8_t delta_x = (int8_t)(curr_x - prev_x[0]);
        int8_t delta_y = (int8_t)(curr_y - prev_y[0]);
        // tud_hid_mouse_report(0, 0, delta_x, delta_y, 0, 0);
    }
    prev_x[0] = curr_x; prev_y[0] = curr_y;
}

// 2 NGÓN TAY: Cuộn trang (Truyền thống) và Zoom
void hid_gesture_2_fingers(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    if (prev_x[0] != -1 && prev_x[1] != -1) {
        // Tính khoảng cách giữa 2 ngón tay (Pytago)
        float current_distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
        
        // Tính độ dời Y trung bình (Để cuộn trang)
        int8_t delta_y = (int8_t)(((y1 - prev_y[0]) + (y2 - prev_y[1])) / 2);
        
        // Phân tích hành vi: Nếu khoảng cách dãn ra/thu vào đáng kể -> PINCH TO ZOOM
        if (prev_distance != -1.0 && fabs(current_distance - prev_distance) > 10.0) { // Ngưỡng 10 pixel
            if (current_distance > prev_distance) {
                // Zoom In (Giả lập MacOS: Cmd + Plus hoặc Windows: Ctrl + Plus)
                // hid_send_key(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_KEYPAD_ADD);
            } else {
                // Zoom Out
                // hid_send_key(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_KEYPAD_SUBTRACT);
            }
        } 
        // Nếu không thay đổi khoảng cách nhiều -> SCROLLING (Truyền thống, không đảo ngược)
        else if (fabs(delta_y) > 2) { 
            // tud_hid_mouse_report(0, 0, 0, 0, delta_y, 0); 
        }
        
        prev_distance = current_distance;
    } else {
        prev_distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
    }
    
    prev_x[0] = x1; prev_y[0] = y1;
    prev_x[1] = x2; prev_y[1] = y2;
}

// 3 NGÓN TAY: MacOS Mission Control hoặc App Switcher
void hid_gesture_3_fingers(int16_t y1, int16_t y2, int16_t y3) {
    if (prev_y[0] != -1) {
        int8_t delta_y = (int8_t)(((y1 - prev_y[0]) + (y2 - prev_y[1]) + (y3 - prev_y[2])) / 3);
        if (delta_y < -20) {
            // Vuốt 3 ngón lên -> MacOS Mission Control (Ctrl + Mũi tên Lên)
            // hid_send_key(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_ARROW_UP);
        } else if (delta_y > 20) {
            // Vuốt 3 ngón xuống -> App Expose (Ctrl + Mũi tên Xuống)
            // hid_send_key(KEYBOARD_MODIFIER_LEFTCTRL, HID_KEY_ARROW_DOWN);
        }
    }
    prev_y[0] = y1; prev_y[1] = y2; prev_y[2] = y3;
}

// TAP (Chạm nhả): Click chuột
void hid_handle_tap(uint8_t finger_count) {
    if (finger_count == 1) {
        // Chuột trái
        // tud_hid_mouse_report(0, MOUSE_BUTTON_LEFT, 0, 0, 0, 0);
        // tud_hid_mouse_report(0, 0, 0, 0, 0, 0);
    } else if (finger_count == 2) {
        // Chuột phải (Chuẩn MacOS)
        // tud_hid_mouse_report(0, MOUSE_BUTTON_RIGHT, 0, 0, 0, 0);
        // tud_hid_mouse_report(0, 0, 0, 0, 0, 0);
    }
}

// Reset trạng thái
void hid_touchpad_release() {
    for(int i=0; i<4; i++) { prev_x[i] = -1; prev_y[i] = -1; }
    prev_distance = -1.0;
}

#endif // HID_DEVICE_H
