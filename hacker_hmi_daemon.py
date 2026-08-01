import asyncio
import sys
import psutil
from bleak import BleakScanner, BleakClient

# UUID tiêu chuẩn của dịch vụ Nordic UART Service (NUS)
NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_RX_CHAR_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

DEVICE_NAME = "HackerHMI_BLE"

# Hàm lấy tên ứng dụng đang hiển thị ở mặt trước (Active Window Process)
def get_active_window_process():
    try:
        if sys.platform == "win32":
            import win32gui
            import win32process
            hwnd = win32gui.GetForegroundWindow()
            _, pid = win32process.GetWindowThreadProcessId(hwnd)
            process = psutil.Process(pid)
            return process.name().lower()
        elif sys.platform == "darwin":
            from AppKit import NSWorkspace
            active_app = NSWorkspace.sharedWorkspace().frontmostApplication()
            return active_app.localizedName().lower()
        else:
            return "unknown"
    except Exception:
        return "unknown"

# Ánh xạ tên Process sang Chế độ hiển thị (Profile) của HMI
def map_process_to_profile(process_name):
    # Trả về mã lệnh ngắn gọn để gửi qua BLE
    if "chrome" in process_name:
        return "APP:CHROME"
    elif "photoshop" in process_name:
        return "APP:PHOTOSHOP"
    elif "code" in process_name or "visual studio" in process_name:
        return "APP:CODE"
    elif "steam" in process_name or "discord" in process_name:
        return "APP:GAME"
    return "APP:DEFAULT"

async def main():
    print("=== HackerHMI Smart Profile Daemon ===")
    print("Đang quét tìm thiết bị Bluetooth: " + DEVICE_NAME + "...")
    
    # Quét thiết bị BLE
    device = await BleakScanner.find_device_by_name(DEVICE_NAME)
    if not device:
        print(f"[-] Không tìm thấy thiết bị '{DEVICE_NAME}'. Hãy đảm bảo HackerHMI đã bật Bluetooth.")
        return

    print(f"[+] Đã tìm thấy HackerHMI tại địa chỉ: {device.address}")
    print("Đang kết nối...")
    
    async with BleakClient(device) as client:
        print("[+] Kết nối Bluetooth thành công!")
        
        last_profile = ""
        while True:
            process_name = get_active_window_process()
            profile = map_process_to_profile(process_name)
            
            # Chỉ gửi dữ liệu nếu phát hiện chuyển đổi ứng dụng
            if profile != last_profile:
                print(f"[>] Phát hiện chuyển đổi ứng dụng: {process_name} -> Gửi mã: {profile}")
                try:
                    # Gửi chuỗi qua cổng RX của dịch vụ BLE UART
                    await client.write_gatt_char(NUS_RX_CHAR_UUID, profile.encode('utf-8'))
                    last_profile = profile
                except Exception as e:
                    print(f"[-] Lỗi truyền dữ liệu BLE: {e}")
                    break
            
            await asyncio.sleep(1.0) # Quét mỗi giây một lần

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nĐã đóng App giám sát.")
