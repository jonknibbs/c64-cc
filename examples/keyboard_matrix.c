#include "../include/c64_screen.h"
#include "../include/c64_vic.h"

void main() {
    unsigned char key_code = 0;
    while (1) {
        key_code = keyboard_matrix_scan();
        if (key_code) {
            screen_put_row0(0, key_code, 1);
            set_border(key_code & 15);
        }
        vic_wait_frame();
    }
}