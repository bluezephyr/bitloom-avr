/*
 * Implementation of the timer module for AVR.
 *
 * Copyright (c) 2018. BlueZephyr
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include "core/scheduler.h"

/*
 * Timer 0 is used to schedule the tasks.
 *
 * An interrupt is generated every ms and the scheduler timer tick function is
 * called.
 */
void timer_init (void)
{
    // Set CPU clock frequency according to value in the Makefile
#if F_CPU == 8000000
    // Set CPU clock speed to 8MHz
    clock_prescale_set (clock_div_1);
#endif

    // CTC mode (page 98, 104 and 106 in the datasheet).
    TCCR0A = (1 << WGM01);

    // Set interrupt on compare match (page 109).
    TIMSK0 = (1 << OCIE0A);

    // Set prescaler and output compare register to generate a tick every ms
    // Prescaler clk/8 and 125 time ticks @1 MHz -> 1ms
    // Prescaler clk/64 and 125 time ticks @8 MHz -> 1ms
    // (Page 108 in the datasheet.)
    OCR0A = 124;

#if F_CPU == 1000000
    TCCR0B = (1 << CS01);
#elif F_CPU == 8000000
    TCCR0B = (1 << CS01) | (1 << CS00);
#endif
}

/*
 * Interrupt routine to update the task timers.
 */
ISR(TIMER0_COMPA_vect)
{
    schedule_timer_tick();
}

void timer_start (void)
{
    sei();
}

void timer_stop (void)
{
}
