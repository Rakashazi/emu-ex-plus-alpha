/*
 * ds12c887.c - DS12C887 RTC emulation.
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

#include "ds12c887.h"
#include "lib.h"
#include "rtc.h"

#include <string.h>

/* The DS12C887 is a parallel based RTC, it has the following features:
 * - Real-Time Clock Counts Seconds, Minutes, Hours, Date of the Month,
 *   Month, Day of the Week, Year and century
 * - 113 x 8 Battery-Backed General-Purpose RAM
 * - 24/12h mode
 * - Settable Daylight Savings flag
 * - Oscilator control bits
 * - Write Protect flag
 * - Clock registers switchable between BCD and decimal
 * - Battery state flag
 * - Alarm registers that can trigger an interrupt signal
 * - Various state bits
 */

/* The DS12C887 has the following clock registers:
 *
 * register 0  : BCD mode:    bit  7   RAM bit
 *               BCD mode:    bits 6-4 10 seconds
 *               BCD mode:    bits 3-0 seconds
 *               binary mode: bits 7-6 RAM bits
 *               binary mode: bits 5-0 seconds
 *
 * register 1  : BCD mode:    bit  7   RAM bit
 *               BCD mode:    bits 6-4 10 seconds alarm
 *               BCD mode:    bits 3-0 seconds alarm
 *               binary mode: bits 7-6 RAM bits
 *               binary mode: bits 5-0 seconds alarm
 *
 * register 2  : BCD mode:    bit  7   RAM bit
 *               BCD mode:    bits 6-4 10 minutes
 *               BCD mode:    bits 3-0 minutes
 *               binary mode: bits 7-6 RAM bits
 *               binary mode: bits 5-0 minutes
 *
 * register 3  : BCD mode:    bit  7   RAM bit
 *               BCD mode:    bits 6-4 10 minutes alarm
 *               BCD mode:    bits 3-0 minutes alarm
 *               binary mode: bits 7-6 RAM bits
 *               binary mode: bits 5-0 minutes alarm
 *
 * register 4  : BCD mode:    24 hour mode: bits 7-6 RAM bits
 *               BCD mode:    24 hour mode: bits 5-4 10 hours
 *               BCD mode:    24 hour mode: bits 3-0 hours
 *               BCD mode:    12 hour mode: bit  7   AM/PM indicator (1 = PM, 0 = AM)
 *               BCD mode:    12 hour mode: bits 6-5 RAM bits
 *               BCD mode:    12 hour mode: bit  4   10 hours
 *               BCD mode:    12 hour mode: bits 3-0 hours
 *               binary mode: 24 hour mode: bits 7-5 RAM bits
 *               binary mode: 24 hour mode: bits 4-0 hours
 *               binary mode: 12 hour mode: bit  7   AM/PM indicator (1 = PM, 0 = AM)
 *               binary mode: 12 hour mode: bits 6-4 RAM bits
 *               binary mode: 12 hour mode: bits 3-0 hours
 *
 * register 5  : BCD mode:    24 hour mode: bits 7-6 RAM bits
 *               BCD mode:    24 hour mode: bits 5-4 10 hours alarm
 *               BCD mode:    24 hour mode: bits 3-0 hours alarm
 *               BCD mode:    12 hour mode: bit  7   alarm AM/PM indicator (1 = PM, 0 = AM)
 *               BCD mode:    12 hour mode: bits 6-5 RAM bits
 *               BCD mode:    12 hour mode: bit  4   10 hours alarm
 *               BCD mode:    12 hour mode: bits 3-0 hours alarm
 *               binary mode: 24 hour mode: bits 7-5 RAM bits
 *               binary mode: 24 hour mode: bits 4-0 hours alarm
 *               binary mode: 12 hour mode: bit  7   alarm AM/PM indicator (1 = PM, 0 = AM)
 *               binary mode: 12 hour mode: bits 6-4 RAM bits
 *               binary mode: 12 hour mode: bits 3-0 hours alarm
 *
 * register 6  : bits 7-4 RAM bits
 *               bits 3-0 day of the week
 *
 * register 7  : BCD mode:    bits 7-6 RAM bits
 *               BCD mode:    bits 5-4 10 days of the month
 *               BCD mode:    bits 3-0 days of the month
 *               binary mode: bits 7-5 RAM bits
 *               binary mode: bits 4-0 days of the month
 *
 * register 8  : BCD mode:    bits 7-5 RAM bits
 *               BCD mode:    bit  4   10 months
 *               BCD mode:    bits 3-0 months
 *               binary mode: bits 7-4 RAM bits
 *               binary mode: bits 3-0 months
 *
 * register 9  : BCD mode:    bits 7-4 10 years
 *               BCD mode:    bits 3-0 years
 *               binary mode: bit  7   RAM bit
 *               binary mode: bits 6-0 years
 *
 * register 10 : bit  7   The Update In Progress (UIP) bit is a status flag that can be
 *                        monitored. When the UIP bit is a 1, the update transfer will
 *                        soon occur. When UIP is a 0, the update transfer will not occur
 *                        for at least 244us. The time, calendar, and alarm information
 *                        in RAM is fully available for access when the UIP bit is 0. The
 *                        UIP bit is read-only and is not affected by RESET . Writing the
 *                        SET bit in Register 11 to a 1 inhibits any update transfer and
 *                        clears the UIP status bit.
 *
 *               bits 6-4 These three bits are used to turn the oscillator on or off and
 *                        to reset the countdown chain. A pattern of 010 is the only
 *                        combination of bits that will turn the oscillator on and allow
 *                        the RTC to keep time. A pattern of 11X will enable the
 *                        oscillator but holds the countdown chain in reset. The next
 *                        update will occur at 500 ms after a pattern of 010 is written
 *                        to DV0, DV1, and DV2.
 *
 *               bits 3-0 These bits are normally used for the square wave output pin
 *                        I decided not to emulate this so these bits are just emulated
 *                        as RAM bits.
 *
 * register 11 : bit 7 The SET bit, when the SET bit is a 0, the update transfer
 *                     functions normally by advancing the counts once per second. When
 *                     the SET bit is written to a 1, any update transfer is inhibited
 *                     and the program can initialize the time and calendar bytes
 *                     without an update occurring in the midst of initializing. Read
 *                     cycles can be executed in a similar manner. SET is a read/write
 *                     bit and is not affected by RESET or internal functions of the
 *                     DS12C887.
 *               bit 6 Normally The Periodic Interrup Enable bit, not emulated, so
 *                     emulated as a RAM bit.
 *               bit 5 The Alarm Interrupt Enable (AIE) bit is a read/write bit which,
 *                     when set to a 1, permits the Alarm Flag (AF) bit in register 12
 *                     to assert IRQ. An alarm interrupt occurs for each second that
 *                     the 3 time bytes equal the 3 alarm bytes including a
 *                     “don’t care” alarm code of binary 11XXXXXX. When the AIE bit is
 *                     set to 0, the AF bit does not initiate the IRQ signal. The
 *                     internal functions of the DS12C887 not affect the AIE bit.
 *               bit 4 The Update Ended Interrupt Enable (UIE) bit is a read/write bit
 *                     that enables the Update End Flag (UF) bit in Register 12 to assert
 *                     IRQ. The RESET pin going low or the SET bit going high clears the
 *                     UIE bit.
 *               bit 3 Normally the Square Wave Interrupt Enable bit, not emulated, so
 *                     emulated as a RAM bit.
 *               bit 2 The Data Mode (DM) bit indicates whether time and calendar
 *                     information is in binary or BCD format. The DM bit is set by the
 *                     program to the appropriate format and can be read as required.
 *                     This bit is not modified by internal functions or RESET . A 1 in
 *                     DM signifies binary data while a 0 in DM specifies Binary Coded
 *                     Decimal (BCD) data.
 *               bit 1 The 24/12 control bit establishes the format of the hours byte.
 *                     A 1 indicates the 24-hour mode and a 0 indicates the 12-hour
 *                     mode. This bit is read/write and is not affected by internal
 *                     functions or RESET.
 *               bit 0 The Daylight Savings Enable (DSE) bit is a read/write bit which
 *                     enables two special updates when DSE is set to 1. On the first
 *                     Sunday in April the time increments from 1:59:59 AM to
 *                     3:00:00 AM. On the last Sunday in October when the time first
 *                     reaches 1:59:59 AM it changes to 1:00:00 AM. These special
 *                     updates do not occur when the DSE bit is a zero. This bit is
 *                     not affected by internal functions or RESET.
 *
 * register 12 : bit  7   The Interrupt Request Flag, it is driven high when both Alarm
 *                        Flag and Alarm Interrupt Enable are high, or when both Update
 *                        Ended Flag and Update Ended Interrupt Enable are high. if
 *                        this bit is high an IRQ will be initiated.
 *               bit  6   The Periodic Interrupt flag, not emulated, so emulated as a
 *                        RAM bit.
 *               bit  5   The Alarm Flag, when high it means that the current time has
 *                        matched the alarm time, when the Alarm Interrupt Enable is
 *                        also high this will generate an IRQ. A reset or a read from
 *                        register 12 will clear this flag.
 *               bit  4   The Update Ended Flag, when high it means that an update to
 *                        the second register has just ended, when the Update Ended
 *                        Interrupt Enable is also high this will generate an IRQ. A
 *                        reset or a read from register 12 will clear this flag.
 *               bits 3-0 0
 *
 * register 13 : bit  7   Condition of the battery, when 1 the battery is good, this
 *                        is emulated as always being 1.
 *               bits 6-0 0
 *
 * register 50 : bits 7-6 RAM bits
 *               bits 5-4 10 centuries
 *               bits 3-0 centuries
 *               this register can only be accessed in BCD mode
 */

/* This module is currently used in the following emulated hardware:
   - C64/C128/VIC20 DS12C887 RTC cartridge
 */

/* ---------------------------------------------------------------------------------------------------- */

void ds12c887_reset(rtc_ds12c887_t *context)
{
    context->ctrl_regs[1] &= 0xaf;
    context->alarm_flag = 0;
    context->end_of_update_flag = 0;
}

rtc_ds12c887_t *ds12c887_init(char *device)
{
    rtc_ds12c887_t *retval = lib_calloc(1, sizeof(rtc_ds12c887_t));
    int loaded = rtc_load_context(device, DS12C887_RAM_SIZE, DS12C887_REG_SIZE);

    if (loaded) {
        retval->ram = rtc_get_loaded_ram();
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->ram = lib_calloc(1, DS12C887_RAM_SIZE);
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, DS12C887_REG_SIZE);
    }
    memcpy(retval->old_ram, retval->ram, DS12C887_RAM_SIZE);
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, DS12C887_REG_SIZE);

    retval->bcd = 1;
    retval->ctrl_regs[0] = 0x20;
    retval->device = lib_stralloc(device);

    return retval;
}

void ds12c887_destroy(rtc_ds12c887_t *context, int save)
{
    if (save) {
        if (memcmp(context->ram, context->old_ram, DS12C887_RAM_SIZE) ||
            memcmp(context->clock_regs, context->old_clock_regs, DS12C887_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(context->ram, DS12C887_RAM_SIZE, context->clock_regs, DS12C887_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->ram);
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static BYTE ds12c887_get_clock(rtc_ds12c887_t *context, BYTE address, time_t latch)
{
    BYTE retval;
    BYTE hour;

    switch (address) {
        case DS12C887_REG_SECONDS:
            retval = context->clock_regs[DS12C887_REG_SECONDS] & ((context->bcd) ? 0x80 : 0xc0);
            retval |= rtc_get_second(latch, context->bcd);
            break;
        case DS12C887_REG_SECONDS_ALARM:
            retval = context->clock_regs[DS12C887_REG_SECONDS_ALARM];
            break;
        case DS12C887_REG_MINUTES:
            retval = context->clock_regs[DS12C887_REG_MINUTES] & ((context->bcd) ? 0x80 : 0xc0);
            retval |= rtc_get_minute(latch, context->bcd);
            break;
        case DS12C887_REG_MINUTES_ALARM:
            retval = context->clock_regs[DS12C887_REG_MINUTES_ALARM];
            break;
        case DS12C887_REG_HOURS:
            retval = context->clock_regs[DS12C887_REG_HOURS] & ((context->bcd) ? ((context->am_pm) ? 0x60 : 0xc0) : ((context->am_pm) ? 0x70 : 0xe0));
            hour = rtc_get_hour(latch, 0);
            if (context->bcd) {
                if (context->am_pm) {
                    if (hour == 0) {
                        hour = 0x12;
                    } else if (hour == 10 || hour == 11) {
                        hour += 6;
                    } else if (hour == 12) {
                        hour = 0x92;
                    } else if (hour > 12 && hour < 22) {
                        hour -= 12;
                        hour |= 0x80;
                    } else if (hour == 22 || hour == 23) {
                        hour -= 6;
                        hour |= 0x80;
                    }
                } else {
                    hour = ((hour / 10) << 4) + (hour % 10);
                }
            } else {
                if (context->am_pm) {
                    if (hour == 0) {
                        hour = 12;
                    } else if (hour == 12) {
                        hour |= 0x80;
                    } else if (hour > 12) {
                        hour -= 12;
                        hour |= 0x80;
                    }
                }
            }
            retval |= hour;
            break;
        case DS12C887_REG_HOURS_ALARM:
            retval = context->clock_regs[DS12C887_REG_HOURS_ALARM];
            break;
        case DS12C887_REG_DAY_OF_WEEK:
            retval = context->clock_regs[DS12C887_REG_DAY_OF_WEEK] & 0xf8;
            retval |= rtc_get_weekday(latch) + 1;
            break;
        case DS12C887_REG_DAY_OF_MONTH:
            retval = context->clock_regs[DS12C887_REG_DAY_OF_MONTH] & ((context->bcd) ? 0xc0 : 0xe0);
            retval |= rtc_get_day_of_month(latch, context->bcd);
            break;
        case DS12C887_REG_MONTHS:
            retval = context->clock_regs[DS12C887_REG_MONTHS] & ((context->bcd) ? 0xe0 : 0xf0);
            retval |= rtc_get_month(latch, context->bcd);
            break;
        case DS12C887_REG_YEARS:
            if (!context->bcd) {
                retval = context->clock_regs[DS12C887_REG_YEARS] & 0x80;
                retval |= rtc_get_year(latch, 0);
            } else {
                retval = rtc_get_year(latch, 1);
            }
            break;
        case DS12C887_REG_CENTURIES:
            if (context->bcd) {
                retval = context->clock_regs[10] & 0xc0;
                retval |= rtc_get_century(latch, 1);
            } else {
                retval = 0;
            }
            break;
        default:
            retval = 0;
    }
    return retval;
}

static void ds12c887_write_clock_byte(rtc_ds12c887_t *context, BYTE address, BYTE data)
{
    int val;
    BYTE temp;

    switch (address) {
        case DS12C887_REG_SECONDS:
            context->clock_regs[address] = data;
            val = data & ((context->bcd) ? 0x7f : 0x3f);
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_second(val, context->clock_halt_latch, context->bcd);
            } else {
                context->offset = rtc_set_second(val, context->offset, context->bcd);
            }
            break;
        case DS12C887_REG_MINUTES:
            context->clock_regs[address] = data;
            val = data & ((context->bcd) ? 0x7f : 0x3f);
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, context->bcd);
            } else {
                context->offset = rtc_set_minute(val, context->offset, context->bcd);
            }
            break;
        case DS12C887_REG_HOURS:
            context->clock_regs[address] = data;
            if (context->am_pm) {
                if (context->bcd) {
                    temp = data & 0x9f;
                    if (temp < 0x10) {
                        val = temp;
                    } else if (temp == 0x10 || temp == 0x11) {
                        val = temp - 6;
                    } else if (temp == 0x12) {
                        val = 0;
                    } else if (temp < 0x90) {
                        val = temp & 0x7f;
                        val += 12;
                    } else if (temp == 0x90 || temp == 0x91) {
                        val = temp & 0x7f;
                        val += 6;
                    } else {
                        val = 12;
                    }
                } else {
                    temp = data & 0x8f;
                    if (temp < 12) {
                        val = temp;
                    } else if (temp == 12) {
                        val = 0;
                    } else if (temp < 140) {
                        val = temp & 0x7f;
                        val += 12;
                    } else {
                        val = 12;
                    }
                }
            } else {
                if (context->bcd) {
                    temp = data & 0x3f;
                    val = ((temp >> 4) * 10) + temp % 16;
                } else {
                    val = data & 0x1f;
                }
            }
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_hour(val, context->clock_halt_latch, 0);
            } else {
                context->offset = rtc_set_hour(val, context->offset, 0);
            }
            break;
        case DS12C887_REG_DAY_OF_WEEK:
            context->clock_regs[address] = data;
            val = data & 7;
            val--;
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_weekday(val, context->clock_halt_latch);
            } else {
                context->offset = rtc_set_weekday(val, context->offset);
            }
            break;
        case DS12C887_REG_DAY_OF_MONTH:
            context->clock_regs[address] = data;
            val = data & ((context->bcd) ? 0x3f : 0x1f);
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_day_of_month(val, context->clock_halt_latch, context->bcd);
            } else {
                context->offset = rtc_set_day_of_month(val, context->offset, context->bcd);
            }
            break;
        case DS12C887_REG_MONTHS:
            context->clock_regs[address] = data;
            val = data & ((context->bcd) ? 0x1f : 0xf);
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_month(val, context->clock_halt_latch, context->bcd);
            } else {
                context->offset = rtc_set_month(val, context->offset, context->bcd);
            }
            break;
        case DS12C887_REG_YEARS:
            context->clock_regs[address] = data;
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_year(data, context->clock_halt_latch, context->bcd);
            } else {
                context->offset = rtc_set_year(data, context->offset, context->bcd);
            }
            break;
        case DS12C887_REG_CENTURIES:
            context->clock_regs[10] = data;
            if (context->bcd) {
                val = data & 0x3f;
                if (context->clock_halt) {
                    context->clock_halt_latch = rtc_set_latched_century(val, context->clock_halt_latch, 1);
                } else {
                    context->offset = rtc_set_century(val, context->offset, 1);
                }
            }
            break;
    }
}

static void ds12c887_write_latched_clock_regs(rtc_ds12c887_t *context)
{
    int i;

    for (i = 0; i < 10; i++) {
        if (context->clock_regs_changed[i]) {
            ds12c887_write_clock_byte(context, (BYTE)i, context->clock_regs[i]);
        }
    }
    if (context->clock_regs_changed[10]) {
        ds12c887_write_clock_byte(context, 50, context->clock_regs[10]);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

/* This function needs to be called at least every 1/10th of a second
 * it returns a 1 if an IRQ was generated */
int ds12c887_update_flags(rtc_ds12c887_t *context)
{
    BYTE current;
    BYTE alarm;
    time_t latch;
    int match = 1;
    int irq_return = 0;

    /* check if the clock is halted */
    if (context->clock_halt) {
        latch = context->clock_halt_latch;
    } else {
        latch = rtc_get_latch(context->offset);
    }

    /* get current second */
    current = ds12c887_get_clock(context, DS12C887_REG_SECONDS, latch);

    /* convert to binary if needed */
    if (context->bcd) {
        current &= 0x7f;
        current = ((current >> 4) * 10) + current % 16;
    } else {
        current &= 0x3f;
    }

    /* compare with previous second */
    if (context->prev_second != current) {
        context->end_of_update_flag = 1;
        context->prev_second = current;
        if (context->ctrl_regs[1] & 0x10) {
            irq_return = 1;
        }
    } else {
        return 0;
    }

    /* get current second */
    current = ds12c887_get_clock(context, DS12C887_REG_SECONDS, latch);

    /* get alarm second */
    alarm = context->clock_regs[DS12C887_REG_SECONDS_ALARM];

    /* check for don't care */
    if (!(alarm & 0xc0)) {
        /* get valid second bits */
        if (context->bcd) {
            current &= 0x7f;
            alarm &= 0x7f;
        } else {
            current &= 0x3f;
            alarm &= 0x7f;
        }

        /* compare seconds */
        if (current != alarm) {
            match = 0;
        }
    }

    if (match) {
        /* get current minute */
        current = ds12c887_get_clock(context, DS12C887_REG_MINUTES, latch);

        /* get alarm second */
        alarm = context->clock_regs[DS12C887_REG_MINUTES_ALARM];

        /* check for don't care */
        if (!(alarm & 0xc0)) {
            /* get valid minute bits */
            if (context->bcd) {
                current &= 0x7f;
                alarm &= 0x7f;
            } else {
                current &= 0x3f;
                alarm &= 0x7f;
            }

            /* compare minutes */
            if (current != alarm) {
                match = 0;
            }
        }
    }

    if (match) {
        /* get current hour */
        current = ds12c887_get_clock(context, DS12C887_REG_HOURS, latch);

        /* get alarm hour */
        alarm = context->clock_regs[DS12C887_REG_HOURS_ALARM];

        /* check for don't care */
        if (!(alarm & 0xc0)) {
            /* get valid hour bits */
            if (context->bcd) {
                if (context->am_pm) {
                    current &= 0x9f;
                    alarm &= 0x9f;
                } else {
                    current &= 0x3f;
                    alarm &= 0x3f;
                }
            } else {
                if (context->am_pm) {
                    current &= 0x8f;
                    alarm &= 0x8f;
                } else {
                    current &= 0x1f;
                    alarm &= 0x1f;
                }
            }

            /* compare hours */
            if (current != alarm) {
                match = 0;
            }
        }
    }

    if (match) {
        context->alarm_flag = 1;
        if (context->ctrl_regs[1] & 0x20) {
            irq_return = 1;
        }
    }
    return irq_return;
}

/* ---------------------------------------------------------------------------------------------------- */

void ds12c887_store_address(rtc_ds12c887_t *context, BYTE address)
{
    context->reg = address & 0x7f;
}

void ds12c887_store_data(rtc_ds12c887_t *context, BYTE data)
{
    int i;

    switch (context->reg) {
        case DS12C887_REG_SECONDS:
        case DS12C887_REG_MINUTES:
        case DS12C887_REG_HOURS:
        case DS12C887_REG_DAY_OF_WEEK:
        case DS12C887_REG_DAY_OF_MONTH:
        case DS12C887_REG_MONTHS:
        case DS12C887_REG_YEARS:
            if (context->set) {
                context->clock_regs[context->reg] = data;
                context->clock_regs_changed[context->reg] = 1;
            } else {
                ds12c887_write_clock_byte(context, context->reg, data);
            }
            break;
        case DS12C887_REG_SECONDS_ALARM:
        case DS12C887_REG_MINUTES_ALARM:
        case DS12C887_REG_HOURS_ALARM:
            context->clock_regs[context->reg] = data;
            break;
        case DS12C887_REG_CENTURIES:
            if (context->set) {
                context->clock_regs[10] = data;
                context->clock_regs_changed[10] = 1;
            } else {
                ds12c887_write_clock_byte(context, context->reg, data);
            }
        case DS12C887_REG_CTRL_A:
            data &= 0x7f;
            if ((data & 0x70) != 0x20) {
                if (!context->clock_halt) {
                    context->clock_halt_latch = rtc_get_latch(context->offset);
                    context->clock_halt = 1;
                }
            } else {
                if (context->clock_halt) {
                    context->offset = context->offset - (rtc_get_latch(0) - (context->clock_halt_latch - context->offset));
                    context->clock_halt = 0;
                }
            }
            context->ctrl_regs[0] = data;
            break;
        case DS12C887_REG_CTRL_B:
            context->ctrl_regs[1] = data;
            if (data & 0x80) {
                if (!context->set) {
                    context->set = 1;
                    context->ctrl_regs[1] &= 0xef;
                    context->set_latch = (context->clock_halt) ? context->clock_halt_latch : rtc_get_latch(context->offset);
                    for (i = 0; i < 11; i++) {
                        context->clock_regs_changed[i] = 0;
                    }
                }
            } else {
                if (context->set) {
                    context->set = 0;
                    ds12c887_write_latched_clock_regs(context);
                }
            }
            if (data & 4) {
                context->bcd = 0;
            } else {
                context->bcd = 1;
            }
            if (data & 2) {
                context->am_pm = 0;
            } else {
                context->am_pm = 1;
            }
            break;
        case DS12C887_REG_CTRL_C:
        case DS12C887_REG_CTRL_D:
            break;
        default:
            context->ram[context->reg] = data;
            break;
    }
}

BYTE ds12c887_read(rtc_ds12c887_t *context)
{
    BYTE retval;
    time_t latch;

    if (context->clock_halt || context->set) {
        if (!context->clock_halt) {
            latch = context->set_latch;
        } else {
            latch = context->clock_halt_latch;
        }
    } else {
        latch = rtc_get_latch(context->offset);
    }

    switch (context->reg) {
        case DS12C887_REG_SECONDS:
        case DS12C887_REG_SECONDS_ALARM:
        case DS12C887_REG_MINUTES:
        case DS12C887_REG_MINUTES_ALARM:
        case DS12C887_REG_HOURS:
        case DS12C887_REG_HOURS_ALARM:
        case DS12C887_REG_DAY_OF_WEEK:
        case DS12C887_REG_DAY_OF_MONTH:
        case DS12C887_REG_MONTHS:
        case DS12C887_REG_YEARS:
        case DS12C887_REG_CENTURIES:
            retval = ds12c887_get_clock(context, context->reg, latch);
            break;
        case DS12C887_REG_CTRL_A:
            retval = context->ctrl_regs[0];
            break;
        case DS12C887_REG_CTRL_B:
            retval = context->ctrl_regs[1];
            break;
        case DS12C887_REG_CTRL_C:
            ds12c887_update_flags(context);
            retval = (context->alarm_flag || context->end_of_update_flag) ? 0x80 : 0;
            retval |= (context->alarm_flag) ? 0x20 : 0;
            retval |= (context->end_of_update_flag) ? 0x10 : 0;
            context->alarm_flag = 0;
            context->end_of_update_flag = 0;
            break;
        case DS12C887_REG_CTRL_D:
            retval = 0x80;
            break;
        default:
            retval = context->ram[context->reg];
    }
    return retval;
}
