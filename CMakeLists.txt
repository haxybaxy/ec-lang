cmake_minimum_required(VERSION 3.0.0)
project(eclang) # VERSION 0.0.0-20211225

add_executable(eclang
  src/chunk.c
        src/compiler.c
        src/main.c
  src/memory.c
  src/object.c
        src/scanner.c
        src/helper.c
  src/value.c
        src/vm.c
)
