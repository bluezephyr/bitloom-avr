cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 99)

add_library(i2c
    i2c.c
    )

target_include_directories(i2c PRIVATE ${BITLOOM_CORE}/include)
