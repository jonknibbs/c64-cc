#pragma once

#include "c64.h"

void vic_wait_raster(unsigned char line) {
    while (c64_vic_raster != line) {
    }
}

void vic_wait_frame(void) {
    while (c64_vic_raster < 250) {
    }
    while (c64_vic_raster >= 250) {
    }
}

void vic_wait_frames(unsigned char frames) {
    while (frames) {
        vic_wait_frame();
        frames--;
    }
}

void cia1_timer_a_start(unsigned char latch_low, unsigned char latch_high) {
    c64_cia1_registers[C64_CIA_TIMER_A_CONTROL] = 0;
    c64_cia1_registers[C64_CIA_TIMER_A_LOW] = latch_low;
    c64_cia1_registers[C64_CIA_TIMER_A_HIGH] = latch_high;
    c64_cia1_registers[C64_CIA_TIMER_A_CONTROL] = 0x11;
}

void cia1_timer_a_stop(void) {
    c64_cia1_registers[C64_CIA_TIMER_A_CONTROL] =
        c64_cia1_registers[C64_CIA_TIMER_A_CONTROL] & 0xFE;
}

unsigned char cia1_timer_a_read_high(void) {
    return c64_cia1_registers[C64_CIA_TIMER_A_HIGH];
}

void sprite_enable(unsigned char sprite, unsigned char enabled) {
    unsigned char mask = 1 << sprite;
    if (sprite >= 8) return;
    if (enabled) c64_vic_sprite_enable = c64_vic_sprite_enable | mask;
    else c64_vic_sprite_enable = c64_vic_sprite_enable & ~mask;
}

void sprite_position(unsigned char sprite, unsigned char x, unsigned char y) {
    unsigned char offset = sprite * 2;
    if (sprite >= 8) return;
    c64_vic_sprite_coordinates[offset] = x;
    c64_vic_sprite_coordinates[offset + 1] = y;
}

void sprite_color(unsigned char sprite, unsigned char color) {
    if (sprite < 8) c64_vic_sprite_colors[sprite] = color & 15;
}

void sprite_pointer(unsigned char sprite, unsigned char bitmap_pointer) {
    if (sprite < 8) c64_vic_sprite_pointers[sprite] = bitmap_pointer;
}

void sprite_x_msb(unsigned char sprite, unsigned char enabled) {
    unsigned char mask = 1 << sprite;
    if (sprite >= 8) return;
    if (enabled) c64_vic_sprite_x_msb = c64_vic_sprite_x_msb | mask;
    else c64_vic_sprite_x_msb = c64_vic_sprite_x_msb & ~mask;
}

void sprite_multicolor(unsigned char sprite, unsigned char enabled) {
    unsigned char mask = 1 << sprite;
    if (sprite >= 8) return;
    if (enabled) c64_vic_sprite_multicolor = c64_vic_sprite_multicolor | mask;
    else c64_vic_sprite_multicolor = c64_vic_sprite_multicolor & ~mask;
}

void sprite_bitmap_write(unsigned char offset, unsigned char value) {
    if (offset < 63) c64_sprite_bitmap[offset] = value;
}

void sprite_bitmap_clear(void) {
    unsigned char offset = 0;
    while (offset < 63) {
        c64_sprite_bitmap[offset] = 0;
        offset++;
    }
}