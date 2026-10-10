#include "../include/c64_screen.h"
#include "../include/c64_vic.h"
#include "../include/c64_sid.h"

void main() {
    unsigned char joystick = joystick_port2();
    screen_put_row0(0, 3, 1);
    if (joystick & 1) set_border(2);
    sprite_position(0, 120, 100);
    sprite_color(0, 1);
    sprite_enable(0, 1);
    sid_set_volume(15);
    sid_set_frequency(0, 0xCF, 0x1D);
    sid_set_adsr(0, 9, 0xF0);
    sid_set_control(0, 0x11);
    while (1) {
    }
}