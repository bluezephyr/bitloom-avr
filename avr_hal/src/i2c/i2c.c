/*
 * Implementation of the I2C module for AVR.
 *
 * Copyright (c) 2018-2020. BlueZephyr
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 */

#include "hal/i2c.h"
#include <util/twi.h>
#include <avr/interrupt.h>
#include <stddef.h>

/*
 * The TWINT Flag is set in the following situations:
 * - After the TWI has transmitted a START/REPEATED START condition.
 * - After the TWI has transmitted SLA+R/W.
 * - After the TWI has transmitted an address byte.
 * - After the TWI has lost arbitration.
 * - After the TWI has been addressed by own slave address or general call.
 * - After the TWI has received a data byte.
 * - After a STOP or REPEATED START has been received while still addressed as a Slave.
 * - When a bus error has occurred due to an illegal START or STOP condition.
 *
 * The flag is cleared by writing a 1 to it.
 */

/*
 * Type of operation that the application has requested.
 */
typedef enum
{
    read_op,
    read_register_op,
    write_op
} i2c_operation_t;

/*
 * Current operation state.
 */
typedef enum
{
    idle,
    start,
    send_sla_w,
    write_data,
    write_register
} i2c_state_t;

static struct i2c_controller_t
{
    i2c_operation_t operation;
    uint8_t slave_address;
    uint8_t read_register;
    i2c_state_t state;
    uint8_t* buffer;
    uint8_t buffer_length;
    uint8_t handled_bytes;
    enum i2c_op_result_t *operation_result;
    uint8_t operation_status_code;
} self;

/*
 * Local function declarations
 */
static inline void i2c_send_start (void);
static inline void i2c_write_byte (uint8_t byte);
static inline void i2c_send_stop (void);


void i2c_init (void)
{
    self.state = idle;
    self.slave_address = 0;
    self.read_register = 0;
    self.operation_result = NULL;
    self.operation_status_code = 0;

    // TWBR – TWI Bit Rate Register
    // Datasheet page 230
    TWBR = 32;  // 50 kHz
    TWCR |= (1 << TWEN);
}

enum i2c_request_t i2c_write (uint8_t address, uint8_t *buffer, uint8_t length, enum i2c_op_result_t *result)
{
    if (self.state == idle)
    {
        // Store the input data
        self.slave_address = address;
        self.buffer = buffer;
        self.buffer_length = length;

        // Prepare write operation - will be performed in ISR context
        self.operation = write_op;
        self.operation_result = result;
        *self.operation_result = i2c_operation_processing;
        self.state = start;
        self.handled_bytes = 0;
        i2c_send_start();

        return i2c_request_ok;
    }
    return i2c_request_busy;
}

enum i2c_request_t i2c_read_register(uint8_t address, uint8_t read_register, uint8_t *buffer,
                                     uint8_t length, enum i2c_op_result_t *result)
{
    if (self.state == idle)
    {
        self.slave_address = address;
        self.read_register = read_register;
        self.buffer = buffer;
        self.buffer_length = length;

        // Prepare read register operation. Will be started next tick
        self.operation = read_register_op;
        self.operation_result = result;
        *self.operation_result = i2c_operation_processing;
        self.state = start;
        self.handled_bytes = 0;
        i2c_send_start();

        return i2c_request_ok;
    }
    return i2c_request_busy;
}

uint8_t i2c_get_error_code(void)
{
    return self.operation_status_code;
}

static inline void i2c_send_start (void)
{
    // Datasheet page 217 (v 1/2015)
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA) | (1 << TWIE);
}

static inline void i2c_write_byte (uint8_t byte)
{
    TWDR = byte;
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
}

enum i2c_op_result_t i2c_restart (void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    //i2c_wait_for_complete();
    if (TW_STATUS != TW_REP_START)
    {
        return i2c_operation_error;
    }
    return i2c_operation_ok;
}

static inline void i2c_send_stop (void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

/*
 * The main processing of the TWI communication is done in interrupt context.
 */
ISR(TWI_vect)
{
    switch (self.state)
    {
        case start:
            if (TW_STATUS == TW_START)
            {
                self.state = send_sla_w;
                i2c_write_byte(self.slave_address);
            }
            else
            {
                *self.operation_result = i2c_operation_start_error;
                self.operation_status_code = TW_STATUS;
            }
            // Stop shall not be sent
            return;

        case send_sla_w:
            if (TW_STATUS == TW_MT_SLA_ACK)
            {
                if (self.operation == write_op)
                {
                    self.state = write_data;
                    i2c_write_byte(self.buffer[self.handled_bytes++]);
                }
                else
                {
                    self.state = write_register;
                }
                return;
            }
            else
            {
                // Stop and return error
                *self.operation_result = i2c_operation_sla_error;
                self.operation_status_code = TW_STATUS;
            }
            break;

        case write_data:
            if (TW_STATUS == TW_MT_DATA_ACK)
            {
                if (self.handled_bytes < self.buffer_length)
                {
                    i2c_write_byte(self.buffer[self.handled_bytes++]);
                    return;
                }
                else
                {
                    // All bytes written -- stop and return ok
                    *self.operation_result = i2c_operation_ok;
                }
            }
            else
            {
                // Stop and return error
                *self.operation_result = i2c_operation_write_error;
                self.operation_status_code = TW_STATUS;
            }
            break;

        case write_register:
//            switch (TW_STATUS)
//            {
//                case TW_MT_DATA_ACK:
//                    return i2c_ack_received;
//                case TW_MT_SLA_NACK:
//                case TW_MT_DATA_NACK:
//                    return i2c_nack_received;
//                case TW_MT_ARB_LOST:
//                    return i2c_arbitration_lost;
//                default:
//                    return i2c_operation_error;
//            }
            break;
    }

    i2c_send_stop();
}
