#include "../include/c64.h"

void main() {
    unsigned char x = 120;
    unsigned char y = 100;
    unsigned char byte = 0;
    sprite_bitmap_clear();
    for (byte = 0; byte < 63; byte++) {
        sprite_bitmap_write(byte, 0xFF);
    }
    sprite_pointer(0, 0x80);
    sprite_position(0, x, y);
    sprite_color(0, 1);
    sprite_multicolor(0, 0);
    sprite_x_msb(0, 0);
    sprite_enable(0, 1);
    poke(C64_BORDER_COLOR, 6);
    while (1) {
        while (peek(C64_VIC_RASTER) < 250) {
        }
        while (peek(C64_VIC_RASTER) >= 250) {
        }
        x++;
        sprite_position(0, x, y);
    }
}