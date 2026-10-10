#include "../include/c64_screen.h"

void main() {
    unsigned char index = 0;
    while (index < 40) {
        screen_put(index, (index % 26) + 1, index % 16);
        index++;
    }
    c64_border_color = 6;
    c64_background_color = 0;
    while (1) {
    }
}