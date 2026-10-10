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
#define C64_CIA_PORT_A 0
#define C64_CIA_PORT_B 1
#define C64_CIA_DDRA 2
#define C64_CIA_DDRB 3
#define C64_CIA_TIMER_A_LOW 4
#define C64_CIA_TIMER_A_HIGH 5
#define C64_CIA_TIMER_A_CONTROL 14
#define C64_VIC_SPRITE_COORDINATES 0xD000
#define C64_SCREEN_RAM_PAGE_1 0x0500
#define C64_SCREEN_RAM_PAGE_2 0x0600
#define C64_SCREEN_RAM_PAGE_3 0x0700
#define C64_COLOR_RAM_PAGE_1 0xD900
#define C64_COLOR_RAM_PAGE_2 0xDA00
#define C64_COLOR_RAM_PAGE_3 0xDB00
__mem(C64_SCREEN_RAM) volatile unsigned char c64_screen_ram[256];
__mem(C64_SCREEN_RAM_PAGE_1) volatile unsigned char c64_screen_ram_page_1[256];
__mem(C64_SCREEN_RAM_PAGE_2) volatile unsigned char c64_screen_ram_page_2[256];
__mem(C64_SCREEN_RAM_PAGE_3) volatile unsigned char c64_screen_ram_page_3[232];
__mem(C64_COLOR_RAM) volatile unsigned char c64_color_ram[256];
__mem(C64_COLOR_RAM_PAGE_1) volatile unsigned char c64_color_ram_page_1[256];
__mem(C64_COLOR_RAM_PAGE_2) volatile unsigned char c64_color_ram_page_2[256];
__mem(C64_COLOR_RAM_PAGE_3) volatile unsigned char c64_color_ram_page_3[232];
__mem(C64_BORDER_COLOR) volatile unsigned char c64_border_color;
__mem(C64_BACKGROUND_COLOR) volatile unsigned char c64_background_color;
__mem(C64_VIC_RASTER) volatile unsigned char c64_vic_raster;
__mem(C64_VIC_SPRITE_X_MSB) volatile unsigned char c64_vic_sprite_x_msb;
__mem(C64_VIC_SPRITE_ENABLE) volatile unsigned char c64_vic_sprite_enable;
__mem(C64_VIC_SPRITE_MULTICOLOR) volatile unsigned char c64_vic_sprite_multicolor;
__mem(C64_VIC_SPRITE_COORDINATES) volatile unsigned char c64_vic_sprite_coordinates[16];
__mem(C64_VIC_SPRITE_COLOR_BASE) volatile unsigned char c64_vic_sprite_colors[8];
__mem(C64_VIC_SPRITE_POINTERS) volatile unsigned char c64_vic_sprite_pointers[8];
__mem(C64_SPRITE_BITMAP) volatile unsigned char c64_sprite_bitmap[63];
__mem(C64_SID_BASE) volatile unsigned char c64_sid_registers[25];
__mem(C64_CIA1_BASE) volatile unsigned char c64_cia1_registers[16];
__mem(C64_CIA2_BASE) volatile unsigned char c64_cia2_registers[16];

unsigned char kernal_getin(void);
unsigned char peek(unsigned int address);
void poke(unsigned int address, unsigned char value);

