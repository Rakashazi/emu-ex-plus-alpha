/*
 * ted-timer.c - Timer implementation for the TED emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "alarm.h"
#include "log.h"
#include "maincpu.h"
#include "ted-irq.h"
#include "ted-timer.h"
#include "tedtypes.h"
#include "types.h"


/*#define DEBUG_TIMER*/


static alarm_t *ted_t1_alarm = NULL;
static alarm_t *ted_t2_alarm = NULL;
static alarm_t *ted_t3_alarm = NULL;

static unsigned int t1_start;
static unsigned int t2_start;
static unsigned int t3_start;

static unsigned int t1_last_restart;
static unsigned int t2_last_restart;
static unsigned int t3_last_restart;


static unsigned int t1_running;
static unsigned int t2_running;
static unsigned int t3_running;


static unsigned int t1_value;
static unsigned int t2_value;
static unsigned int t3_value;

/*-----------------------------------------------------------------------*/

static void ted_t1(CLOCK offset, void *data)
{
    alarm_set(ted_t1_alarm, maincpu_clk
              + (t1_start == 0 ? 65536 : t1_start) * 2 - offset);
    t1_value = (t1_start == 0 ? 65536 : t1_start) * 2 - offset;
#ifdef DEBUG_TIMER
    log_debug("TI1 ALARM %x", maincpu_clk);
#endif
    ted_irq_timer1_set();
    t1_last_restart = maincpu_clk - offset;
}

static void ted_t2(CLOCK offset, void *data)
{
    alarm_set(ted_t2_alarm, maincpu_clk + 65536 * 2 - offset);
    t2_start = 0;
    t2_value = 65536 * 2 - offset;
#ifdef DEBUG_TIMER
    log_debug("TI2 ALARM %x", maincpu_clk);
#endif
    ted_irq_timer2_set();
    t2_last_restart = maincpu_clk - offset;
}

static void ted_t3(CLOCK offset, void *data)
{
    alarm_set(ted_t3_alarm, maincpu_clk + 65536 * 2 - offset);
    t3_start = 0;
    t3_value = 65536 * 2 - offset;
#ifdef DEBUG_TIMER
    log_debug("TI3 ALARM %x", maincpu_clk);
#endif
    ted_irq_timer3_set();
    t3_last_restart = maincpu_clk - offset;
}

/*-----------------------------------------------------------------------*/

static void ted_timer_t1_store_low(BYTE value)
{
    alarm_unset(ted_t1_alarm);
    if (t1_running) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    t1_value = (t1_start = (t1_start & 0xff00) | value) << 1;
    t1_running = 0;
}

static void ted_timer_t1_store_high(BYTE value)
{
    alarm_unset(ted_t1_alarm);
    t1_value = (t1_start = (t1_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t1_alarm, maincpu_clk
              + (t1_start == 0 ? 65536 : t1_start) * 2);
    t1_last_restart = maincpu_clk;
    t1_running = 1;
}

static void ted_timer_t2_store_low(BYTE value)
{
    alarm_unset(ted_t2_alarm);
    t2_value = (t2_start = (t2_start & 0xff00) | value) << 1;
    t2_running = 0;
}

static void ted_timer_t2_store_high(BYTE value)
{
    alarm_unset(ted_t2_alarm);
    t2_value = (t2_start = (t2_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t2_alarm, maincpu_clk
              + (t2_start == 0 ? 65536 : t2_start) * 2);
    t2_last_restart = maincpu_clk;
    t2_running = 1;
}

static void ted_timer_t3_store_low(BYTE value)
{
    alarm_unset(ted_t3_alarm);
    t3_value = (t3_start = (t3_start & 0xff00) | value) << 1;
    t3_running = 0;
}

static void ted_timer_t3_store_high(BYTE value)
{
    alarm_unset(ted_t3_alarm);
    t3_value = (t3_start = (t3_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t3_alarm, maincpu_clk
              + (t3_start == 0 ? 65536 : t3_start) * 2);
    t3_last_restart = maincpu_clk;
    t3_running = 1;
}

static BYTE ted_timer_t1_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI1 READL %02x", t1_value & 0xff);
#endif
    if (t1_running) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    return (BYTE)(t1_value >> 1);
}

static BYTE ted_timer_t1_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI1 READH %02x", t1_value >> 8);
#endif
    if (t1_running) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    return (BYTE)(t1_value >> 9);
}

static BYTE ted_timer_t2_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI2 READL %02x", t2_value & 0xff);
#endif
    if (t2_running) {
        t2_value -= maincpu_clk - t2_last_restart;
        t2_last_restart = maincpu_clk;
    }
    return (BYTE)(t2_value >> 1);
}

static BYTE ted_timer_t2_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI2 READH %02x", t2_value >> 8);
#endif
    if (t2_running) {
        t2_value -= maincpu_clk - t2_last_restart;
        t2_last_restart = maincpu_clk;
    }
    return (BYTE)(t2_value >> 9);
}

static BYTE ted_timer_t3_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI3 READL %02x", t3_value & 0xff);
#endif
    if (t3_running) {
        t3_value -= maincpu_clk - t3_last_restart;
        t3_last_restart = maincpu_clk;
    }
    return (BYTE)(t3_value >> 1);
}

static BYTE ted_timer_t3_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI3 READH %02x", t3_value >> 8);
#endif
    if (t3_running) {
        t3_value -= maincpu_clk - t3_last_restart;
        t3_last_restart = maincpu_clk;
    }
    return (BYTE)(t3_value >> 9);
}

/*-----------------------------------------------------------------------*/

void ted_timer_store(WORD addr, BYTE value)
{
#ifdef DEBUG_TIMER
    log_debug("TI STORE %02x %02x CLK %x", addr, value, maincpu_clk);
#endif
    switch (addr) {
        case 0:
            ted_timer_t1_store_low(value);
            break;
        case 1:
            ted_timer_t1_store_high(value);
            break;
        case 2:
            ted_timer_t2_store_low(value);
            break;
        case 3:
            ted_timer_t2_store_high(value);
            break;
        case 4:
            ted_timer_t3_store_low(value);
            break;
        case 5:
            ted_timer_t3_store_high(value);
            break;
    }
}

BYTE ted_timer_read(WORD addr)
{
    switch (addr) {
        case 0:
            return ted_timer_t1_read_low();
        case 1:
            return ted_timer_t1_read_high();
        case 2:
            return ted_timer_t2_read_low();
        case 3:
            return ted_timer_t2_read_high();
        case 4:
            return ted_timer_t3_read_low();
        case 5:
            return ted_timer_t3_read_high();
    }
    return 0;
}

void ted_timer_init(void)
{
    ted_t1_alarm = alarm_new(maincpu_alarm_context, "TED T1", ted_t1, NULL);
    ted_t2_alarm = alarm_new(maincpu_alarm_context, "TED T2", ted_t2, NULL);
    ted_t3_alarm = alarm_new(maincpu_alarm_context, "TED T3", ted_t3, NULL);
}

void ted_timer_reset(void)
{
    alarm_unset(ted_t1_alarm);
    alarm_unset(ted_t2_alarm);
    alarm_unset(ted_t3_alarm);
}
