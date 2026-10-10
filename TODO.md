# c64cc Roadmap

## 1. Runtime Verification

- Add VICE-backed checks for program behavior, not only successful compilation.
- Verify the sprite demo is visible, correctly positioned, and moves at a useful speed.
- Add focused checks for keyboard input, screen/color writes, and SID behavior where automation permits.

## 2. C Language Semantics

- [x] Add lexical block scope, nested shadowing, and `for`-initializer scope.
- [ ] Move locals, parameters, and temporaries to stack-based frames.
- Define a reentrant calling convention and support recursion.
- Add signed byte values and true 16-bit integer arithmetic.
- Expand `for` initializer/update clauses toward ordinary C expressions.

## 3. Memory and Arrays

- [x] Add C-level fixed placement for global objects and functions with emitted-range overlap checks.
- Add pointer arithmetic and address-of expressions.
- Support array-to-pointer conversion and array initializer lists.
- Add runtime or optional bounds checking for arrays.
- Allow `peek` and `poke` to use variable addresses and pointer expressions.

## 4. C64 Library

- Move beyond compiler built-ins toward reusable helpers for screen output, keyboard scanning, VIC-II, and SID.
- Keep hardware register addresses and helper declarations in `include/c64.h` or focused headers.
- Document VIC bank, bitmap, sprite pointer table, and memory-layout assumptions.

## 5. Preprocessing and Optimization

- Extend the limited include/numeric-define expander only as needed; consider function-like macros and conditional compilation.
- Add more optimization passes with assembly-level regression checks.
- Measure generated code size and runtime cost for arithmetic and common C64 operations.

## 6. Diagnostics and Tests

- Add negative tests for malformed syntax, invalid types/operands, duplicate declarations, and unsupported directives.
- Improve diagnostics with source file and line information, especially across included headers.
- Add tests for branch ranges, address-space overflow, pointer ranges, and array boundaries.