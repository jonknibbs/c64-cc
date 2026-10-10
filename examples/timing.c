#include "../include/c64_screen.h"
#include "../include/c64_vic.h"

void main() {
    unsigned char timer_high = 0;
    cia1_timer_a_start(0x40, 0x42);
    while (1) {
        vic_wait_raster(100);
        timer_high = cia1_timer_a_read_high();
        set_border(timer_high & 15);
        vic_wait_frames(1);
    }
}