/*
 * Implementation of the PIN digital IO module for AVR.
 *
 * Copyright (c) 2018. BlueZephyr
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

#include "port_config.h"

void pin_digital_io_write(uint16_t pin_id, bool on)
{
}

void pin_digital_io_write_high(uint16_t pin_id)
{
    LED_PORT |= (1 << pin_id);
}

void pin_digital_io_write_low(uint16_t pin_id)
{
    LED_PORT &= ~(1 << pin_id);
}

