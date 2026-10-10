#include "../include/c64.h"

unsigned char set_color(void);
__at(0x2000) unsigned char shared_bytes[4];

void main() {
    shared_bytes[0] = 6;
    set_color();
    while (1) {
    }
}

__at(0x1000) unsigned char set_color(void) {
    set_border(2);
    return shared_bytes[0];
}