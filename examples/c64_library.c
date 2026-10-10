#include "../include/c64_screen.h"
#include "../include/c64_vic.h"

void main() {
    unsigned char code = key();
    while (code == 0) {
        code = key();
    }
    set_border(code & 15);
    set_background(peek(0xD020));
    poke(0xD021, code);
    while (1) {
    }
}