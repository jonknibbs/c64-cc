// Turns the C64 border red, then loops forever.
void main() {
    poke(0xD020, 2);
    while (1) {
    }
}
