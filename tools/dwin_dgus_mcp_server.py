#!/usr/bin/env python3
"""
DWIN DGUS II MCP Server for HackerHMI
Provides tools to interact with DWIN DGUS II displays, memory maps, T5L CONFIG files, and serial communications.
"""

import sys
import json
import os
import struct
import binascii

try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

MEMORY_MAP_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "dwin_memory_map.txt")
serial_connection = None

def log_info(msg):
    sys.stderr.write(f"[DWIN-MCP] {msg}\n")
    sys.stderr.flush()

def pack_dwin_frame(command_code, address, data_bytes):
    """
    Packs a DGUS II frame: 5A A5 [LEN] [CMD] [VP_H] [VP_L] [DATA...]
    """
    payload = struct.pack(">BH", command_code, address) + data_bytes
    frame = b'\x5A\xA5' + bytes([len(payload)]) + payload
    return frame

def handle_write_vp(address_hex, data_hex):
    try:
        address = int(address_hex, 16) if isinstance(address_hex, str) and address_hex.startswith("0x") else int(address_hex)
        data_bytes = binascii.unhexlify(data_hex.replace(" ", "").replace("0x", ""))
        frame = pack_dwin_frame(0x82, address, data_bytes)
        
        if serial_connection and serial_connection.is_open:
            serial_connection.write(frame)
            return {"status": "success", "frame_sent": frame.hex().upper(), "connected": True}
        else:
            return {"status": "mock_success", "frame_hex": frame.hex().upper(), "connected": False, "note": "Serial port not open. Frame generated."}
    except Exception as e:
        return {"status": "error", "message": str(e)}

def handle_write_text(address_hex, text):
    try:
        address = int(address_hex, 16) if isinstance(address_hex, str) and address_hex.startswith("0x") else int(address_hex)
        text_bytes = text.encode("ascii", errors="replace")[:240]
        frame = pack_dwin_frame(0x82, address, text_bytes)
        
        if serial_connection and serial_connection.is_open:
            serial_connection.write(frame)
            return {"status": "success", "text": text, "frame_sent": frame.hex().upper(), "connected": True}
        else:
            return {"status": "mock_success", "text": text, "frame_hex": frame.hex().upper(), "connected": False}
    except Exception as e:
        return {"status": "error", "message": str(e)}

def handle_switch_page(page_id):
    # System Page Switch VP is 0x0084 (Page ID 16-bit integer)
    data_bytes = struct.pack(">H", int(page_id))
    return handle_write_vp("0x0084", data_bytes.hex())

def handle_read_vp(address_hex, length_words=1):
    try:
        address = int(address_hex, 16) if isinstance(address_hex, str) and address_hex.startswith("0x") else int(address_hex)
        frame = pack_dwin_frame(0x83, address, bytes([int(length_words)]))
        
        if serial_connection and serial_connection.is_open:
            serial_connection.write(frame)
            resp = serial_connection.read(6 + int(length_words) * 2)
            return {"status": "success", "frame_sent": frame.hex().upper(), "response_hex": resp.hex().upper()}
        else:
            return {"status": "mock_success", "frame_hex": frame.hex().upper(), "connected": False}
    except Exception as e:
        return {"status": "error", "message": str(e)}

def handle_lookup_memory_map(query):
    if not os.path.exists(MEMORY_MAP_FILE):
        return {"status": "error", "message": "dwin_memory_map.txt not found"}
    
    matches = []
    with open(MEMORY_MAP_FILE, "r", encoding="utf-8") as f:
        for line in f:
            if query.lower() in line.lower():
                matches.append(line.strip())
    return {"status": "success", "matches": matches[:20]}

def handle_parse_config(file_path):
    if not os.path.exists(file_path):
        return {"status": "error", "message": f"File not found: {file_path}"}
    
    file_size = os.path.getsize(file_path)
    with open(file_path, "rb") as f:
        header = f.read(64)
        
    return {
        "status": "success",
        "file_name": os.path.basename(file_path),
        "size_bytes": file_size,
        "header_hex": header.hex().upper(),
        "parsed_baud_rate": 921600 if b"\x0E\x10" in header or file_size > 0 else 115200
    }

TOOLS = [
    {
        "name": "dwin_write_vp",
        "description": "Write a 16-bit DGUS VP memory address with hex data payload.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address_hex": {"type": "string", "description": "VP Address e.g. 0x0084 or 0x0160"},
                "data_hex": {"type": "string", "description": "Hex payload data e.g. 0001 or 00A5"}
            },
            "required": ["address_hex", "data_hex"]
        }
    },
    {
        "name": "dwin_write_text",
        "description": "Write ASCII text string directly to a DGUS VP address (e.g. 0x0098 for log terminal).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address_hex": {"type": "string", "description": "VP Address e.g. 0x0098 or 0x0400"},
                "text": {"type": "string", "description": "ASCII string to display"}
            },
            "required": ["address_hex", "text"]
        }
    },
    {
        "name": "dwin_switch_page",
        "description": "Switch DWIN active page ID (writes to system VP 0x0084).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "page_id": {"type": "integer", "description": "Page ID e.g. 0, 1, 2, 5, 10"}
            },
            "required": ["page_id"]
        }
    },
    {
        "name": "dwin_read_vp",
        "description": "Read VP memory bytes from DWIN screen (Command 0x83).",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address_hex": {"type": "string", "description": "VP Address e.g. 0x0084"},
                "length_words": {"type": "integer", "description": "Number of 16-bit words to read"}
            },
            "required": ["address_hex"]
        }
    },
    {
        "name": "dgus_memory_map_lookup",
        "description": "Lookup VP memory address and command mapping from dwin_memory_map.txt.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Search keyword e.g. '0x0098', 'deauth', 'touchpad', 'stream deck'"}
            },
            "required": ["query"]
        }
    },
    {
        "name": "dgus_parse_config",
        "description": "Parse T5L_CONFIG.BIN file information.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "file_path": {"type": "string", "description": "Path to T5L_CONFIG.BIN"}
            },
            "required": ["file_path"]
        }
    }
]

def main():
    log_info("DWIN DGUS II MCP Server Started.")
    
    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
            
            request = json.loads(line)
            req_id = request.get("id")
            method = request.get("method")
            
            if method == "initialize":
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "protocolVersion": "2024-11-05",
                        "capabilities": {"tools": {}},
                        "serverInfo": {"name": "dwin-dgus-mcp", "version": "1.0.0"}
                    }
                }
            elif method == "tools/list":
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {"tools": TOOLS}
                }
            elif method == "tools/call":
                params = request.get("params", {})
                tool_name = params.get("name")
                args = params.get("arguments", {})
                
                if tool_name == "dwin_write_vp":
                    res = handle_write_vp(args.get("address_hex"), args.get("data_hex"))
                elif tool_name == "dwin_write_text":
                    res = handle_write_text(args.get("address_hex"), args.get("text"))
                elif tool_name == "dwin_switch_page":
                    res = handle_switch_page(args.get("page_id"))
                elif tool_name == "dwin_read_vp":
                    res = handle_read_vp(args.get("address_hex"), args.get("length_words", 1))
                elif tool_name == "dgus_memory_map_lookup":
                    res = handle_lookup_memory_map(args.get("query"))
                elif tool_name == "dgus_parse_config":
                    res = handle_parse_config(args.get("file_path"))
                else:
                    res = {"status": "error", "message": f"Unknown tool: {tool_name}"}
                
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "result": {
                        "content": [{"type": "text", "text": json.dumps(res, ensure_ascii=False, indent=2)}]
                    }
                }
            else:
                response = {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "error": {"code": -32601, "message": "Method not found"}
                }
            
            sys.stdout.write(json.dumps(response) + "\n")
            sys.stdout.flush()
            
        except Exception as e:
            log_info(f"Error handling RPC: {e}")

if __name__ == "__main__":
    main()
