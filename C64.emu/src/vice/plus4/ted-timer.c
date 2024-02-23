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
#include "ted.h"
#include "types.h"


/*#define DEBUG_TIMER*/


static alarm_t *ted_t1_alarm = NULL;
static alarm_t *ted_t2_alarm = NULL;
static alarm_t *ted_t3_alarm = NULL;

/* static CLOCK t1_start; */
static CLOCK t2_start;
static CLOCK t3_start;

static CLOCK t1_last_restart;
static CLOCK t2_last_restart;
static CLOCK t3_last_restart;

static CLOCK t1_value;
static CLOCK t2_value;
static CLOCK t3_value;

/*-----------------------------------------------------------------------*/

static void ted_t1(CLOCK offset, void *data)
{
    alarm_set(ted_t1_alarm, maincpu_clk
              + (ted.t1_start == 0 ? 65536 : ted.t1_start) * 2 - offset);
    t1_value = (ted.t1_start == 0 ? 65536 : ted.t1_start) * 2 - offset;
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

static void ted_timer_t1_store_low(uint8_t value)
{
    alarm_unset(ted_t1_alarm);
    if (ted.timer_running[0]) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    t1_value = (ted.t1_start = (ted.t1_start & 0xff00) | value) << 1;
    ted.timer_running[0] = 0;
}

static void ted_timer_t1_store_high(uint8_t value)
{
    alarm_unset(ted_t1_alarm);
    t1_value = (ted.t1_start = (ted.t1_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t1_alarm, maincpu_clk
              + (ted.t1_start == 0 ? 65536 : ted.t1_start) * 2);
    t1_last_restart = maincpu_clk;
    ted.timer_running[0] = 1;
}

static void ted_timer_t2_store_low(uint8_t value)
{
    alarm_unset(ted_t2_alarm);
    t2_value = (t2_start = (t2_start & 0xff00) | value) << 1;
    ted.timer_running[1] = 0;
}

static void ted_timer_t2_store_high(uint8_t value)
{
    alarm_unset(ted_t2_alarm);
    t2_value = (t2_start = (t2_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t2_alarm, maincpu_clk
              + (t2_start == 0 ? 65536 : t2_start) * 2);
    t2_last_restart = maincpu_clk;
    ted.timer_running[1] = 1;
}

static void ted_timer_t3_store_low(uint8_t value)
{
    alarm_unset(ted_t3_alarm);
    t3_value = (t3_start = (t3_start & 0xff00) | value) << 1;
    ted.timer_running[2] = 0;
}

static void ted_timer_t3_store_high(uint8_t value)
{
    alarm_unset(ted_t3_alarm);
    t3_value = (t3_start = (t3_start & 0x00ff) | (value << 8)) << 1;
    alarm_set(ted_t3_alarm, maincpu_clk
              + (t3_start == 0 ? 65536 : t3_start) * 2);
    t3_last_restart = maincpu_clk;
    ted.timer_running[2] = 1;
}

static uint8_t ted_timer_t1_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI1 READL %02x", t1_value & 0xff);
#endif
    if (ted.timer_running[0]) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    return (uint8_t)(t1_value >> 1);
}

static uint8_t ted_timer_t1_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI1 READH %02x", t1_value >> 8);
#endif
    if (ted.timer_running[0]) {
        t1_value -= maincpu_clk - t1_last_restart;
        t1_last_restart = maincpu_clk;
    }
    return (uint8_t)(t1_value >> 9);
}

static uint8_t ted_timer_t2_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI2 READL %02x", t2_value & 0xff);
#endif
    if (ted.timer_running[1]) {
        t2_value -= maincpu_clk - t2_last_restart;
        t2_last_restart = maincpu_clk;
    }
    return (uint8_t)(t2_value >> 1);
}

static uint8_t ted_timer_t2_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI2 READH %02x", t2_value >> 8);
#endif
    if (ted.timer_running[1]) {
        t2_value -= maincpu_clk - t2_last_restart;
        t2_last_restart = maincpu_clk;
    }
    return (uint8_t)(t2_value >> 9);
}

static uint8_t ted_timer_t3_read_low(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI3 READL %02x", t3_value & 0xff);
#endif
    if (ted.timer_running[2]) {
        t3_value -= maincpu_clk - t3_last_restart;
        t3_last_restart = maincpu_clk;
    }
    return (uint8_t)(t3_value >> 1);
}

static uint8_t ted_timer_t3_read_high(void)
{
#ifdef DEBUG_TIMER
    log_debug("TI3 READH %02x", t3_value >> 8);
#endif
    if (ted.timer_running[2]) {
        t3_value -= maincpu_clk - t3_last_restart;
        t3_last_restart = maincpu_clk;
    }
    return (uint8_t)(t3_value >> 9);
}

/*-----------------------------------------------------------------------*/

void ted_timer_store(uint16_t addr, uint8_t value)
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

uint8_t ted_timer_read(uint16_t addr)
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
    ted.t1_start = 0;
    ted_t1_alarm = alarm_new(maincpu_alarm_context, "TED T1", ted_t1, NULL);
    ted_t2_alarm = alarm_new(maincpu_alarm_context, "TED T2", ted_t2, NULL);
    ted_t3_alarm = alarm_new(maincpu_alarm_context, "TED T3", ted_t3, NULL);
}

void ted_timer_reset(void)
{
    alarm_unset(ted_t1_alarm);
    ted.t1_start = 0;
    alarm_unset(ted_t2_alarm);
    alarm_unset(ted_t3_alarm);
}
