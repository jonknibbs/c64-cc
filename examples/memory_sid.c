#include "../include/c64_sid.h"

void main() {
    sid_set_volume(15);
    sid_set_adsr(0, 9, 0xF0);
    sid_set_frequency(0, 0xCF, 0x1D);
    sid_set_control(0, 0x11);
    while (1) {
    }
}