# DWIN HMI: Hướng dẫn Thiết kế Giao diện & Cấu hình DGUS

Tài liệu này cung cấp sơ đồ thiết kế UI/UX cho màn hình **DWIN 7" (1024x600)** với phong cách Dark-Mode Cyberpunk hiện đại, đồng thời đặc tả chi tiết tọa độ và thuộc tính để cấu hình trong phần mềm **DGUS II**.

---

## 1. Thiết kế Giao diện Trang chủ (Page 0)

Dưới đây là bản thiết kế mockup trực quan cho **Trang chủ Dashboard (Page 0)** với tông màu xám tối và các chi tiết điểm nhấn neon (Cyan & Purple):

![DWIN Main Dashboard Mockup](file:///D:/HackerHMI/DGUS/image/page0_dashboard.png)

### Bố cục Không gian (Layout Grid - 1024x600)
*   **Sidebar Navigation (Trái - Rộng 80px):** Chứa 7 Icon chức năng:
    1.  *Dashboard Home* (Trang chủ)
    2.  *Wi-Fi Deauther* (Tấn công Wi-Fi)
    3.  *BadUSB & Stream Deck* (Macro pad / Multi-touch Touchpad)
    4.  *RFID/NFC/iButton Cloner* (Sao chép thẻ từ & iButton)
    5.  *Radio CC1101 / IR* (Sub-GHz KeeLoq/De Bruijn & IR RX/Universal)
    6.  *SSH Terminal* (Điều khiển console)
    7.  *Task Manager* (Quản lý đa nhiệm)

### Hệ màu sắc Thiết kế Neon Cyberpunk (Design Color Tokens)

| Thành phần | Mã màu RGB Hex | Hệ màu DWIN (RGB565) | Mô tả phong cách hiển thị |
| :--- | :---: | :---: | :--- |
| **Nền chính (Background)** | `#030307` | `0x0000` | Xám đen vũ trụ sâu thẳm. |
| **Nền Card (Container)** | `#06060c` | `0x0021` | Xám mờ đục translucent. |
| **Viền Neon Tĩnh (Border)** | `#8b5cf6` | `0x8AEF` | Neon Violet ánh sáng dịu. |
| **Viền Sáng Active (Glow)** | `#bc39fa` | `0xBC1F` | Neon Magenta đậm nổi bật. |
| **Chữ chính (Primary)** | `#f3f4f6` | `0xFFFF` | Trắng sáng, phản xạ cao. |
| **Chữ phụ (Secondary)** | `#a78bfa` | `0xA47F` | Tím pastel nhẹ. |
| **Neon Blue (Kênh vẽ/Chữ)**| `#3b82f6` | `0x3C1F` | Neon Blue sáng cho đồ thị/IP. |

---

## 2. Đặc tả Cấu hình Widget DGUS cho từng Trang (100% Protocol Parity Release)

### Trang 0: Dashboard Home
*   **System Log Terminal:**
    *   *Widget:* Text Display (ASCII) - VP `0x0098` (Tọa độ: X=120, Y=180, Rộng=800, Cao=380).
*   **Weather Widget:** VP `0x0160` (Nhiệt độ), VP `0x0162` (Độ ẩm), VP `0x0164` (Icon), VP `0x0166` (AQI).

### Trang 2: BadUSB, Touchpad & Stream Deck Mode
*   **Bàn phím ảo 75% Layout:** VP `0x0200` (Nhận ký tự ASCII gõ vào).
*   **Touchpad Cảm ứng Đa điểm (Multi-Touch Precision Touchpad):**
    *   `0x0210`: Tọa độ Ngón 1 ($X_1, Y_1$ - 4 bytes).
    *   `0x0214`: Tọa độ Ngón 2 ($X_2, Y_2$ - 4 bytes).
    *   `0x0218`: Tọa độ Ngón 3 ($X_3, Y_3$ - 4 bytes).
    *   `0x021C`: Tọa độ Ngón 4 ($X_4, Y_4$ - 4 bytes).
    *   `0x0220`: Cờ Tap (1 = Left Click, 2 = Right Click).
    *   `0x0224`: Lựa chọn OS Target (`0` = Win, `1` = Mac, `2` = Linux, `3` = Android).
*   **Stream Deck Controls:**
    *   `0x0300`: Thanh trượt Âm lượng (Volume 0-100).
    *   `0x0302`: Thanh trượt Độ sáng (Brightness 0-100).
    *   `0x0310`: Nhận lệnh tự động hoán đổi giao diện HMI tùy theo phần mềm active trên PC.

### Trang 3: RFID / NFC / iButton Cloner (Mở rộng toàn bộ)
*   **Nút cảm ứng Đọc/Giả lập RFID/iButton mới:**
    *   `CMD_IBUTTON_READ`: Bắt đầu quét iButton 1-Wire (DS1990A).
    *   `CMD_IBUTTON_CYFRAL_READ`: Quét iButton chuẩn Cyfral (DC2000).
    *   `CMD_IBUTTON_METAKOM_READ`: Quét iButton chuẩn Metakom (TM2002).
    *   `CMD_RFID_INDALA_READ`: Quét thẻ Indala PSK 64-bit.
    *   `CMD_RFID_AWID_READ`: Quét thẻ AWID 26/50-bit.
    *   `CMD_RFID_FDXB_READ`: Quét chip thú cưng FDX-B 134.2kHz.

### Trang 4: Sub-GHz CC1101 & IR Controller (Mở rộng toàn bộ)
*   **Nút cảm ứng Vét cạn & Học lệnh IR mới:**
    *   `CMD_RF_BRUTEFORCE`: Kích hoạt quét vét cạn chuỗi De Bruijn 12-bit (4,096 mã cố định).
    *   `CMD_IR_LEARN`: Bắt đầu chế độ thu & học lệnh hồng ngoại (VS1838B).
    *   `CMD_IR_AC_UNIVERSAL`: Bắn chuỗi xung hồng ngoại vạn năng tắt/bật Điều hòa (Daikin/Panasonic).

---

## 3. Danh mục Mã Lệnh ASCII từ HMI DWIN (Command Mapping Update)

```
[Touch Button] --(UART ASCII)--> [RP2350 Core 0] --(FIFO)--> [Core 1 Multicore (300MHz)]
                                        │
                                        └──(SPI0)--> [ESP32-C5 NVS Database]
```

### Bảng tra cứu Lệnh cảm ứng HMI mới:

| Mã lệnh HMI ASCII | Module xử lý | Hành vi hệ thống |
| :--- | :--- | :--- |
| `CMD_IBUTTON_READ` | `src/ibutton/ibutton.c` | Đọc mã ROM 64-bit DS1990A 1-Wire trên GPIO 28. |
| `CMD_IBUTTON_CYFRAL_READ` | `src/ibutton/ibutton_ext.c` | Đọc chìa từ biến đổi dòng điện Cyfral. |
| `CMD_IBUTTON_METAKOM_READ` | `src/ibutton/ibutton_ext.c` | Đọc chìa từ biến đổi dòng điện Metakom. |
| `CMD_RFID_INDALA_READ` | `src/rfid/rfid_protocols_ext.c` | Đọc thẻ điều chế pha Indala 64-bit PSK. |
| `CMD_RFID_AWID_READ` | `src/rfid/rfid_protocols_ext.c` | Đọc thẻ AWID 26/50-bit. |
| `CMD_RFID_FDXB_READ` | `src/rfid/rfid_protocols_ext.c` | Đọc chip sinh học thú cưng ISO 11784/11785 FDX-B (134.2kHz). |
| `CMD_RF_BRUTEFORCE` | `src/radio/debruijn_gen.c` | Phát chuỗi De Bruijn $B(2,12)$ vét cạn 4,096 mã cửa cổng. |
| `CMD_IR_LEARN` | `src/ir/ir_rx.c` | Thu học xung thô hồng ngoại từ VS1838B trên GPIO 23. |
| `CMD_IR_AC_UNIVERSAL` | `src/ir/ir_universal_db.c` | Bắn chuỗi xung hồng ngoại vạn năng tắt/bật Điều hòa. |
