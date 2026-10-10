void main() {
    unsigned char total = 0;
    for (unsigned char outer = 0; outer < 8; outer++) {
        if (outer == 2) {
            continue;
        }
        for (unsigned char inner = 0; inner < 4; inner++) {
            if (inner == 2) {
                break;
            }
            total++;
        }
    }
    poke(0xD020, total);
    while (1) {
    }
}