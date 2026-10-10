// Plays a continuous triangle-wave note on SID voice 1 (about 440 Hz on PAL).
void main() {
    poke(0xD418, 15);   // master volume
    poke(0xD405, 0x09); // attack/decay
    poke(0xD406, 0xF0); // sustain/release
    poke(0xD401, 0x1D); // frequency high byte
    poke(0xD400, 0xCF); // frequency low byte
    poke(0xD404, 0x11); // triangle waveform + gate on
    while (1) {
    }
}
