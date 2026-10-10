#include "../include/c64.h"

unsigned char value = 1;

void main() {
    unsigned char item = 2;
    {
        unsigned char item = 3;
        poke(C64_BORDER_COLOR, item);
    }
    poke(C64_BACKGROUND_COLOR, item);
    for (unsigned char index = 0; index < 2; index++) {
        poke(C64_BORDER_COLOR, index);
    }
    poke(C64_BACKGROUND_COLOR, value);
    while (1) {
    }
}