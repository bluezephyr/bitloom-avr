find_program(AVR_CC avr-gcc)
find_program(AVR_SIZE avr-size)
find_program(AVR_OBJCOPY avr-objcopy)
find_program(AVR_OBJDUMP avr-objdump)
find_program(AVRDUDE avrdude)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_C_COMPILER ${AVR_CC})

set(MCU atmega328p)
set(F_CPU 8000000)
set(BAUD 9600)
#set(AVR_PROGRAMMER usbtiny)
set(AVR_PROGRAMMER avrisp)
set(AVR_PROGRAMMER_ARGS -b 19200)
set(AVR_PROGRAMMER_PORT -P /dev/ttyACM0)


set(CMAKE_C_FLAGS "-mmcu=${MCU} -DF_CPU=${F_CPU}UL -DBAUD=${BAUD}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-Map,mapfile.map")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -Wall -Wstrict-prototypes -g -ggdb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--relax")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")

set(TOOLCHAIN_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BITLOOM_HAL ${TOOLCHAIN_DIR}/avr_hal )

# Print configuration
message( STATUS "Toolchain CMakefile directory: ${TOOLCHAIN_DIR}")
message( STATUS "AVR MCU: ${MCU}" )
message( STATUS "CPU Frequency: ${F_CPU} Hz" )
message( STATUS "BAUD Rate: ${BAUD}" )
message( STATUS "AVR Programmer: ${AVR_PROGRAMMER}" )

# Cross-compile version of the add_executable command
function(cc_add_executable NAME)
    if(NOT ARGN)
        message(FATAL_ERROR "No source files for ${NAME}.")
    endif(NOT ARGN)

    set(elf_file ${NAME}.elf)
    set(hex_file ${NAME}.hex)
    set(map_file ${NAME}.map)
    set(eeprom_image ${NAME}-eeprom.hex)

    # elf file
    add_executable(${elf_file} EXCLUDE_FROM_ALL ${ARGN})
    add_custom_target(${NAME} ALL DEPENDS ${hex_file})

    # Save the original name to be used in the cc_target_include_directories
    # and cc_target_link_libraries function calls
    set_target_properties( ${NAME} PROPERTIES OUTPUT_NAME "${elf_file}" )

    add_custom_command(
            OUTPUT ${hex_file}
            COMMAND ${AVR_OBJCOPY} -R .eeprom -O ihex ${elf_file} ${hex_file}
            DEPENDS ${elf_file}
            VERBATIM
    )

    # Flash target
    add_custom_target(
            flash
            COMMAND ${AVRDUDE} -p ${MCU} -c ${AVR_PROGRAMMER} ${AVR_PROGRAMMER_ARGS} ${AVR_PROGRAMMER_PORT} -U flash:w:${hex_file}
            DEPENDS ${hex_file}
            COMMENT "Flashing ${hex_file} to ${MCU} using ${AVR_PROGRAMMER}"
            VERBATIM
    )

    # Assembler listing
    add_custom_target(
            list
            COMMAND ${AVR_OBJDUMP} -h -S ${elf_file} > ${NAME}.lst
            DEPENDS ${elf_file}
            COMMENT "Creating assembler listing in ${NAME}.lst based on ${elf_file}"
            VERBATIM
    )

    # Size
    add_custom_target(
            size
            COMMAND ${AVR_SIZE} -C --mcu=${MCU} ${elf_file}
            DEPENDS ${elf_file}
            COMMENT "Size of the binary ${elf_file}"
            VERBATIM
    )

endfunction(cc_add_executable)


# Cross-compile version of the target_include_directories command
function(cc_target_include_directories NAME)
    get_target_property(TARGET ${NAME} OUTPUT_NAME)
    target_include_directories(${TARGET} ${ARGN})
endfunction(cc_target_include_directories)


# Cross-compile version of the target_link_libraries command
function(cc_target_link_libraries NAME)
    get_target_property(TARGET ${NAME} OUTPUT_NAME)
    target_link_libraries(${TARGET} ${ARGN})
endfunction(cc_target_link_libraries)

