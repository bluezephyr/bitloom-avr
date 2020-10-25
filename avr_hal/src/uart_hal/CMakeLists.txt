cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 99)

add_library(uart_hal
        uart_hal.c
        )

target_include_directories(uart_hal PRIVATE ${BITLOOM_CORE}/include)
target_include_directories(uart_hal PRIVATE ${CUTIL}/include)
