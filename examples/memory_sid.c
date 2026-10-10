#include "../include/c64.h"

void main() {
    c64_sid_registers[24] = 15;
    c64_sid_registers[5] = 9;
    c64_sid_registers[6] = 0xF0;
    c64_sid_registers[1] = 0x1D;
    c64_sid_registers[0] = 0xCF;
    c64_sid_registers[4] = 0x11;
    while (1) {
    }
}