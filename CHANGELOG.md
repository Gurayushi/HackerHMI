# CHANGELOG - HackerHMI Project

All notable changes to the HackerHMI project are documented in this file.

## [2.0.0] - 2026-08-04 - 100% Flipper Zero Protocol Parity & Core Refactoring

### Added
- **iButton Multi-Protocol Support (`src/ibutton/`):**
  - Maxim 1-Wire DS1990A Read & Emulation (GPIO 28 with CRC8 verification).
  - Cyfral (DC2000) & Metakom (TM2002) current-modulation reading and emulation.
  - Automatic multi-protocol pin scanning on GPIO 28.
- **NFC Crypto1 Cipher Engine (`src/nfc_crypto/`):**
  - Full Crypto1 48-bit LFSR encryption/decryption algorithm implementation.
  - PRNG successor calculation for Mifare Classic 1K/4K sector key recovery.
- **Sub-GHz Advanced Algorithms (`src/radio/`):**
  - KeeLoq NLFSR 528-round Decryptor (Serial, Counter, and Button Status extraction).
  - De Bruijn $B(2,12)$ 12-bit Brute-Force Generator (fast scanning of 4,096 fixed codes).
  - Somfy RTS 433.42MHz Manchester & Security+ 1.0/2.0 decoders with automatic frequency shifting.
- **Extended 125kHz / 134.2kHz RFID Protocols (`src/rfid/`):**
  - Indala 64-bit PSK phase-demodulation reading and emulation.
  - AWID 26-bit / 50-bit protocol decoder.
  - FDX-B (ISO 11784/11785) 134.2kHz BPSK animal ID tag reader.
- **IR Learning Receiver & Universal Database (`src/ir/`):**
  - VS1838B IR learning receiver capturing microsecond pulse durations on GPIO 23.
  - Universal AC (Daikin/Panasonic/LG/Midea) & Projector power toggle database.
- **BadUSB DuckyScript 2.0 Engine & Multi-touch Touchpad (`src/usb/`):**
  - DuckyScript 2.0 parser (`DELAY`, `STRING`, `VAR`, `IF/ELSE`, `REPEAT`, `GUI`, `ALT`, `CTRL`).
  - 1-finger, 2-finger scroll, and 3-finger gesture touchpad emulation.

### Refactored
- **Codebase Modularization:**
  - Split monolith `main.c` into clean sub-modules: `comm/c5_spi`, `multicore/core1_task`, `ui/hmi_cmd_parser`, `ibutton/`, `nfc_crypto/`, `radio/`, `rfid/`, `ir/`.
  - Converted header-only function implementations to dedicated `.c` translation units to eliminate linker duplicate symbol errors.
  - Reduced `main.c` to <180 lines focusing on 300MHz system init, Core 1 dispatch, and UART/SPI loop.

### Documentation
- Updated `HackerHMI_Architecture.md` with complete 100% protocol coverage spec and physical pinout map.
- Updated `HackerHMI_UI_Design.md` with new HMI DWIN touch command code mapping.
