#include "radio_cc1101.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

void cc1101_init(float frequency_mhz) {
    spi_init(CC1101_SPI_PORT, 5000 * 1000); // 5MHz
    gpio_set_function(CC1101_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(CC1101_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CC1101_PIN_MOSI, GPIO_FUNC_SPI);
    
    gpio_init(CC1101_PIN_CS);
    gpio_set_dir(CC1101_PIN_CS, GPIO_OUT);
    gpio_put(CC1101_PIN_CS, 1);
    
    gpio_init(CC1101_PIN_GDO0);
    gpio_set_dir(CC1101_PIN_GDO0, GPIO_IN);

    cc1101_cmd_strobe(CC1101_SRES);
    sleep_ms(10);

    // Default 433.92 MHz
    cc1101_write_reg(0x0D, 0x10); // FREQ2
    cc1101_write_reg(0x0E, 0xB1); // FREQ1
    cc1101_write_reg(0x0F, 0x3B); // FREQ0

    cc1101_write_reg(0x12, 0x30); // MDMCFG2: OOK
    cc1101_write_reg(0x11, 0x83); // MDMCFG3
    cc1101_write_reg(0x10, 0xF8); // MDMCFG4
    
    cc1101_write_reg(0x02, 0x0D); // IOCFG0: Async Serial Output
    
    cc1101_write_reg(0x18, 0x18); // MCSM0
    cc1101_write_reg(0x1D, 0x91); // AGCCTRL0
    cc1101_write_reg(0x0B, 0x06); // FSCTRL1
    
    cc1101_cmd_strobe(CC1101_SIDLE);
}

void cc1101_cmd_strobe(uint8_t cmd) {
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &cmd, 1);
    gpio_put(CC1101_PIN_CS, 1);
}

void cc1101_write_reg(uint8_t regAddr, uint8_t value) {
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &regAddr, 1);
    spi_write_blocking(CC1101_SPI_PORT, &value, 1);
    gpio_put(CC1101_PIN_CS, 1);
}

uint8_t cc1101_read_reg(uint8_t regAddr) {
    uint8_t value;
    regAddr |= 0x80; 
    gpio_put(CC1101_PIN_CS, 0);
    spi_write_blocking(CC1101_SPI_PORT, &regAddr, 1);
    spi_read_blocking(CC1101_SPI_PORT, 0x00, &value, 1);
    gpio_put(CC1101_PIN_CS, 1);
    return value;
}

void cc1101_set_rx_mode(void) {
    cc1101_cmd_strobe(CC1101_SIDLE);
    cc1101_cmd_strobe(CC1101_SFRX);
    cc1101_cmd_strobe(CC1101_SRX);
}

void cc1101_set_tx_mode(void) {
    cc1101_cmd_strobe(CC1101_SIDLE);
    cc1101_cmd_strobe(CC1101_SFTX);
    cc1101_cmd_strobe(CC1101_STX);
}

bool cc1101_decode_princeton(uint32_t* durations, uint16_t count, uint32_t* out_code) {
    if (count < 50) return false;
    
    int sync_idx = -1;
    for (int i = 0; i < count - 49; i++) {
        if (durations[i+1] >= 8000 && durations[i+1] <= 15000) {
            sync_idx = i + 2;
            break;
        }
    }
    
    if (sync_idx == -1) return false;
    
    uint32_t code = 0;
    for (int i = 0; i < 24; i++) {
        int idx = sync_idx + i * 2;
        uint32_t high_dur = durations[idx];
        uint32_t low_dur = durations[idx+1];
        
        if (high_dur == 0 || low_dur == 0) return false;
        
        code <<= 1;
        if (high_dur > low_dur) {
            code |= 1;
        }
    }
    
    *out_code = code;
    return true;
}

bool cc1101_decode_came(uint32_t* durations, uint16_t count, uint32_t* out_code) {
    if (count < 26) return false;
    
    int sync_idx = -1;
    for (int i = 0; i < count - 25; i++) {
        if (durations[i+1] >= 4000 && durations[i+1] <= 7000) {
            sync_idx = i + 2;
            break;
        }
    }
    
    if (sync_idx == -1) return false;
    
    uint32_t code = 0;
    for (int i = 0; i < 12; i++) {
        int idx = sync_idx + i * 2;
        uint32_t high_dur = durations[idx];
        uint32_t low_dur = durations[idx+1];
        
        if (high_dur == 0 || low_dur == 0) return false;
        
        code <<= 1;
        if (high_dur > low_dur) {
            code |= 1;
        }
    }
    
    *out_code = code;
    return true;
}

uint16_t cc1101_encode_princeton(uint32_t code, uint32_t* out_durations) {
    uint16_t idx = 0;
    out_durations[idx++] = 350;
    out_durations[idx++] = 11500;
    
    for (int i = 23; i >= 0; i--) {
        if ((code >> i) & 1) {
            out_durations[idx++] = 1050;
            out_durations[idx++] = 350;
        } else {
            out_durations[idx++] = 350;
            out_durations[idx++] = 1050;
        }
    }
    return idx;
}

uint16_t cc1101_encode_came(uint32_t code, uint32_t* out_durations) {
    uint16_t idx = 0;
    out_durations[idx++] = 320;
    out_durations[idx++] = 5760;
    
    for (int i = 11; i >= 0; i--) {
        if ((code >> i) & 1) {
            out_durations[idx++] = 640;
            out_durations[idx++] = 320;
        } else {
            out_durations[idx++] = 320;
            out_durations[idx++] = 640;
        }
    }
    return idx;
}

void cc1101_transmit_raw(uint32_t* durations, uint16_t count) {
    cc1101_set_tx_mode();
    gpio_set_dir(CC1101_PIN_GDO0, GPIO_OUT);
    
    for (uint16_t i = 0; i < count; i++) {
        gpio_put(CC1101_PIN_GDO0, (i % 2 == 0) ? 1 : 0);
        sleep_us(durations[i]);
    }
    
    gpio_put(CC1101_PIN_GDO0, 0);
    gpio_set_dir(CC1101_PIN_GDO0, GPIO_IN);
    cc1101_cmd_strobe(CC1101_SIDLE);
}

void cc1101_transmit_signal(uint8_t* payload, uint8_t length) {
    cc1101_set_tx_mode();
    
    gpio_put(CC1101_PIN_CS, 0);
    uint8_t tx_fifo_addr = 0x3F;
    spi_write_blocking(CC1101_SPI_PORT, &tx_fifo_addr, 1);
    spi_write_blocking(CC1101_SPI_PORT, payload, length);
    gpio_put(CC1101_PIN_CS, 1);
    
    cc1101_cmd_strobe(CC1101_STX);
    sleep_ms(20);
    cc1101_cmd_strobe(CC1101_SIDLE);
}
