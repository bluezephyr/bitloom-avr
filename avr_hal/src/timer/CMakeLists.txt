cmake_minimum_required(VERSION 3.12)
project(timer C)

set(CMAKE_C_STANDARD 99)

add_library(timer
        timer.c
        )

target_include_directories(timer PRIVATE ${BITLOOM_CORE}/include)
