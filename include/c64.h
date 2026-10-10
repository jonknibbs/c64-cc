#pragma once

#define C64_SCREEN_RAM 0x0400
#define C64_COLOR_RAM 0xD800
#define C64_BORDER_COLOR 0xD020
#define C64_BACKGROUND_COLOR 0xD021
#define C64_VIC_RASTER 0xD012
#define C64_VIC_SPRITE_X_MSB 0xD010
#define C64_VIC_SPRITE_ENABLE 0xD015
#define C64_VIC_SPRITE_MULTICOLOR 0xD01C
#define C64_VIC_SPRITE_COLOR_BASE 0xD027
#define C64_VIC_SPRITE_POINTERS 0x07F8
#define C64_SPRITE_BITMAP 0x2000
#define C64_SID_BASE 0xD400
#define C64_CIA1_BASE 0xDC00
#define C64_CIA2_BASE 0xDD00

void set_border(unsigned char color);
void set_background(unsigned char color);
unsigned char key(void);
unsigned char peek(unsigned int address);
void poke(unsigned int address, unsigned char value);
void sprite_enable(unsigned char sprite, unsigned char enabled);
void sprite_position(unsigned char sprite, unsigned char x, unsigned char y);
void sprite_color(unsigned char sprite, unsigned char color);
void sprite_pointer(unsigned char sprite, unsigned char bitmap_pointer);
void sprite_x_msb(unsigned char sprite, unsigned char enabled);
void sprite_multicolor(unsigned char sprite, unsigned char enabled);
void sprite_bitmap_clear(void);
void sprite_bitmap_write(unsigned char offset, unsigned char value);