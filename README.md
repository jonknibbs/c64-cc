# c64cc

A small C compiler for the Commodore 64. It compiles a C subset to 6502 machine code and writes a `.prg` file (with a BASIC `SYS` stub) and, optionally, an assembly listing.

## Requirements

- A C compiler (GCC or Clang), CMake 3.20+, and Ninja (or another CMake generator)
- Optional: [VICE](https://vice-emu.sourceforge.io/) to run the output (`winget install VICE.VICE`)

## Build

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

This produces `build/c64cc` (`build\c64cc.exe` on Windows).

## Usage

```
c64cc input.c [-o out.prg] [-S out.asm] [--run] [--vice path]
```

| Option | Description |
|---|---|
| `input.c` | Source file to compile |
| `-o out.prg` | Output program (default: `out.prg`) |
| `-S out.asm` | Also write a readable assembly listing |
| `--run` | Launch the generated program in VICE after compiling |
| `--vice path` | VICE executable to launch with `--run` (default: `x64sc`) |

Example:

```powershell
.\build\c64cc.exe examples\border.c -o build\border.prg -S build\border.asm --run
```

On Windows, if VICE is not on `PATH`, pass its executable explicitly, for example:

```powershell
.\build\c64cc.exe examples\colors.c -o build\colors.prg --run --vice "C:\path\to\x64sc.exe"
```

Or load it in the emulator with `LOAD "BORDER.PRG",8,1` and `RUN`. On real hardware, copy the `.prg` to a disk image or SD device.

## Supported language

Current subset:

- `void` functions with no parameters; `main` is the entry point
- Blocks `{ ... }`, empty statements `;`, and `return;`
- `while (constant) statement`
- `poke(address, value);` with constant arguments (address 0..65535, value 0..255)
- Calls to your own functions: `name();`
- Integer constants (decimal or `0x` hex), plus `//` and `/* */` comments

Not yet supported: variables, expressions, `if`, parameters, return values.

## Examples

| File | Shows |
|---|---|
| [examples/border.c](examples/border.c) | Turns the border red |
| [examples/hello.c](examples/hello.c) | Writes "HELLO C64" to screen RAM |
| [examples/colors.c](examples/colors.c) | User-defined functions, including forward use |
| [examples/sid_tone.c](examples/sid_tone.c) | Plays a continuous note on the SID chip |

```c
// Turns the C64 border red, then loops forever.
void main() {
    poke(0xD020, 2);
    while (1) {
    }
}
```

## Output layout

The `.prg` loads at `$0801` with the BASIC line `10 SYS 2061`. Code starts at `$080D` with a startup stub that calls `main` and then loops forever.

## Source layout

| File | Role |
|---|---|
| `src/lexer.c` | Tokenizer |
| `src/compiler.c` | Parser and code generator |
| `src/emit.c` | Instruction list, assembly listing, and `.prg` encoder |
| `src/main.c` | Command-line driver |
