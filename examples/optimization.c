void main() {
    unsigned char color = (2 + 3) ^ 0;
    poke(0xD020, color);
    while (1) {
    }
}