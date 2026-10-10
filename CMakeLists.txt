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
