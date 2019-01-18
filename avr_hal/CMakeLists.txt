cmake_minimum_required(VERSION 3.12)
project(scheduler C)

set(CMAKE_C_STANDARD 99)

add_library(hal
        src/avr_i2c.c
        src/avr_pin_digital_io.c
        src/avr_timer.c)

target_include_directories(hal PRIVATE ${BITLOOM_ROOT}/bitloom-core/include/hal)
target_include_directories(hal PRIVATE ${BITLOOM_ROOT}/bitloom-core/modules/scheduler/include)
