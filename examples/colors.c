// Calls user-defined functions; functions may be defined after they are used.
void main() {
    set_border();
    set_background();
    while (1) {
    }
}

void set_border() {
    poke(0xD020, 5);  // green
}

void set_background() {
    poke(0xD021, 6);  // blue
}
