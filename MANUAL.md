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

Local names follow block scope: nested blocks can shadow outer locals, and a `for`-initializer variable is only visible within its loop. Storage is still static: variables, parameters, arrays, and compiler temporaries do not use stack frames. Functions are therefore not reentrant or recursive. Initializers on local scalar variables are emitted as code at the declaration point. See [examples/scopes.c](examples/scopes.c).

### Memory-Bound Variables

Use `__mem(address)` before a file-scope byte variable or fixed-size array to bind its name to existing memory:

```c
__mem(0xD020) volatile unsigned char border_color;
__mem(0x0400) volatile unsigned char screen_page[256];

void main() {
    border_color = 2;
    screen_page[0] = 1;
}
```

These declarations emit no data bytes into the PRG. Reads and writes compile to direct absolute-address instructions; indexed arrays use absolute,X. The current array index is 8-bit, so memory arrays may contain at most 256 bytes. For larger areas, declare multiple non-overlapping windows. `include/c64.h` provides `c64_screen_ram*` and `c64_color_ram*` windows plus common VIC-II, sprite, SID, and CIA registers. `volatile` is accepted as a qualifier; memory-bound accesses are always emitted as hardware accesses. See [examples/memory_variables.c](examples/memory_variables.c), [examples/memory_screen.c](examples/memory_screen.c), [examples/memory_sprite.c](examples/memory_sprite.c), and [examples/memory_sid.c](examples/memory_sid.c).

`__mem` is different from [`__at`](#fixed-memory-placement): it aliases memory already present on the C64 and does not initialize it. The compiler does not reserve or initialize the bound region, so select addresses that are appropriate for the C64 memory map. `peek` and `poke` remain useful for one-off accesses to constant addresses.

### Fixed Memory Placement

Use `__at(address)` immediately before a global object or function definition to choose its address:

```c
__at(0x2000) unsigned char sprite_data[63];

__at(0x1000) void set_color() {
    set_border(2);
}
```

Unannotated code and data continue from the current automatic location. One object is allowed per placed global declaration. Function prototypes cannot carry `__at`; put the attribute on the definition. Placed addresses must be `$080D` or higher because `$0801-$080C` contains the PRG's BASIC loader stub.

The PRG format is contiguous, so gaps before placed regions are zero-filled. The compiler rejects intersections among emitted code/data regions and with the BASIC stub, reporting the first conflicting address. These checks only cover memory occupied by this compilation; they do not know about memory used by the C64 ROM, hardware, another program, or regions your program reserves for its own purposes. Large gaps also make the PRG larger and will overwrite the gap when loaded. Choose addresses with the C64 memory map in mind. See [examples/placement.c](examples/placement.c).

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

## 8. C64 Library

The C64 support is split into focused headers. [include/c64.h](include/c64.h) provides numeric addresses and `__mem` aliases only. Add the implementation header(s) you use; their helper functions are ordinary C definitions compiled into the program, not compiler-specific call cases. Project examples use source-relative includes; external projects can use `-I include`.

| Header | Contents |
|---|---|
| `c64.h` | Screen/color RAM windows and VIC-II, sprite, SID, and CIA register aliases |
| `c64_screen.h` | Screen/color writes, `key`, joystick port 2, and keyboard matrix scanning |
| `c64_vic.h` | Sprite helpers, raster/frame waits, and CIA1 Timer A helpers |
| `c64_sid.h` | SID volume, voice frequency, ADSR, and control helpers |

The common addresses include `C64_SCREEN_RAM`, `C64_COLOR_RAM`, `C64_BORDER_COLOR`, `C64_BACKGROUND_COLOR`, `C64_VIC_RASTER`, `C64_VIC_SPRITE_ENABLE`, `C64_VIC_SPRITE_POINTERS`, `C64_SPRITE_BITMAP`, `C64_SID_BASE`, and the CIA base addresses. `peek(address)` and `poke(address, value)` remain compiler intrinsics for one-off constant-address accesses; `print(address, "text")` is still an intrinsic as well.

`set_border(value)` and `set_background(value)` mask the value to the VIC-II four-bit color range. `screen_put(offset, code, color)` writes to the first 256-byte screen/color RAM windows; `screen_put_row0(column, code, color)` restricts the offset to the first 40 columns. The current byte index means larger screen areas are addressed through the explicit page aliases in `c64.h`.

`key()` wraps the KERNAL `GETIN` call at `$FFE4` and returns zero when no key is available. `joystick_port2()` reads CIA1 port B and inverts its active-low bits. `keyboard_matrix_scan()` scans all 64 CIA1 keyboard intersections and returns the first pressed key in scan order as `row * 8 + column + 1`; zero means no key. It returns a matrix coordinate, not ASCII or a C64 key code. It temporarily configures CIA1 port A as output and port B as input, then restores their previous data and direction values. Simultaneous key presses, keyboard ghosting, and debounce handling are not resolved by this simple first-key scanner.

`vic_wait_raster(line)` waits for the VIC-II raster register to equal a low-byte line value (0..255). `vic_wait_frame()` waits for a raster wrap near line 250, and `vic_wait_frames(count)` repeats that wait. These are frame-synchronized busy waits, not cycle-exact delays; PAL and NTSC frame rates differ.

`cia1_timer_a_start(low, high)` loads and starts CIA1 Timer A in continuous mode. The latch is a 16-bit phi2-cycle count, so elapsed time depends on the machine's PAL/NTSC clock. `cia1_timer_a_read_high()` reads the counter's high byte; `cia1_timer_a_stop()` stops it. Keep this timer usage separate from other code that owns CIA1 Timer A.

`print(address, "text")` writes supported characters to screen memory as C64 screen codes. It remains a compiler intrinsic.

`print` maps letters to screen codes 1 through 26. Other accepted characters use their byte values for codes 32 through 63. For other hardware access, use `peek`, `poke`, or a pointer.

### Sprites

The sprite helper functions accept a sprite number from 0 through 7:

| Helper | Behavior |
|---|---|
| `sprite_enable(sprite, enabled)` | Set or clear the sprite's bit in `$D015` |
| `sprite_position(sprite, x, y)` | Write the low 8 bits of X and the Y coordinate |
| `sprite_x_msb(sprite, enabled)` | Set or clear the ninth X-coordinate bit in `$D010` |
| `sprite_color(sprite, color)` | Set the sprite's individual color |
| `sprite_multicolor(sprite, enabled)` | Set or clear the sprite's multicolor bit in `$D01C` |
| `sprite_pointer(sprite, pointer)` | Write the bitmap pointer in the default screen's pointer table at `$07F8` |
| `sprite_bitmap_clear()` | Clear 63 bitmap bytes beginning at `$2000` |
| `sprite_bitmap_write(offset, value)` | Write a byte at `$2000 + offset`; keep offset in 0..62 |

The sprite helpers use memory-mapped aliases in `c64.h`. Bitmap helpers assume VIC-II bank 0, screen RAM at `$0400`, and bitmap storage at `$2000`; the pointer table is then at `$07F8`. `sprite_pointer(0, 0x80)` selects `$2000` because each bitmap occupies 64 bytes. A bitmap uses 63 data bytes; the final byte is reserved. The X coordinate's high bit is managed separately. No helper configures VIC bank or screen placement automatically. See [examples/memory_sprite.c](examples/memory_sprite.c) and [examples/sprites.c](examples/sprites.c).

### SID

`sid_set_volume` takes a four-bit volume. The voice helpers accept voice indices 0 through 2 and write the standard seven-register voice blocks beginning at `$D400`; frequency is supplied as low/high bytes. See [examples/memory_sid.c](examples/memory_sid.c) and [examples/sid_tone.c](examples/sid_tone.c).

## 9. Output and Tests

The generated PRG loads at `$0801`. Its BASIC stub calls the machine code at `$080D` (`SYS 2061`); the startup code calls `main` and then parks the CPU. `__at` can place code/data later in the address space; the encoder pads the single PRG image across gaps and rejects overlaps or output beyond the 6502 address space.

Compile the examples and run the optimization output check with:

```powershell
ctest --test-dir build --output-on-failure
```

The [examples](examples) directory contains focused programs for expressions, functions, arrays, pointers, control flow, screen memory, sprites, SID, and optimization.

Keyboard and timing demonstrations: [examples/keyboard_matrix.c](examples/keyboard_matrix.c) and [examples/timing.c](examples/timing.c).

## 10. Current Boundaries

This is a subset of C. It does not implement a general preprocessor, stack-based local frames, recursion, reentrant calls, signed arithmetic, 16-bit integer arithmetic, pointer arithmetic, dynamic addresses for `peek`/`poke`, array initialization, or runtime array bounds checks. Placement is limited to global objects and function definitions at or above `$080D`; automatic layout and system/hardware memory reservations are not modeled. Preprocessing is limited to quoted `#include`, `#pragma once`, and numeric object-like `#define`. Unsupported syntax is reported as a compile error.