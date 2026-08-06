# HackerHMI: Tài liệu Kiến trúc Hệ thống & Tính năng Chi tiết (100% Flipper Zero Parity)

Tài liệu này đóng vai trò là cẩm nang thiết kế kỹ thuật, luồng hoạt động, cấu trúc bộ nhớ và sự phối hợp giữa hai vi điều khiển (RP2350 & ESP32-C5) để bạn dễ dàng quản lý, tra cứu và bảo trì mã nguồn dự án HackerHMI.

---

## I. TỔNG QUAN PHÂN CHIA NHIỆM VỤ PHẦN CỨNG (AMP ARCHITECTURE)

Hệ thống hoạt động theo mô hình **Đa xử lý không đối xứng (Asymmetric Multi-Processing)** kết hợp với kiến trúc chạy đa nhân (Dual-Core RP2350 + ESP32-C5):
*   **Raspberry Pi RP2350 (Main MCU - Chạy 2 nhân 300MHz Overclock):** 
    *   **Core 0 (Nhân quản trị & UI):** Quản lý giao diện đồ họa HMI DWIN qua cổng UART0, bắt sự kiện phím cảm ứng, quản lý phím/chuột ảo BadUSB / Touchpad 3-ngón, và là nhân duy nhất giao tiếp SPI0 với ESP32-C5 để đảm bảo an toàn tuyến.
    *   **Core 1 (Nhân phần cứng thời gian thực & Thuật toán nặng):** Nhận lệnh từ Core 0 qua FIFO phần cứng để thực hiện các thuật toán mật mã và giao thức thời gian thực:
        *   **NFC Crypto1 Cipher Engine & Nested Attack** (Mifare Classic 1K/4K Full Sector).
        *   **Sub-GHz KeeLoq 528-round Decryptor & De Bruijn 12-bit Brute-force Generator** (CC1101).
        *   **Somfy RTS 433.42MHz Manchester & Security+ 1.0/2.0 Decoders**.
        *   **Quét RFID 125kHz / 134.2kHz:** EM4100 (ASK), HID Prox (FSK 26-bit), Indala (PSK), AWID, và FDX-B (chip thú cưng).
        *   **Trình đọc/giả lập iButton 1-Wire:** Tự động quét DS1990A $\rightarrow$ Cyfral (DC2000) $\rightarrow$ Metakom (TM2002) trên GPIO 28.
        *   **Mắt thu & Học lệnh Hồng ngoại (IR RX):** Đo micro-giây chu kỳ xung qua mắt thu VS1838B trên GPIO 23.
    *   *Đồng bộ liên nhân:* Sử dụng cờ báo `g_core1_result_ready` để truyền dữ liệu và khóa Mutex `dwin_uart_mutex` bảo vệ việc in log lên màn hình DWIN.
*   **ESP32-C5 (Network Co-Processor - N16R8):**
    *   Tận dụng kết nối không dây băng tần kép (Wi-Fi 2.4GHz / 5GHz + BLE 5.0) để chạy động cơ tấn công Wi-Fi Deauther.
    *   Vận hành trình quản lý đa nhiệm đa phiên (Task Manager) cho SSH Terminal và Resource Monitor Dashboard nhờ vùng nhớ **8MB PSRAM**.
    *   Quản lý cơ sở dữ liệu NVS lưu trữ thông tin thẻ RFID, NFC, iButton, mã sóng RF và tài khoản Wi-Fi/SSH (100 LFU profiles).

---

## II. DANH SÁCH CHI TIẾT TỪNG TÍNH NĂNG & THUẬT TOÁN

### TÍNH NĂNG 1: Wi-Fi Deauther (Hỗ trợ 5GHz)
*   **Chức năng con:** Quét AP/Client, chọn mục tiêu tấn công, phát gói tin Deauth/Disassociation giả mạo, spam Beacon ảo, chế độ Nuke phá sóng.
*   **Thư viện:** `src/deauther/` trên ESP32-C5 tích hợp bản vá `libnet80211.a`.

### TÍNH NĂNG 2: BadUSB, Multi-touch Touchpad & Stream Deck Mode
*   **BadUSB:** Động cơ **DuckyScript 2.0 Parser** hỗ trợ `DELAY`, `STRING`, `VAR`, `IF/ELSE`, `REPEAT`, `GUI`, `ALT`, `CTRL`.
*   **Multi-touch Touchpad:** Giả lập chuột cảm ứng đa điểm 1-ngón (di chuyển, click), 2-ngón (cuộn trang), 3-ngón (chuyển tab/app) chuẩn USB HID.
*   **Stream Deck Mode:** Phím tắt Macro Pad kết hợp Consumer Control (điều chỉnh độ sáng, âm lượng).

### TÍNH NĂNG 3: RFID 125kHz & 134.2kHz Full Protocol (`src/rfid/`)
*   **EM4100 (ASK):** Đọc, ghi (thẻ T5577), và giả lập qua chân MOSFET `RFID_EMU_PIN`.
*   **HID Prox (FSK 26-bit H10301):** Giải điều chế FSK phần mềm (15.625kHz / 12.5kHz), giả lập qua nhấp nhả MOSFET.
*   **Indala (PSK) & AWID:** Giải điều chế pha PSK 64-bit và mã hóa AWID 26/50-bit.
*   **FDX-B (134.2kHz):** Đọc chuẩn chip sinh học định danh thú cưng ISO 11784/11785 (BPSK).

### TÍNH NĂNG 4: NFC PN532 & Thuật toán Crypto1 Cipher (`src/rfid/` & `src/nfc_crypto/`)
*   **ISO14443-A UID Read/Emulate:** Giao tiếp I2C đọc/giả lập thẻ đích qua `tgInitAsTarget`.
*   **Mifare Classic 1K/4K Full Sector:** Xác thực Key A/B qua `InDataExchange` với danh sách Transport Keys mặc định.
*   **Crypto1 Cipher Engine:** Mã hóa/giải mã LFSR 48-bit (`crypto1.c`), tính toán PRNG successor khôi phục 16 sector keys.
*   **ISO15693 (iCLASS) & Sony FeliCa:** Pass-through khung truyền lệnh 212/424 kbps qua PN532.

### TÍNH NĂNG 5: Sub-GHz CC1101 & Thuật toán KeeLoq / De Bruijn (`src/radio/`)
*   **Fixed Code (433.92MHz):** Giải mã phần mềm Princeton (24-bit) và Came (12-bit).
*   **Frequency Analyzer:** Tự động quét đo cường độ sóng RSSI trên các dải tần 315MHz, 433.92MHz, 868MHz, 915MHz.
*   **Raw Signal Capture & Replay:** Ghi nhận chuỗi xung thô bất kỳ vào RAM (tối đa 1024 xung) và phát lại.
*   **KeeLoq NLFSR 528-round Decryptor:** Giải mã 528 vòng trích xuất Serial Number, Counter, và Button Status.
*   **Somfy RTS (433.42MHz) & Security+ 1.0/2.0:** Chuyển tần số tự động sang 433.42MHz, giải mã Manchester 80-bit Somfy và Tri-State 39/66-bit.
*   **De Bruijn 12-bit Brute-force Generator:** Sinh thuật toán chuỗi De Bruijn $B(2, 12)$ quét 4,096 mã cửa cổng cố định siêu tốc.

### TÍNH NĂNG 6: iButton Multi-Protocol (`src/ibutton/`)
*   **DS1990A 1-Wire:** Đọc/Giả lập mã ROM 64-bit kèm kiểm tra CRC8 trên **GPIO 28**.
*   **Cyfral (DC2000) & Metakom (TM2002):** Đọc/Giả lập dòng điện biến đổi Period Pulse Modulation.
*   **Tự động nhận diện:** Quét tuần tự `DS1990A` $\rightarrow$ `Cyfral` $\rightarrow$ `Metakom`.

### TÍNH NĂNG 7: Hồng ngoại IR Receiver & Universal DB (`src/ir/`)
*   **IR Blaster (PIO 38kHz Carrier):** Chương trình PIO0 `ir_carrier.pio` phát sóng mang chuẩn 38kHz.
*   **TV Buster:** Phát mã tắt nguồn vạn năng cho 12 hãng TV lớn.
*   **IR RX Learning (VS1838B):** Đo thời lượng $us$ các sườn xung trên **GPIO 23** để học lệnh thực tế.
*   **Universal AC & Projector DB:** Mã phát vạn năng cho điều hòa (Daikin, Panasonic, LG, Midea) và máy chiếu (Epson, Sony, BenQ).

---

## III. CẤU TRÚC THƯ MỤC MÃ NGUỒN MÔ-ĐUN HÓA (`RP2350_Core/src/`)

```
RP2350_Core/src/
├── comm/
│   ├── c5_spi.h / c5_spi.c       # Giao tiếp SPI0 với ESP32-C5
├── multicore/
│   ├── core1_task.h / core1_task.c # Quản lý chạy đa nhân Core 1 (300MHz)
├── ui/
│   ├── dwin_ui.h                 # Driver UART0 màn hình DWIN
│   ├── hmi_cmd_parser.h / .c     # Bộ phân tích cú pháp lệnh HMI
├── ibutton/
│   ├── ibutton.h / ibutton.c     # 1-Wire DS1990A driver
│   ├── ibutton_ext.h / .c        # Cyfral & Metakom protocols
├── nfc_crypto/
│   ├── crypto1.h / crypto1.c     # Crypto1 48-bit LFSR cipher
├── radio/
│   ├── radio_cc1101.h / .c       # CC1101 OOK RX/TX & Princeton/Came decoders
│   ├── keeloq.h / keeloq.c       # KeeLoq 528-round NLFSR decryptor
│   ├── debruijn_gen.h / .c       # De Bruijn 12-bit brute-force generator
│   ├── rolling_codes.h / .c      # Somfy RTS (433.42MHz) & Security+ decoders
├── rfid/
│   ├── rfid_nfc.h / rfid_nfc.c   # EM4095 EM4100/HID Prox & PN532 NFC
│   ├── rfid_protocols_ext.h / .c # Indala PSK, AWID, FDX-B (134.2kHz)
├── ir/
│   ├── ir_blaster.h              # PIO IR TX driver
│   ├── ir_rx.h / ir_rx.c         # VS1838B IR learning receiver
│   ├── ir_universal_db.h / .c    # Universal AC & Projector IR DB
├── usb/
│   ├── badusb.h                  # BadUSB DuckyScript 2.0 engine
│   ├── hid_device.h              # Multi-touch Touchpad mouse emulation
├── flasher/
│   ├── esp32_flasher.h           # Mạch nạp cứu hộ C5 UART Bridge
└── main.c                        # Tệp khởi chạy chính (<180 dòng)
```

---

## IV. BẢN ĐỒ BỘ NHỚ VÙNG ĐỆM DWIN DGUS II (VP MEMORY MAP REFERENCE)

Dựa trên tài liệu tham khảo `dwin_memory_map.txt` và `T5L_DGUSII-Application-Development-Guide-V2.921.pdf`:

### 1. Vùng nhớ Văn bản & Log Console (Text Display VP Map)
*   **`0x0098`:** **System Console Log** (Log hệ thống, in kết quả quét UID thẻ RFID, NFC, iButton).
*   **`0x0400`:** **Detail Console Log** (Wi-Fi AP List, SSH Terminal, Task Monitor, UID chi tiết).
*   **`0x0100` - `0x0140`:** Vùng đệm gõ phím ảo cấu hình Wi-Fi SSID, Pass, SSH IP/User/Pass.

### 2. Vùng nhớ Cảm ứng Đa điểm (Multi-Touch Precision Touchpad VP Map)
*   **`0x0210`:** Tọa độ Ngón 1 ($X_1, Y_1$).
*   **`0x0214`:** Tọa độ Ngón 2 ($X_2, Y_2$).
*   **`0x0218`:** Tọa độ Ngón 3 ($X_3, Y_3$).
*   **`0x021C`:** Tọa độ Ngón 4 ($X_4, Y_4$).
*   **`0x0220`:** Cờ Tap (1 = Click Trái, 2 = Click Phải).
*   **`0x0224`:** Lựa chọn OS target (0 = Windows, 1 = MacOS, 2 = Linux, 3 = Android).

### 3. Vùng nhớ Stream Deck & Control Pad VP Map
*   **`0x0300`:** Thanh trượt điều chỉnh Âm lượng hệ thống (0 - 100).
*   **`0x0302`:** Thanh trượt điều chỉnh Độ sáng màn hình (0 - 100).
*   **`0x0310`:** Nhận lệnh tự động đổi giao diện HMI (Smart Profile) từ PC gửi xuống qua USB.

### 4. Vùng nhớ Wi-Fi Deauther 5GHz VP Map
*   **`0x0470`:** Nhãn trạng thái Deauther (`"RUNNING"`, `"STOPPED"`).
*   **`0x0480`:** Số lượng gói tin Deauth đã bắn (`"Sent: 2.4G: xxx, 5G: yyy"`).
*   **`0x0490`:** Nhãn chế độ tấn công đang chọn.

---

## V. SƠ ĐỒ PINOUT ĐẤU NỐI VẬT LÝ HOÀN CHỈNH (PINOUT MAP)

| Tên Module | Chân Chức năng | Chân RP2350 | Ghi chú & Cấu hình |
| :--- | :--- | :---: | :--- |
| **iButton (1-Wire)** | Data / I/O | **GPIO 28** | Trở kéo lên 4.7kΩ lên 3.3V (DS1990A / Cyfral / Metakom). |
| **IR RX (Mắt thu)** | Signal Out | **GPIO 23** | Kết nối mắt thu VS1838B (Active LOW). |
| **IR TX (LED phát)** | Anode | **GPIO 22** | Phát băm xung 38kHz qua PIO0. |
| **EM4095 RFID 125k** | SHD (Shutdown) | **GPIO 20** | LOW = Bật phát sóng, HIGH = Tắt phát sóng. |
| | DEMOD / OUT | **GPIO 26** | Đọc tín hiệu Manchester/FSK/PSK thô. |
| | MOD / EMU | **GPIO 27** | Lái MOSFET ngắt sóng mang / giả lập thẻ. |
| **CC1101 Sub-GHz** | SPI1 MISO/MOSI/SCK/CS | **GPIO 12, 15, 14, 13** | Tuyến SPI1 dành riêng cho CC1101. |
| | GDO0 (RX Interrupt) | **GPIO 10** | Ngắt thu tín hiệu OOK/Manchester. |
| **PN532 NFC 13.56M** | I2C0 SDA / SCL | **GPIO 4, 5** | Tuyến I2C0 kéo trở pull-up 4.7kΩ. |
| **ESP32-C5 SPI** | SPI0 MISO/MOSI/SCK/CS | **GPIO 16, 19, 18, 17** | Tuyến SPI0 bus cao tốc liên vi điều khiển. |
| | Handshake Interrupt | **GPIO 22** | Chân báo ngắt dữ liệu SPI. |
| **DWIN 7" HMI** | UART0 TX / RX | **GPIO 0, 1** | Tốc độ baud **921600 bps** (Native TTL 3.3V qua hàng 6-pin 2.54mm header UART2/PGL2). |

---

## VI. CÔNG CỤ HỖ TRỢ AI DEVELOPMENT: DWIN DGUS MCP SERVER

Hệ thống tích hợp một MCP Server (Model Context Protocol) chuyên dụng bằng Python tại tệp [`tools/dwin_dgus_mcp_server.py`](file:///D:/HackerHMI/tools/dwin_dgus_mcp_server.py) để cho phép AI Agent tương tác trực tiếp với DWIN HMI:

*   **`dwin_write_vp`**: Đóng gói và gửi khung truyền DGUS `5A A5 [LEN] 82 [VP] [DATA]`.
*   **`dwin_write_text`**: Ghi văn bản ASCII lên địa chỉ VP DWIN (`0x0098` log hệ thống, `0x0400` log terminal).
*   **`dwin_switch_page`**: Đổi trang hiển thị HMI bằng cách ghi ID trang xuống VP `0x0084`.
*   **`dwin_read_vp`**: Đọc giá trị biến VP từ DWIN qua khung lệnh `5A A5 [LEN] 83 [VP] [LEN]`.
*   **`dgus_memory_map_lookup`**: Tra cứu nhanh địa chỉ vùng nhớ và mã lệnh từ `dwin_memory_map.txt`.
*   **`dgus_parse_config`**: Phân tích file cấu hình phần cứng `T5L_CONFIG.BIN`.
*   **Cấu hình đăng ký:** Tệp [`tools/mcp_config.json`](file:///D:/HackerHMI/tools/mcp_config.json).

