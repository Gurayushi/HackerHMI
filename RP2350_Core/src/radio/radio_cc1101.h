#ifndef RADIO_CC1101_H
#define RADIO_CC1101_H

#include <stdint.h>
#include <stdbool.h>

#define CC1101_SPI_PORT spi1
#define CC1101_PIN_MISO 12
#define CC1101_PIN_CS   13
#define CC1101_PIN_SCK  14
#define CC1101_PIN_MOSI 15
#define CC1101_PIN_GDO0 10 

#define CC1101_SRES     0x30
#define CC1101_SFSTXON  0x31
#define CC1101_SIDLE    0x36
#define CC1101_SFRX     0x3A
#define CC1101_SFTX     0x3B
#define CC1101_SRX      0x34
#define CC1101_STX      0x35

void cc1101_init(float frequency_mhz);
void cc1101_write_reg(uint8_t regAddr, uint8_t value);
uint8_t cc1101_read_reg(uint8_t regAddr);
void cc1101_cmd_strobe(uint8_t cmd);
void cc1101_set_rx_mode(void);
void cc1101_set_tx_mode(void);
bool cc1101_decode_princeton(uint32_t* durations, uint16_t count, uint32_t* out_code);
bool cc1101_decode_came(uint32_t* durations, uint16_t count, uint32_t* out_code);
uint16_t cc1101_encode_princeton(uint32_t code, uint32_t* out_durations);
uint16_t cc1101_encode_came(uint32_t code, uint32_t* out_durations);
void cc1101_transmit_raw(uint32_t* durations, uint16_t count);
void cc1101_transmit_signal(uint8_t* payload, uint8_t length);

#endif // RADIO_CC1101_H
