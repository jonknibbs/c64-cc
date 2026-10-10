#pragma once

#include "c64.h"

void sid_set_volume(unsigned char volume) {
    c64_sid_registers[24] = volume & 15;
}

void sid_set_frequency(unsigned char voice, unsigned char low, unsigned char high) {
    unsigned char offset = voice * 7;
    if (voice >= 3) return;
    c64_sid_registers[offset] = low;
    c64_sid_registers[offset + 1] = high;
}

void sid_set_adsr(unsigned char voice, unsigned char attack_decay,
                  unsigned char sustain_release) {
    unsigned char offset = voice * 7;
    if (voice >= 3) return;
    c64_sid_registers[offset + 5] = attack_decay;
    c64_sid_registers[offset + 6] = sustain_release;
}

void sid_set_control(unsigned char voice, unsigned char control) {
    unsigned char offset = voice * 7;
    if (voice < 3) c64_sid_registers[offset + 4] = control;
}