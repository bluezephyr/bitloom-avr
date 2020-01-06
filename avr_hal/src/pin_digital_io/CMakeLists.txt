cmake_minimum_required(VERSION 3.12)
project(pin_digital_io C)

set(CMAKE_C_STANDARD 99)

add_library(pin_digital_io
        pin_digital_io.c
        )

target_include_directories(pin_digital_io PRIVATE ${BITLOOM_CORE}/include)
