#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "gesture.h"

int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // wait for USB serial to connect

    if (!apds_init()) {
        printf("[FAIL]  Sensor init failed. Halting.\n");
        while (1) tight_loop_contents();
    }

    multicore_launch_core1(core1_entry);

    while (1) {
        if (multicore_fifo_rvalid()) {
            uint32_t dir = multicore_fifo_pop_blocking();
            switch (dir) {
                case DIR_UP:    printf(">>> GESTURE: UP\n\n");    break;
                case DIR_DOWN:  printf(">>> GESTURE: DOWN\n\n");  break;
                case DIR_LEFT:  printf(">>> GESTURE: LEFT\n\n");  break;
                case DIR_RIGHT: printf(">>> GESTURE: RIGHT\n\n"); break;
            }
            // TODO: replace printf with move_character(dir)
        }

        // TODO: game loop runs here — rendering, physics, etc.
    }
}
