/*
 * ds1216e.c - DS1216E RTC emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "ds1216e.h"
#include "lib.h"
#include "rtc.h"

#include <string.h>

/* The DS1216E is a phantom RTC module, it can be used in between a ROM
 * socket and the actual ROM, the RTC registers are accessed by first
 * activating the output by means of a read-bit-match-pattern, once
 * the output has been activated the RTC registers will be serially
 * output on the D0 pin in 64 consecutive reads. The RTC has the
 * following features:
 * - Real-Time Clock Counts centi-seconds, seconds, minutes, hours, date of the month,
 *   months, and years
 * - Watch function is transparant to ROM function
 * - Month and year determine the amount of days in the year, leap-year compensation
 *   valid upto 2100
 * - All clock registers are in BCD format
 */

/* The DS1216E has the following clock registers:
 *
 * register 0 : bits 7-4 tenths of seconds
 *              bits 3-0 hundreths of seconds
 *
 * register 1 : bit  7   0
 *              bits 6-4 10 seconds
 *              bits 3-0 seconds
 *
 * register 2 : bit  7   0
 *              bits 6-4 10 minutes
 *              bits 3-0 minutes
 *
 * register 3 : bit  7 12/24 hour indicator
 *              bit  6 0
 *              bit  5 20 hours (24 hour format) / AM/PM indicator (12 hour format)
 *              bit  4 10 hours
 *              bits 3-0 hours
 *
 * register 4 : bits 7-6 0
 *              bit  5   oscilator control
 *              bit  4   reset control
 *              bit  3   0
 *              bits 2-0 day of week (1-7, mon-sun)
 *
 * register 5: bits 7-6 0
 *             bits 5-4 10 day of month
 *             bits 3-0 day of month
 *
 * register 6: bits 7-5 0
 *             bit  4   10 months
 *             bits 3-0 months
 *
 * register 7: bits 7-4 10 years
 *             bits 3-0 years
 */

/* This module is currently used in the following emulated hardware:
   - IEC FD2000/4000 disk drives
 */

/* ---------------------------------------------------------------------------------------------------- */

rtc_ds1216e_t *ds1216e_init(char *device)
{
    rtc_ds1216e_t *retval = lib_calloc(1, sizeof(rtc_ds1216e_t));
    int loaded = rtc_load_context(device, 0, DS1216E_REG_SIZE);

    if (loaded) {
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, DS1216E_REG_SIZE);
    }
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, DS1216E_REG_SIZE);

    retval->device = lib_stralloc(device);

    return retval;
}

void ds1216e_destroy(rtc_ds1216e_t *context, int save)
{
    if (save) {
        if (memcmp(context->clock_regs, context->old_clock_regs, DS1216E_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(NULL, 0, context->clock_regs, DS1216E_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static BYTE pattern[64] = {
    1, 0, 1, 0, 0, 0, 1, 1,
    0, 1, 0, 1, 1, 1, 0, 0,
    1, 1, 0, 0, 0, 1, 0, 1,
    0, 0, 1, 1, 1, 0, 1, 0,
    1, 0, 1, 0, 0, 0, 1, 1,
    0, 1, 0, 1, 1, 1, 0, 0,
    1, 1, 0, 0, 0, 1, 0, 1,
    0, 0, 1, 1, 1, 0, 1, 0
};

static void ds1216e_latch_regs(rtc_ds1216e_t *context)
{
    time_t latch = (context->inactive) ? context->latch : rtc_get_latch(context->offset);

    context->clock_regs[DS1216E_REGISTER_CENTISECONDS] = rtc_get_centisecond(1);
    context->clock_regs[DS1216E_REGISTER_SECONDS] = rtc_get_second(latch, 1);
    context->clock_regs[DS1216E_REGISTER_MINUTES] = rtc_get_minute(latch, 1);
    context->clock_regs[DS1216E_REGISTER_HOURS] = (context->hours12) ? 0x80 : 0;
    if (context->hours12) {
        context->clock_regs[DS1216E_REGISTER_HOURS] |= rtc_get_hour_am_pm(latch, 1);
    } else {
        context->clock_regs[DS1216E_REGISTER_HOURS] |= rtc_get_hour(latch, 1);
    }
    context->clock_regs[DS1216E_REGISTER_WEEKDAYS] = (context->inactive) ? 0x20 : 0;
    context->clock_regs[DS1216E_REGISTER_WEEKDAYS] |= (context->reset) ? 0x10 : 0;
    context->clock_regs[DS1216E_REGISTER_WEEKDAYS] |= ((rtc_get_weekday(latch) - 1) % 7) + 1;
    context->clock_regs[DS1216E_REGISTER_MONTHDAYS] = rtc_get_day_of_month(latch, 1);
    context->clock_regs[DS1216E_REGISTER_MONTHS] = rtc_get_month(latch, 1);
    context->clock_regs[DS1216E_REGISTER_YEARS] = rtc_get_year(latch, 1);
}

static void ds1216e_match_pattern(rtc_ds1216e_t *context, WORD address)
{
    int i;

    /* check for read cycle */
    if (address & 4) {
        context->pattern_pos = 0;
        context->pattern_ignore = 0;
        return;
    }

    /* check for pattern ignore */
    if (context->pattern_ignore) {
        return;
    }

    /* check pattern bit */
    if ((address & 1) == pattern[context->pattern_pos]) {
        context->pattern_pos++;
        if (context->pattern_pos == 64) {
            context->output = 1;
            context->output_pos = 0;
            for (i = 0; i < 8; i++) {
                context->clock_regs_changed[i] = 0;
            }
            ds1216e_latch_regs(context);
        }
    } else {
        context->pattern_ignore = 1;
    }
}

static void ds1216e_update_clock(rtc_ds1216e_t *context)
{
    int new_osc;
    int new_reset;
    int new_12;

    /* setting centiseconds has no effect on the offset used for the clock,
       as this is defined in seconds, so any changes to the centiseconds
       will just be ignored.
     */

    /* prepare clock regs for updating, clear unused bits, fix value offsets */
    context->clock_regs[DS1216E_REGISTER_SECONDS] &= 0x7f;
    context->clock_regs[DS1216E_REGISTER_MINUTES] &= 0x7f;

    new_12 = context->clock_regs[DS1216E_REGISTER_HOURS] >> 7;
    context->clock_regs[DS1216E_REGISTER_HOURS] &= 0x3f;

    new_osc = (context->clock_regs[DS1216E_REGISTER_WEEKDAYS] & 0x20) ? 1 : 0;
    new_reset = (context->clock_regs[DS1216E_REGISTER_WEEKDAYS] & 0x10) ? 1 : 0;
    context->clock_regs[DS1216E_REGISTER_WEEKDAYS] &= 7;

    context->clock_regs[DS1216E_REGISTER_MONTHDAYS] &= 0x3f;
    context->clock_regs[DS1216E_REGISTER_MONTHS] &= 0x1f;

    if (context->inactive) {
        if (context->clock_regs_changed[DS1216E_REGISTER_YEARS]) {
            context->latch = rtc_set_latched_year(context->clock_regs[DS1216E_REGISTER_YEARS], context->latch, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MONTHS]) {
            context->latch = rtc_set_latched_month(context->clock_regs[DS1216E_REGISTER_MONTHS], context->latch, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MONTHDAYS]) {
            context->latch = rtc_set_latched_day_of_month(context->clock_regs[DS1216E_REGISTER_MONTHDAYS], context->latch, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_WEEKDAYS]) {
            context->latch = rtc_set_latched_weekday(context->clock_regs[DS1216E_REGISTER_WEEKDAYS] % 7, context->latch);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_HOURS]) {
            if (new_12) {
                context->latch = rtc_set_latched_hour_am_pm(context->clock_regs[DS1216E_REGISTER_HOURS], context->latch, 1);
            } else {
                context->latch = rtc_set_latched_hour(context->clock_regs[DS1216E_REGISTER_HOURS], context->latch, 1);
            }
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MINUTES]) {
            context->latch = rtc_set_latched_minute(context->clock_regs[DS1216E_REGISTER_MINUTES], context->latch, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_SECONDS]) {
            context->latch = rtc_set_latched_second(context->clock_regs[DS1216E_REGISTER_SECONDS], context->latch, 1);
        }
        if (!new_osc) {
            context->offset = context->offset - (rtc_get_latch(0) - (context->latch - context->offset));
            context->inactive = 0;
        }
    } else {
        if (context->clock_regs_changed[DS1216E_REGISTER_YEARS]) {
            context->offset = rtc_set_year(context->clock_regs[DS1216E_REGISTER_YEARS], context->offset, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MONTHS]) {
            context->offset = rtc_set_month(context->clock_regs[DS1216E_REGISTER_MONTHS], context->offset, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MONTHDAYS]) {
            context->offset = rtc_set_day_of_month(context->clock_regs[DS1216E_REGISTER_MONTHDAYS], context->offset, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_WEEKDAYS]) {
            context->offset = rtc_set_weekday(context->clock_regs[DS1216E_REGISTER_WEEKDAYS] % 7, context->offset);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_HOURS]) {
            if (new_12) {
                context->offset = rtc_set_hour_am_pm(context->clock_regs[DS1216E_REGISTER_HOURS], context->offset, 1);
            } else {
                context->offset = rtc_set_hour(context->clock_regs[DS1216E_REGISTER_HOURS], context->offset, 1);
            }
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_MINUTES]) {
            context->offset = rtc_set_minute(context->clock_regs[DS1216E_REGISTER_MINUTES], context->offset, 1);
        }
        if (context->clock_regs_changed[DS1216E_REGISTER_SECONDS]) {
            context->offset = rtc_set_second(context->clock_regs[DS1216E_REGISTER_SECONDS], context->offset, 1);
        }
        if (new_osc) {
            context->latch = rtc_get_latch(context->offset);
            context->inactive = new_osc;
        }
    }
    context->reset = new_reset;
    context->hours12 = new_12;
}

static void inc_output_pos(rtc_ds1216e_t *context)
{
    context->output_pos++;

    if (context->output_pos == 64) {
        context->output = 0;
        context->pattern_pos = 0;
        ds1216e_update_clock(context);
    }
}

static BYTE ds1216e_output_bit(rtc_ds1216e_t *context, BYTE val)
{
    int reg;
    int bit;
    BYTE mask;

    /* clear bit 0 */
    val &= 0xfe;

    /* decode the position */
    reg = context->output_pos >> 3;
    bit = context->output_pos & 7;
    mask = 1 << bit;

    /* put the bit in place */
    if (context->clock_regs[reg] & mask) {
        val |= 1;
    }

    inc_output_pos(context);

    return val;
}

static void ds1216e_input_bit(rtc_ds1216e_t *context, WORD address)
{
    int reg;
    int bit;
    BYTE mask;

    /* decode the position */
    reg = context->output_pos >> 3;
    bit = context->output_pos & 7;
    mask = 1 << bit;

    /* set/clear the correct bit */
    context->clock_regs[reg] &= ~mask;
    context->clock_regs[reg] |= (address & 1) << bit;

    /* indicate that changes have been made */
    context->clock_regs_changed[reg] = 1;

    inc_output_pos(context);
}

/* ---------------------------------------------------------------------------------------------------- */

BYTE ds1216e_read(rtc_ds1216e_t *context, WORD address, BYTE origbyte)
{
    if (context->output) {
        if (address & 4) {
            return ds1216e_output_bit(context, origbyte);
        } else {
            ds1216e_input_bit(context, address);
        }
    } else {
        ds1216e_match_pattern(context, address);
    }
    return origbyte;
}
