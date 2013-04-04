/*
 * rtc-58321a.c - RTC-58321A RTC emulation.
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

#include "rtc-58321a.h"
#include "lib.h"
#include "rtc.h"

#include <string.h>

/* The RTC-58321A is a 4bit multiplexed address/data line RTC module,
 * the RTC registers are accessed by first setting the address and
 * then reading/writing. The RTC has the following features:
 * - Real-Time Clock Counts seconds, minutes, hours, date of the month,
 *   months, and years
 * - Clock can be stopped by asserting the stop line
 * - All registers are decimal
 */

/* The RTC-58321A has the following clock registers:
 *
 * register 0 : bits 3-0 seconds
 *
 * register 1 : bit  3   0
 *              bits 2-0 10 seconds
 *
 * register 2 : bits 3-0 minutes
 *
 * register 3 : bit  3   0
 *              bits 2-0 10 minutes
 *
 * register 4 : bits 3-0 hours
 *
 * register 5 : bit  3   24/12 hour selection (0 = 12 hour, 1 = 24 hour)
 *              bit  2   AM/PM indication bit (when in 12 hour mode, otherwise 0)
 *              bits 1-0 10 hours
 *
 * register 6 : bit  3   0
 *              bits 2-0 weekdays
 *
 * register 7 : bits 3-0 day of the month
 *
 * register 8 : bits 3-2 leapyear indicator
 *              bits 1-0 10 day of the month
 *
 * register 9 : bits 3-0 months
 *
 * register A : bits 3-1 0
 *              bit  0   10 months
 *
 * register B : bits 3-0 years
 *
 * register C : bits 3-0 10 years
 *
 * register D : bits 3-0 0 (reset register)
 *
 * register E : bits 3-0 0 (SS0 register)
 *
 * register F : bits 3-0 0 (SS1 register)
 */

/* This module is currently used in the following emulated hardware:
   - C64/C128 userport RTC expansion
 */

/* ---------------------------------------------------------------------------------------------------- */

rtc_58321a_t *rtc58321a_init(time_t *offset)
{
    rtc_58321a_t *retval = lib_malloc(sizeof(rtc_58321a_t));
    memset(retval, 0, sizeof(rtc_58321a_t));
    retval->offset = offset;
    retval->hour24 = 1;

    return retval;
}

void rtc58321a_destroy(rtc_58321a_t *context)
{
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

BYTE rtc58321a_read(rtc_58321a_t *context)
{
    BYTE retval = 0;
    time_t latch = (context->stop) ? context->latch : rtc_get_latch(context->offset[0]);

    switch (context->address) {
        case RTC58321A_REGISTER_SECONDS:
            retval = rtc_get_second(latch, 0);
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10SECONDS:
            retval = rtc_get_second(latch, 0);
            retval /= 10;
            break;
        case RTC58321A_REGISTER_MINUTES:
            retval = rtc_get_minute(latch, 0);
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10MINUTES:
            retval = rtc_get_minute(latch, 0);
            retval /= 10;
            break;
        case RTC58321A_REGISTER_HOURS:
            if (context->hour24) {
                retval = rtc_get_hour(latch, 0);
            } else {
                retval = rtc_get_hour_am_pm(latch, 0);
                retval &= 0x1f;
            }
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10HOURS:
            if (context->hour24) {
                retval = rtc_get_hour(latch, 0);
                retval /= 10;
                retval |= 8;
            } else {
                retval = rtc_get_hour_am_pm(latch, 0);
                if (retval > 23) {
                    retval = (retval - 32) / 10;
                    retval |= 4;
                } else {
                    retval /= 10;
                }
            }
            break;
        case RTC58321A_REGISTER_WEEKDAYS:
            retval = rtc_get_weekday(latch) - 1;
            break;
        case RTC58321A_REGISTER_MONTHDAYS:
            retval = rtc_get_day_of_month(latch, 0);
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10MONTHDAYS:
            retval = rtc_get_day_of_month(latch, 0);
            retval /= 10;
/* TODO: leapyear calculation */
            break;
        case RTC58321A_REGISTER_MONTHS:
            retval = rtc_get_month(latch, 0);
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10MONTHS:
            retval = rtc_get_month(latch, 0);
            retval /= 10;
            break;
        case RTC58321A_REGISTER_YEARS:
            retval = rtc_get_year(latch, 0);
            retval %= 10;
            break;
        case RTC58321A_REGISTER_10YEARS:
            retval = rtc_get_year(latch, 0);
            retval /= 10;
            break;
    }
    return retval;
}


void rtc58321a_write_address(rtc_58321a_t *context, BYTE address)
{
    context->address = address & 0xf;
}

#define LIMIT_9(x) (x > 9) ? 9 : x

void rtc58321a_write_data(rtc_58321a_t *context, BYTE data)
{
    time_t latch = (context->stop) ? context->latch : rtc_get_latch(context->offset[0]);
    BYTE real_data = data & 0xf;
    BYTE new_data;

    switch (context->address) {
        case RTC58321A_REGISTER_SECONDS:
            new_data = rtc_get_second(latch, 0);
            new_data /= 10;
            new_data *= 10;
            new_data += LIMIT_9(real_data);
            if (context->stop) {
                context->latch = rtc_set_latched_second(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_second(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_10SECONDS:
            new_data = rtc_get_second(latch, 0);
            new_data %= 10;
            new_data += ((real_data & 7) * 10);
            if (context->stop) {
                context->latch = rtc_set_latched_second(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_second(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_MINUTES:
            new_data = rtc_get_minute(latch, 0);
            new_data /= 10;
            new_data *= 10;
            new_data += LIMIT_9(real_data);
            if (context->stop) {
                context->latch = rtc_set_latched_minute(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_minute(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_10MINUTES:
            new_data = rtc_get_minute(latch, 0);
            new_data %= 10;
            new_data += ((real_data & 7) * 10);
            if (context->stop) {
                context->latch = rtc_set_latched_minute(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_minute(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_HOURS:
            if (context->hour24) {
                new_data = rtc_get_hour(latch, 0);
                new_data /= 10;
                new_data *= 10;
                new_data += LIMIT_9(real_data);
                if (context->stop) {
                    context->latch = rtc_set_latched_hour(new_data, latch, 0);
                } else {
                    context->offset[0] = rtc_set_hour(new_data, context->offset[0], 0);
                }
            } else {
                new_data = rtc_get_hour_am_pm(latch, 0);
                if (new_data >= 32) {
                    new_data -= 32;
                    new_data /= 10;
                    new_data *= 10;
                    new_data += (LIMIT_9(real_data) + 32);
                } else {
                    new_data /= 10;
                    new_data *= 10;
                    new_data += LIMIT_9(real_data);
                }
                if (context->stop) {
                    context->latch = rtc_set_latched_hour_am_pm(new_data, latch, 0);
                } else {
                    context->offset[0] = rtc_set_hour_am_pm(new_data, context->offset[0], 0);
                }
            }
            break;
        case RTC58321A_REGISTER_10HOURS:
            if (real_data & 8) {
                new_data = rtc_get_hour(latch, 0);
                new_data %= 10;
                new_data += ((real_data & 3) * 10);
                context->hour24 = 1;
                if (context->stop) {
                    context->latch = rtc_set_latched_hour(new_data, latch, 0);
                } else {
                    context->offset[0] = rtc_set_hour(new_data, context->offset[0], 0);
                }
            } else {
                real_data &= 7;
                new_data = rtc_get_hour_am_pm(latch, 0);
                if (new_data >= 32) {
                    new_data -= 32;
                }
                new_data %= 10;
                new_data += ((real_data & 3) * 10);
                if (real_data & 4) {
                    new_data += 32;
                }
                context->hour24 = 0;
                if (context->stop) {
                    context->latch = rtc_set_latched_hour_am_pm(new_data, latch, 0);
                } else {
                    context->offset[0] = rtc_set_hour_am_pm(new_data, context->offset[0], 0);
                }
            }
            break;
        case RTC58321A_REGISTER_WEEKDAYS:
            if (context->stop) {
                context->latch = rtc_set_latched_weekday(((real_data + 1) & 7), latch);
            } else {
                context->offset[0] = rtc_set_weekday(((real_data + 1) & 7), context->offset[0]);
            }
            break;
        case RTC58321A_REGISTER_MONTHDAYS:
            new_data = rtc_get_day_of_month(latch, 0);
            new_data /= 10;
            new_data *= 10;
            new_data += LIMIT_9(real_data);
            if (context->stop) {
                context->latch = rtc_set_latched_day_of_month(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_day_of_month(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_10MONTHDAYS:
            new_data = rtc_get_day_of_month(latch, 0);
            new_data %= 10;
            new_data += ((real_data & 3) * 10);
            if (context->stop) {
                context->latch = rtc_set_latched_day_of_month(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_day_of_month(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_MONTHS:
            new_data = rtc_get_month(latch, 0);
            new_data /= 10;
            new_data *= 10;
            new_data += LIMIT_9(real_data);
            if (context->stop) {
                context->latch = rtc_set_latched_month(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_month(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_10MONTHS:
            new_data = rtc_get_month(latch, 0);
            new_data %= 10;
            new_data += ((real_data & 1) * 10);
            if (context->stop) {
                context->latch = rtc_set_latched_month(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_month(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_YEARS:
            new_data = rtc_get_year(latch, 0);
            new_data /= 10;
            new_data *= 10;
            new_data += LIMIT_9(real_data);
            if (context->stop) {
                context->latch = rtc_set_latched_year(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_year(new_data, context->offset[0], 0);
            }
            break;
        case RTC58321A_REGISTER_10YEARS:
            new_data = rtc_get_year(latch, 0);
            new_data %= 10;
            new_data += (LIMIT_9(real_data) * 10);
            if (context->stop) {
                context->latch = rtc_set_latched_year(new_data, latch, 0);
            } else {
                context->offset[0] = rtc_set_year(new_data, context->offset[0], 0);
            }
            break;
    }
}

void rtc58321_stop_clock(rtc_58321a_t *context)
{
    if (!context->stop) {
        context->stop = 1;
        context->latch = rtc_get_latch(context->offset[0]);
    }
}

void rtc58321_start_clock(rtc_58321a_t *context)
{
    if (context->stop) {
        context->stop = 0;
        context->offset[0] = context->offset[0] - (rtc_get_latch(0) - (context->latch - context->offset[0]));
    }
}
