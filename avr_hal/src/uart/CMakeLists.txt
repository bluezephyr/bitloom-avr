cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 99)

add_library(uart
        uart.c
        )

target_include_directories(uart PRIVATE ${BITLOOM_CORE}/include)
target_include_directories(uart PRIVATE ${CUTIL}/include)
