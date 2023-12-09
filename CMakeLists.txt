cmake_minimum_required(VERSION 3.26)
project(ec_lang C)

set(CMAKE_C_STANDARD 11)

add_executable(ec_lang
        src/main.c
        src/include/main.h)
