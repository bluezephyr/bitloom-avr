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
    stop,
    send_sla_w,
    write_data,
    write_register
} i2c_state_t;

static struct i2c_controller_t
{
    i2c_operation_t operation;
    uint8_t slave_address;
    i2c_operation_status_t operation_status;
    i2c_state_t state;
    uint8_t* data;
    uint16_t data_len;
    uint16_t handled_bytes;
} self;


static inline void i2c_send_start (void);
static inline void i2c_write_byte (uint8_t byte);
static inline void i2c_send_stop (void);


void i2c_init (void)
{
    self.state = idle;
    self.slave_address = 0;
    self.operation_status = i2c_ok;

    // TWBR – TWI Bit Rate Register
    // Datasheet page 230
    TWBR = 32;  // 50 kHz
    TWCR |= (1 << TWEN);
}

void i2c_write (uint8_t address, uint8_t *data, uint16_t data_len)
{
    if (self.state == idle)
    {
        // Store the input data
        self.slave_address = address;
        self.data = data;
        self.data_len = data_len;

        // Prepare write operation - will be performed in ISR context
        self.operation = write_op;
        self.operation_status = i2c_busy;
        self.state = start;
        self.handled_bytes = 0;
        i2c_send_start();
    }
}

void i2c_read_register (uint8_t slave_id, uint8_t reg, uint8_t *data)
{

}

i2c_operation_status_t i2c_get_operation_status(void)
{
    return self.operation_status;
}

static inline void i2c_send_start (void)
{
    // Datasheet page 217 (v 1/2015)
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);
}

static inline void i2c_write_byte (uint8_t byte)
{
    TWDR = byte;
    TWCR = (1 << TWINT) | (1 << TWEN);
}

i2c_operation_status_t i2c_restart (void)
{
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTA);

    //i2c_wait_for_complete();
    if (TW_STATUS != TW_REP_START)
    {
        return i2c_operation_error;
    }
    return i2c_ok;
}

static inline void i2c_send_stop (void)
{
    TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

// TODO: Change to ISR
void i2c_callback_TWINT(void)
{
    switch (self.state)
    {
        case idle:
            // Error - cannot happen
            break;

        case start:
            // Verify that the operation succeeded (TW_START)
            if (TW_STATUS != TW_START)
            {
                self.operation_status = i2c_start_error;
                return;
            }

            // Send slave address
            i2c_write_byte(self.slave_address);
            break;

        case send_sla_w:
            // TODO: Compare with valid values and handle errors outside the
            // switch statement?
            if (TW_STATUS != TW_MT_SLA_ACK)
            {
                self.operation_status = i2c_sla_error;
                return;
            }

            if (self.operation == write_op)
            {
                self.state = write_data;
                i2c_write_byte(self.data[self.handled_bytes++]);
            }
            else
            {
                self.state = write_register;
            }

        case write_data:
            if (TW_STATUS != TW_MT_DATA_ACK)
            {
                self.operation_status = i2c_write_error;
                return;
            }
            if (self.handled_bytes < self.data_len)
            {
                i2c_write_byte(self.data[self.handled_bytes++]);
            }
            else
            {
                // All bytes written -- stop
                i2c_send_stop();
                self.operation_status = i2c_ok;
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

        case stop:
            if (TW_STATUS != TW_SR_STOP)
                while (!(TWCR & (1 << TWSTO)));
            break;
    }
}
