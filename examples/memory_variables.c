#include "../include/c64.h"

void main() {
    c64_screen_ram[0] = 1;
    c64_screen_ram[1] = 2;
    c64_color_ram[0] = 7;
    c64_border_color = 6;
    c64_background_color = 0;
    c64_vic_sprite_enable = 1;
    c64_vic_sprite_colors[0] = 1;
    while (c64_vic_raster < 250) {
    }
    while (1) {
    }
}