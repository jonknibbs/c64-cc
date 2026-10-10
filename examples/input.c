#include "../include/c64_screen.h"

void main() {
    unsigned char code = key();
    while (code == 0) {
        code = key();
    }
    set_border(code & 15);
    set_background(code & 7);
    while (1) {
    }
}