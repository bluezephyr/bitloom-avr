/*
 * Implementation of the I2C module for AVR.
 *
 * Copyright (c) 2018. BlueZephyr
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 */

#include "i2c.h"
#include <util/twi.h>

static void inline i2c_wait_for_complete (void)
{
    while (!(TWCR & (1 << TWINT)));
}

void i2c_init (void)
{
    // TWBR – TWI Bit Rate Register
    // Datasheet page 230
    TWBR = 32;  // 50 kHz
    TWCR |= (1 << TWEN);
}

i2c_result_t i2c_start (void)
{
    // Datasheet page 220
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
    i2c_wait_for_complete();

    if (TW_STATUS != TW_START)
    {
        return i2c_operation_error;
    }
    return i2c_ok;
}

i2c_result_t i2c_restart (void)
{
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
    i2c_wait_for_complete();
    if (TW_STATUS != TW_REP_START)
    {
        return i2c_operation_error;
    }
    return i2c_ok;
}

void i2c_stop (void)
{
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (!(TWCR & (1 << TWSTO)));
}

i2c_result_t i2c_write_byte (uint8_t byte)
{
    TWDR = byte;
    TWCR = (1 << TWINT) | (1 << TWEN);
    i2c_wait_for_complete();

    switch (TW_STATUS)
    {
        case TW_MT_SLA_ACK:
        case TW_MT_DATA_ACK:
            return i2c_ack_received;
        case TW_MT_SLA_NACK:
        case TW_MT_DATA_NACK:
            return i2c_nack_received;
        case TW_MT_ARB_LOST:
            return arbitration_lost_error;
        default:
            return operation_error;
    }
}

uint8_t i2c_read_byte (uint8_t send_ack)
{
    // Not implemented
    return operation_error;
}
