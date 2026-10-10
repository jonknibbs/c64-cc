# c64cc Manual

`c64cc` compiles a small, byte-oriented C dialect into a Commodore 64 PRG. It is intended for small demos and experiments, not as a replacement for a full C compiler.

## 1. Build c64cc

Requirements:

- CMake 3.20 or newer
- A C compiler such as GCC or Clang
- Ninja, unless you choose another CMake generator
- VICE is optional and is only needed for the `--run` option

From the project directory, build with:

```powershell
cmake -S . -B build -G Ninja
cmake --build build
```

On Windows, the compiler executable is `build\c64cc.exe`.

## 2. Compile and Run

Basic compilation:

```powershell
.\build\c64cc.exe examples\border.c -o build\border.prg
```

Write an assembly listing as well:

```powershell
.\build\c64cc.exe examples\border.c -o build\border.prg -S build\border.asm
```

Quoted headers are expanded from the including file's directory first, then from each `-I` directory:

```powershell
.\build\c64cc.exe examples\c64_library.c -I include -o build\c64_library.prg
```

`#pragma once` prevents repeated expansion. Numeric object-like definitions such as `#define SCREEN 0x0400` are supported. Function-like macros, angle-bracket includes, and conditional preprocessing are not supported.

Compile and launch in VICE:

```powershell
.\build\c64cc.exe examples\border.c -o build\border.prg --run --vice "C:\path\to\x64sc.exe"
```

The `--vice` option is optional when `x64sc` is available on `PATH`. Without `--run`, copy the generated PRG to a disk image or storage device and load it on a C64. The PRG includes a BASIC `SYS 2061` stub, so it can be started with `LOAD "BORDER.PRG",8,1` followed by `RUN`.

Use `c64cc --help` for the command summary and `c64cc --version` to print the compiler version.

## 3. Program Shape

The entry point is `void main()`. Other functions can appear before or after `main`; add a prototype before an earlier call when that call passes arguments or uses a return value.

```c
unsigned char add(unsigned char left, unsigned char right);

void main() {
    unsigned char result = add(20, 22);
    set_border(result);
    while (1) {
    }
}

unsigned char add(unsigned char left, unsigned char right) {
    return left + right;
}
```

See [examples/functions.c](examples/functions.c) for a complete buildable program.

## 4. Types and Storage

Supported scalar spellings are `char`, `unsigned char`, and `int`. In this dialect, each is an unsigned 8-bit value. Arithmetic wraps modulo 256. For example, assigning `255 + 1` produces `0`.

Global and local variables can be initialized; global initializers must be constants. Fixed-size arrays can be declared globally or inside a function:

```c
unsigned char colors[4];

void main() {
    unsigned char index = 0;
    colors[index] = 2;
}
```

Array lengths are 1 through 255. Initializer lists and bounds checking are not implemented.

Variables, parameters, arrays, and compiler temporaries use static storage. Local variables therefore have function-wide storage rather than block scope, and functions are not reentrant or recursive. Initializers on local scalar variables are emitted as code at the declaration point.

## 5. Expressions

Supported operators, from higher to lower precedence:

| Group | Operators |
|---|---|
| Unary | `+`, `-`, `!`, `~`, dereference `*` |
| Multiplicative | `*`, `/`, `%` |
| Additive | `+`, `-` |
| Shift | `<<`, `>>` |
| Relational | `<`, `<=`, `>`, `>=` |
| Equality | `==`, `!=` |
| Bitwise | `&`, `^`, `|` |
| Logical | `&&`, `||` |

Logical operators short-circuit and produce `0` or `1`. Comparisons are unsigned. Right shift is logical. Multiplication, division, and shifts are implemented in generated 6502 code and can be slower than hand-written assembly. Division by zero yields zero for `/`; `%` returns the dividend.

Expressions support constants, variables, function calls that return a byte, and parentheses. Assignment is a statement, not an expression. Post-increment and post-decrement statements (`value++`, `value--`) are supported.

## 6. Functions and Control Flow

Functions may return `void` or one byte and accept up to eight byte parameters. Use `return;` in a `void` function and `return expression;` in a byte-returning function. A byte-returning function must provide a value at each explicit `return`.

Supported statements include blocks, empty statements, `if`/`else`, `while`, and `for`. `break` exits the innermost loop; `continue` moves to the next iteration. A `for` initializer can be a variable declaration or a scalar assignment. Its update clause supports assignment, `++`, or `--`.

## 7. Pointers and Memory

Byte pointers are 16-bit addresses. They may be initialized or reassigned with a constant address, then dereferenced for a byte read or write:

```c
unsigned char *screen = 0x0400;

void main() {
    *screen = 1;
}
```

Pointer arithmetic, taking addresses with `&`, and automatic array-to-pointer conversion are not supported. Dereferencing uses zero-page locations `$FB` and `$FC` as scratch storage.

The `peek(address)` and `poke(address, value)` helpers access a constant address. `poke` accepts an expression for its value. For an address held in a pointer variable, use `*pointer` instead.

## 8. C64 Helpers

The common built-in declarations are in [include/c64.h](include/c64.h). Include it with `#include "c64.h"` and pass `-I include` when compiling from elsewhere. The compiler implements these calls directly; the header provides declarations, not separate runtime code. The `print` helper remains available as a compiler built-in but is not declared in this header.

The header also defines useful addresses, including `C64_SCREEN_RAM`, `C64_COLOR_RAM`, `C64_BORDER_COLOR`, `C64_BACKGROUND_COLOR`, `C64_VIC_RASTER`, `C64_VIC_SPRITE_ENABLE`, `C64_VIC_SPRITE_POINTERS`, `C64_SPRITE_BITMAP`, `C64_SID_BASE`, and the CIA base addresses.

| Helper | Behavior |
|---|---|
| `set_border(value)` | Write a byte to VIC-II border color register `$D020` |
| `set_background(value)` | Write a byte to VIC-II background color register `$D021` |
| `key()` | Read a key through the KERNAL `GETIN` routine at `$FFE4`; returns zero when no key is available |
| `print(address, "text")` | Write supported characters to screen memory as C64 screen codes |

`print` maps letters to screen codes 1 through 26. Other accepted characters use their byte values for codes 32 through 63. For other hardware access, use `peek`, `poke`, or a pointer.

### Sprites

The sprite helpers select sprite numbers as compile-time constants from 0 through 7:

| Helper | Behavior |
|---|---|
| `sprite_enable(sprite, enabled)` | Set or clear the sprite's bit in `$D015` |
| `sprite_position(sprite, x, y)` | Write the low 8 bits of X and the Y coordinate |
| `sprite_x_msb(sprite, enabled)` | Set or clear the ninth X-coordinate bit in `$D010` |
| `sprite_color(sprite, color)` | Set the sprite's individual color |
| `sprite_multicolor(sprite, enabled)` | Set or clear the sprite's multicolor bit in `$D01C` |
| `sprite_pointer(sprite, pointer)` | Write the bitmap pointer in the default screen's pointer table at `$07F8` |
| `sprite_bitmap_clear()` | Clear 63 bitmap bytes beginning at `$2000` |
| `sprite_bitmap_write(offset, value)` | Write a byte at `$2000 + offset` using indexed addressing; keep offset in 0..62 |

The bitmap helpers assume VIC-II bank 0 and bitmap storage at `$2000`; `sprite_pointer(0, 0x80)` selects that address because each bitmap occupies 64 bytes. A sprite bitmap uses 63 bytes, with the final byte reserved. For X positions above 255, set the low coordinate with `sprite_position` and control its high bit separately with `sprite_x_msb`. See [examples/sprites.c](examples/sprites.c) for a complete moving sprite setup.

## 9. Output and Tests

The generated PRG loads at `$0801`. Its BASIC stub calls the machine code at `$080D` (`SYS 2061`); the startup code calls `main` and then parks the CPU. The compiler rejects output that exceeds the 6502 address space.

Compile the examples and run the optimization output check with:

```powershell
ctest --test-dir build --output-on-failure
```

The [examples](examples) directory contains focused programs for expressions, functions, arrays, pointers, control flow, keyboard input, and optimization.

## 10. Current Boundaries

This is a subset of C. It does not implement a general preprocessor, local stack frames, recursion, signed arithmetic, 16-bit integer arithmetic, pointer arithmetic, dynamic addresses for `peek`/`poke`, array initialization, or runtime array bounds checks. Preprocessing is limited to quoted `#include`, `#pragma once`, and numeric object-like `#define`. Unsupported syntax is reported as a compile error.