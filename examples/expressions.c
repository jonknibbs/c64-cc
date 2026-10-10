unsigned char base = 2;

void main() {
    char count = 0;
    int value = 0;
    value = +((7 + 3 - 2) * 2 / 4 % 3);
    value = (value & 3) | (1 ^ 2);
    value = ~-value;
    value = (value << 1) | (value >> 1);
    value = peek(0xD020);
    while (count < 3 && base != 0) {
        if (count == 1 || count > 2) {
            poke(0xD020, 5);
        } else {
            poke(0xD020, value + count + base);
        }
        count++;
    }
    count--;
    while (1) {
    }
}