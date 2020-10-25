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
#include "hal/uart_hal.h"

typedef struct
{
    bytebuffer_t *inBuffer;
    bytebuffer_t *outBuffer;
} uart_hal_t;
static uart_hal_t self;

void uart_hal_init(bytebuffer_t *inBuffer, bytebuffer_t *outBuffer)
{
    self.inBuffer = inBuffer;
    self.outBuffer = outBuffer;

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

void uart_hal_send(void)
{
    // Enable USART Data Register Empty Interrupt Enable
    // The interrupt will be enabled until all data in the out buffer
    // has been transmitted.
    UCSR0B |= (1 << UDRIE0);
}

ISR(USART_RX_vect)
{
    uint8_t data = UDR0;

    if (!bytebuffer_isFull(self.inBuffer))
    {
        bytebuffer_write(self.inBuffer, data);
    }
}

ISR(USART_UDRE_vect)
{
    if (bytebuffer_isEmpty(self.outBuffer))
    {
        // Disable USART Data Register Empty Interrupt Enable
        UCSR0B &= ~(1 << UDRIE0);
    }
    else
    {
        UDR0 = bytebuffer_read(self.outBuffer);
    }
}

