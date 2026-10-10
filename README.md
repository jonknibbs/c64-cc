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
c64cc input.c [-I dir] [-o out.prg] [-S out.asm] [--run] [--vice path]
```

| Option | Description |
|---|---|
| `input.c` | Source file to compile |
| `-I dir` | Add a directory to quoted-header search paths |
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

`c64cc` implements a small, byte-oriented C dialect rather than full ISO C:

- `void` and byte-returning functions with up to 8 byte parameters; `main` is the entry point
- Global and function-local `char`, `unsigned char`, and `int` variables; all are unsigned 8-bit values
- Decimal and `0x` constants, assignment, `++`/`--`, unary `+`, `-`, `!`, `~`
- `+`, `-`, `*`, `/`, `%`, shifts `<<`/`>>`, bitwise `&`, `|`, `^`, comparisons, and short-circuit `&&`/`||`
- `if`/`else`, dynamic `while` and `for` loops, `break`, `continue`, blocks, and `return`
- Block-scoped locals with nested shadowing; `for`-initializer variables are scoped to their loop
- `__at(0xADDRESS)` placement for global objects and function definitions, with compiler-owned overlap checks
- `__mem(0xADDRESS)` bindings for direct scalar/array access to existing RAM and hardware registers
- Fixed-size global and local byte arrays with variable-index reads and writes
- 16-bit pointer variables initialized/assigned from constant addresses, with unary `*` reads and writes
- Calls to user functions with byte arguments and returns; write prototypes before calls that pass arguments or use return values
- `poke(constant_address, expression)` and `peek(constant_address)`
- C64 helpers `set_border(expression)`, `set_background(expression)`, and `key()` (KERNAL `GETIN`)
- Sprite helpers for enabling, positioning, coloring, selecting bitmap pointers, and writing bitmap data
- Numeric object-like `#define` constants in included headers; `include/c64.h` names common VIC-II, screen, SID, and CIA locations
- `print(constant_address, "text")` writes supported characters as C64 screen codes
- `//` and `/* */` comments
- Quoted `#include "header.h"` and `#pragma once` for reusable declarations
- `c64cc --help` and `c64cc --version` for command-line discovery
- Constant arithmetic and bitwise expressions are folded before output when the emitted pattern is safe to simplify

Variables, parameters, arrays, pointers, and expression temporaries currently have static storage despite lexical scope, so functions are not reentrant or recursive. Array bounds are not checked at runtime. Pointers do not yet support arithmetic or array decay; their addresses must be constants. The compiler uses zero-page `$FB/$FC` as a scratch pointer while dereferencing. Addresses passed to `peek` and `poke` must be constants. Arithmetic wraps to 8 bits; division by zero produces zero for `/` and leaves the dividend for `%`.

Run the compile smoke tests with:

```powershell
ctest --test-dir build --output-on-failure
```

## Examples

| File | Shows |
|---|---|
| [examples/border.c](examples/border.c) | Turns the border red |
| [examples/hello.c](examples/hello.c) | Writes "HELLO C64" to screen RAM |
| [examples/colors.c](examples/colors.c) | User-defined functions, including forward use |
| [examples/sid_tone.c](examples/sid_tone.c) | Plays a continuous note on the SID chip |
| [examples/expressions.c](examples/expressions.c) | Byte variables, all implemented operators, `peek`, and dynamic control flow |
| [examples/functions.c](examples/functions.c) | Prototypes, byte parameters, function calls, and return values |
| [examples/arrays.c](examples/arrays.c) | Global/local arrays and variable-index access |
| [examples/control_flow.c](examples/control_flow.c) | Nested `for` loops, `continue`, and `break` |
| [examples/input.c](examples/input.c) | KERNAL keyboard polling and VIC-II color helpers |
| [examples/pointers.c](examples/pointers.c) | Pointer reads/writes to C64 memory-mapped and screen memory |
| [examples/optimization.c](examples/optimization.c) | Constant-folded byte expressions |
| [examples/c64_library.c](examples/c64_library.c) | Reusable built-in declarations from `include/c64.h` |
| [examples/sprites.c](examples/sprites.c) | VIC-II sprite bitmap setup and movement |
| [examples/scopes.c](examples/scopes.c) | Nested local scopes, shadowing, and loop-variable scope |
| [examples/placement.c](examples/placement.c) | Explicit function and global-array placement |
| [examples/memory_variables.c](examples/memory_variables.c) | Direct variables for C64 RAM and VIC-II registers |
| [examples/memory_screen.c](examples/memory_screen.c) | Writes screen and color RAM through mapped arrays |
| [examples/memory_sprite.c](examples/memory_sprite.c) | Builds and moves a sprite through mapped VIC-II registers |
| [examples/memory_sid.c](examples/memory_sid.c) | Plays a SID tone through mapped sound registers |

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

For setup, language syntax, C64 helpers, examples, and current limitations, see the [c64cc Manual](MANUAL.md).
Remaining development priorities are tracked in [TODO.md](TODO.md).
