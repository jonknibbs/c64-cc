unsigned char colors[4];

void main() {
    unsigned char index = 0;
    char local_values[2];
    char single_value[1];
    colors[0] = 2;
    local_values[0] = 5;
    single_value[0] = 7;
    while (index < 4) {
        colors[index] = colors[0] + index;
        poke(0xD020, colors[index]);
        index++;
    }
    poke(0xD021, local_values[0]);
    poke(0xD020, single_value[0]);
    while (1) {
    }
}