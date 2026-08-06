---
name: DWIN DGUS HMI Integration
description: Instructions and tools for interacting with DWIN DGUS II displays, sending VP commands, switching pages, writing ASCII logs, and querying memory maps for HackerHMI.
---

# DWIN DGUS HMI Skill

This skill allows agents to control and query the DWIN DGUS II display in the HackerHMI workspace.

## Available MCP Tools (Registered via `.agents/mcp.json`)

- `dwin_write_vp(address_hex, data_hex)`: Write 16-bit DGUS VP memory address (e.g. `0x0084`, `0x0160`).
- `dwin_write_text(address_hex, text)`: Send ASCII string to log terminal (`0x0098` or `0x0400`).
- `dwin_switch_page(page_id)`: Change display active page (writes to `0x0084`).
- `dwin_read_vp(address_hex, length_words)`: Read DGUS memory bytes (Command `0x83`).
- `dgus_memory_map_lookup(query)`: Lookup VP addresses and touch command ASCII codes in `dwin_memory_map.txt`.
- `dgus_parse_config(file_path)`: Inspect `T5L_CONFIG.BIN` properties.

## Hardware Baud Rate

The display communicates over UART2 (6-pin 2.54mm header) at **921,600 bps**.
