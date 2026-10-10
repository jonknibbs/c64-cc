cmake_minimum_required(VERSION 3.20)
project(c64cc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(c64cc
  src/main.c
  src/lexer.c
  src/compiler.c
  src/emit.c
)

if(MSVC)
  target_compile_options(c64cc PRIVATE /W4)
else()
  target_compile_options(c64cc PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
foreach(example arrays border colors control_flow expressions functions hello input keyboard_matrix library_helpers memory_screen memory_sid memory_sprite memory_variables optimization pointers sid_tone timing)
  add_test(
    NAME compile_${example}
    COMMAND c64cc
      ${CMAKE_CURRENT_SOURCE_DIR}/examples/${example}.c
      -o ${CMAKE_CURRENT_BINARY_DIR}/${example}-test.prg
  )
endforeach()

add_test(
  NAME verify_c64_library_functions
  COMMAND ${CMAKE_COMMAND}
    -DCOMPILER=$<TARGET_FILE:c64cc>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/library_helpers.c
    -DPRG=${CMAKE_CURRENT_BINARY_DIR}/library-helpers.prg
    -DASM=${CMAKE_CURRENT_BINARY_DIR}/library-helpers.asm
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_c64_library.cmake
)

add_test(
  NAME verify_memory_variables
  COMMAND ${CMAKE_COMMAND}
    -DCOMPILER=$<TARGET_FILE:c64cc>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/memory_variables.c
    -DPRG=${CMAKE_CURRENT_BINARY_DIR}/memory-variables.prg
    -DASM=${CMAKE_CURRENT_BINARY_DIR}/memory-variables.asm
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_memory_variables.cmake
)

add_test(
  NAME compile_c64_library
  COMMAND c64cc
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/c64_library.c
    -I ${CMAKE_CURRENT_SOURCE_DIR}/include
    -o ${CMAKE_CURRENT_BINARY_DIR}/c64_library-test.prg
)

add_test(
  NAME compile_sprites
  COMMAND c64cc
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/sprites.c
    -o ${CMAKE_CURRENT_BINARY_DIR}/sprites-test.prg
)

add_test(NAME cli_help COMMAND c64cc --help)
add_test(NAME cli_version COMMAND c64cc --version)
add_test(
  NAME optimize_constant_expressions
  COMMAND ${CMAKE_COMMAND}
    -DCOMPILER=$<TARGET_FILE:c64cc>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/examples/optimization.c
    -DPRG=${CMAKE_CURRENT_BINARY_DIR}/optimization-check.prg
    -DASM=${CMAKE_CURRENT_BINARY_DIR}/optimization-check.asm
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_optimization.cmake
)

add_test(
  NAME asm_source_comments
  COMMAND ${CMAKE_COMMAND}
    -DCOMPILER=$<TARGET_FILE:c64cc>
    -DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/tests/comments.c
    -DPRG=${CMAKE_CURRENT_BINARY_DIR}/comments-test.prg
    -DASM=${CMAKE_CURRENT_BINARY_DIR}/comments-test.asm
    -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_asm_comments.cmake
)
