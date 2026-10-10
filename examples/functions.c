unsigned char add(unsigned char left, unsigned char right);
void show_value(unsigned char value);

void main() {
    unsigned char result = add(20, 22);
    if (result == 42) {
        show_value(result);
    }
    while (1) {
    }
}

unsigned char add(unsigned char left, unsigned char right) {
    return left + right;
}

void show_value(unsigned char value) {
    poke(0xD020, value);
}