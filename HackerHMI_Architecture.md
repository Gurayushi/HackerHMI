# HackerHMI: Tài liệu Kiến trúc Hệ thống & Tính năng Chi tiết

Tài liệu này đóng vai trò là cẩm nang thiết kế kỹ thuật, luồng hoạt động, cấu trúc bộ nhớ và sự phối hợp giữa hai vi điều khiển (RP2350 & ESP32-C5) để bạn dễ dàng quản lý, tra cứu và bảo trì mã nguồn dự án HackerHMI.

---

## I. TỔNG QUAN PHÂN CHIA NHIỆM VỤ PHẦN CỨNG (AMP ARCHITECTURE)

Hệ thống hoạt động theo mô hình **Đa xử lý không đối xứng (Asymmetric Multi-Processing)**:
*   **Raspberry Pi RP2350 (Main MCU):** 
    *   Quản lý toàn bộ giao diện đồ họa HMI DWIN qua cổng UART0 tốc độ `921600 baud`.
    *   Trực tiếp vận hành các module phần cứng thời gian thực: Giả lập phím ảo BadUSB, thu/phát sóng Sub-1GHz (CC1101), phát hồng ngoại IR, đọc thẻ RFID/NFC.
*   **ESP32-C5 (Network Co-Processor - N16R8):**
    *   Tận dụng kết nối không dây (Wi-Fi 5GHz) để chạy động cơ tấn công Wi-Fi Deauther (bản vá).
    *   Vận hành trình quản lý đa nhiệm đa phiên (Task Manager) cho SSH Terminal và Resource Monitor Dashboard nhờ vùng nhớ **8MB PSRAM**.
    *   Làm mạch nạp SWD không dây cứu hộ hoặc cập nhật OTA cho RP2350.

---

## II. DANH SÁCH CHI TIẾT TỪNG TÍNH NĂNG

### TÍNH NĂNG 1: Wi-Fi Deauther (Hỗ trợ 5GHz)
*   **Chức năng con:** Quét AP/Client, chọn mục tiêu tấn công, phát gói tin Deauth/Disassociation giả mạo, spam Beacon ảo, chế độ Nuke phá sóng.
*   **Luồng hoạt động:** 
    1. Khi người dùng thao tác bấm nút tấn công hoặc quét trên màn hình, DWIN HMI truyền dữ liệu chuỗi (`CMD_DEAUTH_START`, `CMD_DEAUTH_NUKE`, `DEAUTH_SEL:`) qua UART -> RP2350.
    2. RP2350 đóng gói chuỗi này đẩy tiếp qua đường truyền SPI -> ESP32-C5.
    3. ESP32-C5 tiếp nhận lệnh, dùng thư viện Wi-Fi đã vá (`libnet80211.a`) để bypass kiểm duyệt của Espressif, trực tiếp phát các khung dữ liệu thô (raw frames) ra môi trường.
    4. ESP32-C5 thu thập log trạng thái hoặc danh sách AP quét được, đẩy ngược chuỗi `"DEAUTH_LOG:<nội dung>"` qua SPI về RP2350 hiển thị lên HMI.
*   **Cấp phát bộ nhớ:**
    *   *RP2350:* Không lưu trữ trạng thái AP/Client quét được, giải phóng tối đa RAM.
    *   *ESP32-C5:* Dành bộ nhớ đệm tĩnh trên SRAM/PSRAM chứa danh sách tối đa 50 APs và 100 Clients.
*   **Cấu trúc chương trình:**
    *   *ESP32-C5:* Thư mục `src/deauther/` (`wifi_ctrl.c`, `attack.c`, `frames.c`, `sniffer.c`, `targets.c`, `cli.c`, `io.c`).
    *   *RP2350:* `src/main.c` (khối xử lý điều hướng lệnh UART và hiển thị log SPI).

---

### TÍNH NĂNG 2: BadUSB & Stream Deck Mode (Keystroke Injection & Control Pad)
*   **Chức năng con:** 
    *   **BadUSB:** Giả lập bàn phím/chuột USB HID chuẩn, tự động gõ payload tấn công tự động (Ducky scripts) và thiết lập hệ điều hành tương thích (Windows, MacOS, Linux, Android).
    *   **Stream Deck Mode:** Tận dụng bàn phím ảo DWIN HMI làm bàn phím Macro Pad/Control Pad để kích hoạt nhanh các tổ hợp phím tắt (Multi-action macros: `CMD_MACRO_1` đến `CMD_MACRO_4`) và các lệnh điều hướng hệ thống (Điều chỉnh âm lượng qua `VOL_VAL:`, độ sáng màn hình qua `BRIGHT_VAL:` bằng bàn phím đa phương tiện Consumer Control).
*   **Luồng hoạt động:**
    1. Khi người dùng thao tác nhấn nút trên giao diện Stream Deck của HMI, lệnh UART truyền về RP2350.
    2. RP2350 phân tách lệnh, sử dụng driver USB HID (chạy qua TinyUSB trên RP2350) đóng gói các gói tin báo cáo phím (Keyboard/Consumer reports) tương ứng và gửi thẳng qua cổng USB-C vật lý sang máy tính điều khiển.
*   **Cấp phát bộ nhớ:** Chạy hoàn toàn trên bộ nhớ RAM của RP2350 (khoảng 3KB cho cấu trúc mapping phím tắt macro). ESP32-C5 không tham gia vào luồng truyền USB HID này để giữ an toàn tối đa cho CPU phụ.
*   **Cơ chế đồ họa hiển thị (Icon & GIF hoạt họa):**
    *   **Xử lý nội bộ trên DWIN HMI (Offloading):** Để đạt phản hồi hình ảnh 60FPS không trễ, toàn bộ tài nguyên Icon (định dạng ảnh tĩnh) và GIF hoạt họa (các frame chuyển động) của từng phím tắt được thiết kế trực tiếp trong phần mềm DGUS của DWIN dưới dạng file **ICL (Icon Library)**. 
    *   **Trạng thái phím Pressed/Released:** Các thành phần phím bấm trên HMI được cấu hình hiệu ứng "Touch Effect" tự động hoán đổi ID Icon giữa hai trạng thái *Nhấn xuống (Pressed)* và *Nhả ra (Released)*, hoặc kích hoạt hoạt ảnh GIF chạy cục bộ mà không cần RP2350 can thiệp.
    *   **Thay đổi Icon động theo Profile:** RP2350 có khả năng điều khiển thay đổi Icon/GIF hiển thị của các nút từ xa thông qua việc gửi lệnh ghi trực tiếp ID Icon (`dwin_write_text` hoặc ghi giá trị số nguyên) xuống địa chỉ VP điều khiển của phím bấm đó (Ví dụ: ghi giá trị profile ứng dụng đang hoạt động tại cổng `0x0310` để tự động hoán đổi giao diện bộ icon tương ứng).
*   **Cấu trúc chương trình:** 
    *   *RP2350:* `src/usb/hid_device.h` và `src/usb/badusb.h`. Tích hợp phân giải phím tắt trong hàm xử lý chính ở `src/main.c`.
 
---

### TÍNH NĂNG 3: RFID / NFC Reader & Cloner
*   **Chức năng con:** Đọc mã thẻ thang máy tần số thấp 125kHz (RDM6300 UART) và thẻ thông minh NFC tần số cao 13.56MHz (PN532 I2C).
*   **Luồng hoạt động:**
    1. Khi cờ hoạt động `g_rfid_active = true`, RP2350 liên tục kiểm tra UART1 (RDM6300) và giao tiếp I2C (PN532).
    2. Khi phát hiện thẻ quét qua, RP2350 đọc UID, kích hoạt bíp còi buzzer và in thông tin UID trực tiếp lên màn hình HMI.
*   **Cấp phát bộ nhớ:** Lưu chuỗi tạm thời `char rfid_uid[32]` trên RAM RP2350.
*   **Cấu trúc chương trình:**
    *   *RP2350:* `src/rfid/rfid_nfc.h`.

---

### TÍNH NĂNG 4: Sub-1GHz Radio (CC1101 Transceiver)
*   **Chức năng con:** Thu phát, ghi nhận và phát lại các tín hiệu vô tuyến (mã giả lập cửa cuốn, chuông cửa) tần số 315MHz/433MHz.
*   **Luồng hoạt động:** 
    1. Người dùng nhấn nút "Mở cổng" (`CMD_RF_OPEN`) trên HMI.
    2. RP2350 giao tiếp qua SPI1 điều khiển chip vô tuyến CC1101 truyền tải chuỗi byte dữ liệu ra không trung.
*   **Cấu trúc chương trình:**
    *   *RP2350:* `src/radio/radio_cc1101.h` (SPI1: Pins 12, 13, 14, 15).

---

### TÍNH NĂNG 5: Infrared Remote Simulator (IR Blaster)
*   **Chức năng con:** Ghi và phát tín hiệu điều khiển hồng ngoại của các thiết bị gia dụng (Tivi, điều hòa).
*   **Luồng hoạt động:** 
    1. RP2350 nhận lệnh bắn xung `CMD_IR_FIRE` qua UART HMI.
    2. RP2350 sử dụng module PIO0 (Programmable I/O) độc quyền để tạo sóng mang tần số chính xác 38kHz phát qua đèn LED IR (chân GPIO 22).
*   **Cấu trúc chương trình:**
    *   *RP2350:* `src/ir/ir_blaster.h`.

---

### TÍNH NĂNG 6: Mạch nạp Cứu hộ ESP32-C5 UART Bridge
*   **Chức năng con:** Đưa ESP32-C5 vào chế độ Bootloader nạp code từ xa và làm cầu nối UART tốc độ cao nạp trực tiếp qua cổng USB RP2350.
*   **Luồng hoạt động:**
    1. HMI bấm nút "Flash Mode" (`CMD_FLASH_MODE`).
    2. RP2350 kéo IO9 của C5 xuống LOW và nhấp EN của C5 xuống LOW để buộc C5 vào chế độ ROM Bootloader.
    3. RP2350 chuyển sang vòng lặp vô hạn chuyển tiếp dữ liệu thô trực tiếp giữa cổng USB CDC của PC sang cổng UART1 (tốc độ nạp 921600 baud) của ESP32-C5.
*   **Cấu trúc chương trình:**
    *   *RP2350:* `src/flasher/esp32_flasher.h`.

---

### TÍNH NĂNG 7: Cập nhật Không dây Dual OTA
*   **Chức năng con:** Nạp chương trình không dây qua Web Server cho ESP32-C5 và RP2350 (qua giao tiếp SWD).
*   **Luồng hoạt động:**
    1. Người dùng tải file `.bin` lên trang web OTA của ESP32-C5.
    2. Nếu cập nhật cho RP2350, ESP32-C5 kéo chân reset của RP2350 và dùng module SWD giả lập nạp trực tiếp file nhị phân qua cổng nạp debug SWD (SWCLK, SWDIO, RST) của RP2350.
*   **Cấu trúc chương trình:**
    *   *ESP32-C5:* `src/ota/ota_server.h` và `src/flasher/rp2350_swd_flasher.h`.

---

### TÍNH NĂNG 8: Đa nhiệm Đa phiên SSH & Monitor (Nâng cao)
*   **Chức năng con:** Mở song song nhiều phiên SSH Terminal, theo dõi tài nguyên của nhiều máy chủ cùng lúc, ghi dữ liệu biểu đồ 60 giây, tự động đóng task cũ khi tràn RAM (FIFO).
*   **Luồng hoạt động:**
    1. Người dùng mở trang quản lý task của tính năng để xem danh sách hoặc chọn mở phiên làm việc mới.
    2. Khi ẩn (Hide): Tiến trình SSH/Monitor của task vẫn chạy ngầm trên ESP32-C5.
    3. Đối với Monitor: C5 định kỳ 1 giây/lần lấy CPU/RAM/Disk ghi vào mảng lịch sử 60 phần tử.
    4. Khi khôi phục (Resume): ESP32-C5 chia nhỏ lịch sử 60 điểm dữ liệu này đẩy qua SPI về RP2350 vẽ lại biểu đồ tức thì lên màn hình.
*   **Cấp phát bộ nhớ:**
    *   **RAM SSH tối đa:** **1MB** trên PSRAM của ESP32-C5. Nếu vượt quá, phiên SSH cũ nhất (ở đầu hàng đợi liên kết đôi) sẽ bị `Kill` giải phóng tài nguyên.
    *   **RAM Monitor tối đa:** **256KB** trên PSRAM.
    *   *RP2350 RAM:* Đã giải phóng bộ đệm tĩnh 30KB. Chỉ lưu trữ cấu hình nhỏ và chuyển tiếp dữ liệu hiển thị.
*   **Cấu trúc chương trình:**
    *   *ESP32-C5:* `src/deauther/task_manager.h`, `src/deauther/task_manager.c`, tích hợp trong `src/main.c`.
    *   *RP2350:* `src/main.c` (Giao tiếp SPI, cập nhật chỉ số đồ họa DWIN `0x0480`, `0x0482`, `0x0484`).

---

## III. BẢN ĐỒ CHI TIẾT ĐỊA CHỈ BIẾN & LỆNH HMI DWIN (VARIABLE & COMMAND MAP)

Màn hình DWIN HMI tương tác với RP2350 thông qua các địa chỉ biến vùng nhớ (VP Address) và các mã lệnh chuỗi ASCII gửi qua UART. Dưới đây là bảng đặc tả đầy đủ:

### 1. Bản đồ Địa chỉ Biến HMI (VP Memory Map)

| Địa chỉ VP (Hex) | Kiểu dữ liệu | Hướng truyền | Vai trò & Tính năng tương ứng |
| :---: | :---: | :---: | :--- |
| **`0x0084`** | Register (16-bit) | Read/Write | **System Page ID:** Màn hình tự cập nhật khi chuyển trang. RP2350 đọc định kỳ để biết người dùng đang xem chức năng nào, hoặc ghi đè để chủ động đổi trang (`dwin_switch_page()`). |
| **`0x0098`** | Text (ASCII) | Write-Only | **System Console Log:** Vùng hiển thị log hệ thống ở trang chủ. In ra các thông tin khởi động, trạng thái kết nối Wi-Fi, đổi chế độ BadUSB, hay UID thẻ từ vừa quét. |
| **`0x0400`** | Text (ASCII) | Write-Only | **Detail Terminal Area:** Vùng hiển thị log đa năng. Dùng để in danh sách AP quét được của Deauther, hiển thị Terminal SSH chạy ngầm, danh sách Task đang chạy, hay lịch sử 60s Monitor. |
| **`0x0470`** | Text (ASCII) | Write-Only | **Deauther Status:** Hiển thị nhãn trạng thái của bộ phát Deauther (`"Scanning..."`, `"RUNNING - Targeted Mode"`, `"RUNNING - Nuke Mode"`, `"STOPPED"`). |
| **`0x0480`** | Text (ASCII) | Write-Only | **CPU / Speed Stats:** Nhãn chỉ số CPU thời gian thực (`"CPU: xx%"`) của Monitor Task đang active, hoặc tốc độ truyền gói thô (`"pkts=xx"`) của Deauther. |
| **`0x0482`** | Text (ASCII) | Write-Only | **RAM Stats:** Nhãn chỉ số RAM thời gian thực (`"RAM: xx%"`) của Monitor Task đang xem. |
| **`0x0484`** | Text (ASCII) | Write-Only | **Disk Stats:** Nhãn chỉ số dung lượng ổ cứng (`"Disk: xx%"`) của Monitor Task đang xem. |
| **`0x0490`** | Text (ASCII) | Write-Only | **Deauther Mode String:** Nhãn thể hiện phân loại chế độ hoạt động hiện thời của Wi-Fi. |
| **`0x5000`** | Text (ASCII) | Read-Only | **Keyboard Command Input:** Vùng đệm bàn phím ảo của DWIN. Mỗi khi người dùng gõ lệnh hoặc nhập thông số cấu hình và nhấn Enter, DWIN sẽ ghi vào địa chỉ này và gửi sự kiện ngắt báo cho RP2350 đọc. |

---

### 2. Danh mục Mã Lệnh ASCII từ HMI DWIN (Command Codes)

Khi người dùng nhấn các nút bấm cảm ứng trên màn hình DWIN, HMI sẽ bắn các chuỗi lệnh tương ứng về RP2350 qua UART để xử lý:

#### A. Nhóm Lệnh Wi-Fi Deauther
*   `CMD_DEAUTH_SCAN`: Yêu cầu ESP32-C5 càn quét các trạm phát sóng Wi-Fi xung quanh.
*   `CMD_DEAUTH_START`: Bắt đầu tấn công ngắt kết nối AP mục tiêu đã chọn.
*   `CMD_DEAUTH_STOP`: Dừng cuộc tấn công Wi-Fi Deauther.
*   `CMD_DEAUTH_NUKE`: Tấn công dồn dập, spam ngắt mạng diện rộng trong 30 giây.
*   `CMD_DEAUTH_HIDE`: Chuyển màn hình về Trang chủ nhưng tiếp tục giữ cuộc tấn công Wi-Fi chạy ẩn.
*   `CMD_DEAUTH_KILL`: Tắt hoàn toàn Wi-Fi Deauther và giải phóng kênh truyền Wi-Fi.
*   `DEAUTH_SEL:<id>`: Chọn ID của AP trong danh sách quét để nhắm mục tiêu.

#### B. Nhóm Lệnh Giả lập BadUSB
*   `CMD_OS_WIN` / `CMD_OS_MAC` / `CMD_OS_LINUX` / `CMD_OS_ANDROID`: Chuyển đổi Driver bàn phím giả lập của RP2350 tương ứng với hệ điều hành của máy tính đích.
*   `CMD_MACRO_1` / `CMD_MACRO_2` / `CMD_MACRO_3` / `CMD_MACRO_4`: Kích hoạt kịch bản gõ phím macro tự động đã lưu (như Work Mode, Dev Mode, Gaming Mode).
*   `CMD_BADUSB_HIDE`: Quay lại Trang chủ, giữ kết nối USB HID chạy ngầm.
*   `CMD_BADUSB_KILL`: Tắt hoàn toàn tính năng giả lập BadUSB.

#### C. Nhóm Lệnh RF & Hồng Ngoại (CC1101 & IR)
*   `CMD_RF_OPEN`: Gửi tín hiệu điều chế RF Sub-1GHz (315/433MHz) thông qua module CC1101.
*   `CMD_RADIO_HIDE` / `CMD_RADIO_KILL`: Ẩn hoặc tắt bộ thu phát CC1101.
*   `CMD_IR_FIRE`: Phát chuỗi mã hồng ngoại 12-bit điều khiển thiết bị gia dụng.
*   `CMD_IR_HIDE` / `CMD_IR_KILL`: Ẩn hoặc dừng hoàn toàn bộ thu phát IR.

#### D. Nhóm Lệnh Mạch Nạp & Đa nhiệm Nâng cao (SSH / Monitor)
*   `CMD_FLASH_MODE`: Đưa ESP32-C5 vào bootloader và kích hoạt cổng cầu nối UART.
*   `CMD_FLASH_HIDE` / `CMD_FLASH_KILL`: Ẩn/tắt chế độ cầu nối cứu hộ.
*   `CMD_SSH_LIST` / `CMD_MONITOR_LIST`: Yêu cầu ESP32-C5 gửi danh sách các task SSH/Monitor đang hoạt động ngầm.
*   `CMD_SSH_START:<host> <user> <pass> <port>`: Khởi tạo một phiên SSH Terminal mới.
*   `CMD_MONITOR_START:<host> <user> <pass> <port>`: Khởi tạo một Dashboard giám sát tài nguyên mới.
*   `CMD_SSH_HIDE:<id>` / `CMD_MONITOR_HIDE:<id>`: Thu nhỏ task SSH/Monitor có ID tương ứng xuống nền.
*   `CMD_SSH_RESUME:<id>` / `CMD_MONITOR_RESUME:<id>`: Phục hồi và mở lại giao diện hiển thị của task.
*   `CMD_SSH_KILL:<id>` / `CMD_MONITOR_KILL:<id>`: Giải phóng tài nguyên và ngắt socket kết nối của task có ID tương ứng.

#### E. Nhóm Lệnh Cấu hình hệ thống (Gửi trực tiếp vào NVS Storage của C5)
*   `wifi_ssid=<value>`: Cài đặt SSID Wi-Fi để ESP32-C5 kết nối internet.
*   `wifi_pass=<value>`: Cài đặt mật khẩu Wi-Fi.
*   `ssh_ip=<value>`: Thiết lập địa chỉ IP SSH mặc định.
*   `ssh_user=<value>`: Thiết lập tài khoản SSH mặc định.
*   `ssh_pass=<value>`: Thiết lập mật khẩu SSH mặc định.
*   `start_ota=1`: Khởi động Web Server OTA cập nhật phần mềm không dây trên ESP32-C5.
*   `start_ota=0`: Tắt Web Server OTA.
