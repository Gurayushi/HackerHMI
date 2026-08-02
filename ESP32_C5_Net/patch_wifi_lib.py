import os
import shutil
from os.path import join, isfile

Import("env")

def patch_wifi_library():
    print("=================== WiFi Deauther Library Patch Script ===================")
    
    # Lay duong dan cua framework-espidf tu PlatformIO
    platform = env.PioPlatform()
    framework_dir = platform.get_package_dir("framework-espidf")
    if not framework_dir:
        print("[WARNING] Khong tim thay thu muc framework-espidf! Co the PlatformIO chua cai dat hoac chua tai xong packages.")
        return

    # Duong dan file libnet80211.a cua ESP32-C5 trong ESP-IDF
    target_lib_dir = join(framework_dir, "components", "esp_wifi", "lib", "esp32c5")
    target_lib_file = join(target_lib_dir, "libnet80211.a")
    backup_lib_file = join(target_lib_dir, "libnet80211.a.orig")

    # Duong dan file da va trong project cua chung ta
    patched_lib_file = join(env.get("PROJECT_DIR"), "lib", "libnet80211.a")

    if not isfile(patched_lib_file):
        print(f"[ERROR] Khong tim thay file thu vien da va tai: {patched_lib_file}")
        return

    if not os.path.exists(target_lib_dir):
        print(f"[WARNING] Thu muc dich khong ton tai: {target_lib_dir}")
        return

    # Tien hanh backup va de file
    if isfile(target_lib_file):
        if not isfile(backup_lib_file):
            print(f"[Patch] Dang tao file backup goc tai: {backup_lib_file}")
            shutil.copyfile(target_lib_file, backup_lib_file)
        
        print(f"[Patch] Dang copy file libnet80211.a da va de vao: {target_lib_file}")
        shutil.copyfile(patched_lib_file, target_lib_file)
        print("[Patch] Da va thu vien Wi-Fi 5GHz thanh cong!")
    else:
        print(f"[WARNING] Khong tim thay file thu vien goc tai: {target_lib_file}")

# Chay patch
patch_wifi_library()
