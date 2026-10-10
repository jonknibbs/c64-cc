unsigned char *target = 0xD020;

void main() {
    unsigned char *screen = 0x0400;
    *target = 2;
    target = 0xD021;
    *target = 6;
    *screen = 1;
    unsigned char code = *screen;
    *target = code;
    while (1) {
    }
}