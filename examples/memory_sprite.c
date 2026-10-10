#include "../include/c64.h"

void main() {
    unsigned char offset = 0;
    unsigned char x = 120;
    while (offset < 63) {
        c64_sprite_bitmap[offset] = 0xFF;
        offset++;
    }
    c64_vic_sprite_pointers[0] = 0x80;
    c64_vic_sprite_coordinates[0] = x;
    c64_vic_sprite_coordinates[1] = 100;
    c64_vic_sprite_colors[0] = 1;
    c64_vic_sprite_enable = 1;
    while (1) {
        while (c64_vic_raster < 250) {
        }
        while (c64_vic_raster >= 250) {
        }
        x++;
        c64_vic_sprite_coordinates[0] = x;
    }
}