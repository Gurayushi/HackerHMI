#ifndef C5_SPI_H
#define C5_SPI_H

#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define C5_SPI_PORT      spi0
#define C5_PIN_MISO      16
#define C5_PIN_CS        17
#define C5_PIN_SCK       18
#define C5_PIN_MOSI      19
#define C5_PIN_HANDSHAKE 22

void init_spi_for_c5(void);
void push_config_to_c5(const char* config_str);

#endif // C5_SPI_H
