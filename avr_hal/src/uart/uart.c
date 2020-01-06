/*
 * Implementation of the UART module for AVR.
 *
 * Copyright (c) 2020. BlueZephyr
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>
#include "hal/uart.h"

typedef struct
{
    uint8_t inBuffer;
    uint8_t inLen;
} uart_t;
static uart_t self;

void uart_init (void)
{
    self.inBuffer = 0;
    self.inLen = 0;

    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif
    UCSR0B = (1 << TXEN0) | (1 << RXEN0); // Enable Rx and Tx
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit

    UCSR0B |= (1 << RXCIE0); // Enable RX Complete Interrupt
}

int8_t uart_read (uint8_t* buffer, int8_t nbyte)
{
    (void)nbyte;
    if (self.inLen != 0)
    {
        *buffer = self.inBuffer;
        self.inLen = 0;
        return 1;
    }

    return 0;
}

ISR(USART_RX_vect)
{
    self.inLen = 1;
    self.inBuffer = UDR0;
}

