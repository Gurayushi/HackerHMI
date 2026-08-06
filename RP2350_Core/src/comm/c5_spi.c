#include "c5_spi.h"
#include <string.h>

void init_spi_for_c5(void) {
    spi_init(C5_SPI_PORT, 1000 * 1000); // 1 MHz
    gpio_set_function(C5_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(C5_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(C5_PIN_MOSI, GPIO_FUNC_SPI);
    
    gpio_init(C5_PIN_CS);
    gpio_set_dir(C5_PIN_CS, GPIO_OUT);
    gpio_put(C5_PIN_CS, 1);

    gpio_init(C5_PIN_HANDSHAKE);
    gpio_set_dir(C5_PIN_HANDSHAKE, GPIO_IN);
    gpio_pull_up(C5_PIN_HANDSHAKE);
}

void push_config_to_c5(const char* config_str) {
    gpio_put(C5_PIN_CS, 0);
    spi_write_blocking(C5_SPI_PORT, (const uint8_t*)config_str, strlen(config_str));
    gpio_put(C5_PIN_CS, 1);
}
