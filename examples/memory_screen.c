#include "../include/c64.h"

void main() {
    unsigned char index = 0;
    while (index < 40) {
        c64_screen_ram[index] = (index % 26) + 1;
        c64_color_ram[index] = index % 16;
        index++;
    }
    c64_border_color = 6;
    c64_background_color = 0;
    while (1) {
    }
}