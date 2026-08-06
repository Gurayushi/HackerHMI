# HackerHMI: The Ultimate Cyber-Deck & Server Dashboard 🚀

> **HackerHMI** is an advanced, asymmetric hardware console that bridges the gap between a professional multi-server dashboard and a portable penetration testing tool (à la Flipper Zero). 

Powered by the bleeding-edge **Raspberry Pi RP2350** for graphics/PIO manipulation and the **ESP32-C5** for stealthy dual-band networking, this 7-inch DWIN touchscreen device is built for system administrators and hardware hackers alike.

## 🔥 Key Features

*   **Asymmetric Brains:** Dual-MCU architecture. RP2350 handles the UI, ANSI Terminal Emulation, and hardware modules. ESP32-C5 manages Wi-Fi 6 (2.4/5GHz), Tailscale VPN routing, and libssh2 connections.
*   **Zero-Client SSH Terminal:** No custom API software required on your servers. HackerHMI connects directly to your Windows/Linux machines via native SSH over a secure Tailscale VPN tunnel.
*   **Hardware Weaponization:** Built-in support for Flipper Zero-style hardware modules via RP2350's PIO:
     *   📡 **Sub-1GHz (CC1101):** Record and replay RF signals (Garage doors, gates).
     *   💳 **NFC & RFID (PN532 / EM4095):** Read, write, and emulate 13.56MHz (NFC target emulation) and 125kHz (EM4100 & HID Prox FSK via software-defined demodulation/modulation) access cards.
     *   ⌨️ **BadUSB / FIDO2:** Keystroke injection and hardware authentication via TinyUSB.
     *   🌐 **Wi-Fi/BLE Marauder:** Deauth and BLE spam attacks powered by ESP32-C5.
     *   📶 **Smart Wi-Fi Manager:** Phone-like Wi-Fi behavior. Background auto-scan & reconnect, LFU-based 100-network profile database on NVS (with overflow protection), wrong password alarms, and active forget.
*   **Silky Smooth UI:** Features a custom Ring-Buffer algorithm enabling full 50KB scrollback history on the DWIN HMI, rendering ANSI colors at a blazing `921600 baud`.
*   **Dual OTA Updates:** ESP32-C5 updates itself via Wi-Fi and acts as an onboard SWD Programmer to flash the RP2350 wirelessly.
*   **Integrated Wi-Fi Deauther:** Incorporates a tailored, hardware-integrated core based on the [esp32-c5-deauth](https://github.com/maxbrito500/esp32-c5-deauth) repository by **maxbrito500**. The original author's native web panel and mobile app controls have been stripped out. Instead, all low-level attack routines, packet engines, and configuration CLI have been fully mapped directly onto the physical DWIN HMI screen via SPI communication.

## 🛠️ Hardware Stack
*   **Main Core:** Raspberry Pi Pico 2 (RP2350)
*   **Network Co-Processor:** ESP32-C5-DevKitC (N16R8)
*   **Display:** 7.0" DWIN HMI (DMT10600C070-07WTZ5)

---
*Built with C/C++ using Pico SDK and ESP-IDF.*
