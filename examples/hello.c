// Writes "HELLO C64" into the top-left of screen RAM.
void main() {
    print(0x0400, "HELLO C64");
    while (1) {
    }
}
