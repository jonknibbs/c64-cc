#pragma once

#include "c64.h"

void set_border(unsigned char color) {
    c64_border_color = color & 15;
}

void set_background(unsigned char color) {
    c64_background_color = color & 15;
}

unsigned char key(void) {
    return kernal_getin();
}

unsigned char joystick_port2(void) {
    return c64_cia1_registers[0] ^ 255;
}

unsigned char keyboard_matrix_scan(void) {
    unsigned char saved_port_a = c64_cia1_registers[C64_CIA_PORT_A];
    unsigned char saved_ddra = c64_cia1_registers[C64_CIA_DDRA];
    unsigned char saved_ddrb = c64_cia1_registers[C64_CIA_DDRB];
    unsigned char column = 0;
    unsigned char column_bit = 1;
    unsigned char result = 0;

    c64_cia1_registers[C64_CIA_DDRB] = 0;
    c64_cia1_registers[C64_CIA_DDRA] = 255;
    while (column < 8 && result == 0) {
        unsigned char row = 0;
        unsigned char row_bit = 1;
        unsigned char pressed_rows;
        c64_cia1_registers[C64_CIA_PORT_A] = 255 ^ column_bit;
        pressed_rows = c64_cia1_registers[C64_CIA_PORT_B] ^ 255;
        while (row < 8 && result == 0) {
            if (pressed_rows & row_bit) result = row * 8 + column + 1;
            row++;
            row_bit = row_bit * 2;
        }
        column++;
        column_bit = column_bit * 2;
    }

    c64_cia1_registers[C64_CIA_PORT_A] = saved_port_a;
    c64_cia1_registers[C64_CIA_DDRA] = saved_ddra;
    c64_cia1_registers[C64_CIA_DDRB] = saved_ddrb;
    return result;
}

void screen_put(unsigned char offset, unsigned char screen_code, unsigned char color) {
    c64_screen_ram[offset] = screen_code;
    c64_color_ram[offset] = color & 15;
}

void screen_put_row0(unsigned char column, unsigned char screen_code, unsigned char color) {
    if (column < 40) {
        screen_put(column, screen_code, color);
    }
}