#include "pico/stdlib.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    
    // Khởi tạo UART cho DWIN với tốc độ 921600 baud
    // TODO: Giai đoạn 3 sẽ viết hàm giao tiếp DWIN tại đây

    while (true) {
        printf("RP2350 Core: Đang chờ DWIN và C6...\n");
        sleep_ms(1000);
    }
    return 0;
}
