/*
 * bq4830y.c - BQ4830Y RTC emulation.
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

#include "bq4830y.h"
#include "lib.h"
#include "rtc.h"
#include "snapshot.h"

#include <string.h>

/* The BQ4830Y is a RAM + RTC module, it can be used in place of a RAM
 * and the RTC registers can be accessed at $7FF8-$7FFF and it has the
 * following features:
 * - Real-Time Clock Counts Seconds, Minutes, Hours, Date of the Month,
 *   Month, Day of the Week, and Year with Leap-Year,
 *   Compensation Valid Up to 2100
 * - 32760 x 8 Battery-Backed General-Purpose RAM
 * - Clock Halt flag
 * - Calibration register
 * - Frequency Test bit
 * - All clock registers are in BCD format
 */

/* The BQ4830Y has the following clock registers:
 *
 * register $7FF8 : bit  7   Write to clock  (halts updates to clock registers)
 *                  bit  6   Read from clock (halts updates to clock registers)
 *                  bit  5   Direction of correction for calibration (emulated as a RAM bit)
 *                  bits 4-0 Calibration (emulated as RAM bits)
 *
 * register $7FF9 : bit  7   Clock Halt
 *                  bits 6-4 10 seconds
 *                  bits 3-0 seconds
 *
 * register $7FFA : bit  7   RAM bit
 *                  bits 6-4 10 minutes
 *                  bits 3-0 minutes
 *
 * register $7FFB : bits 7-6 RAM bits
 *                  bits 5-4 10 hours
 *                  bits 3-0 hours
 *
 * register $7FFC : bit  7   RAM bit
 *                  bit  6   Test Frequency bit (emulated as RAM bit)
 *                  bits 5-3 RAM bits
 *                  bits 2-0 days (of week)
 *
 * register $7FFD : bits 7-6 RAM bits
 *                  bits 5-4 10 days (of month)
 *                  bits 3-0 days (of month)
 *
 * register $7FFE : bits 7-5 RAM bits
 *                  bit  4   10 months
 *                  bits 3-0 months
 *
 * register $7FFF : bits 7-4 10 years
 *                  bits 3-0 years
 */

/* This module is currently used in the following emulated hardware:
   - C128 internal/external function RAM+RTC expansion
 */

/* ---------------------------------------------------------------------------------------------------- */

rtc_bq4830y_t *bq4830y_init(char *device)
{
    rtc_bq4830y_t *retval = lib_calloc(1, sizeof(rtc_bq4830y_t));
    int loaded = rtc_load_context(device, BQ4830Y_RAM_SIZE, BQ4830Y_REG_SIZE);

    if (loaded) {
        retval->ram = rtc_get_loaded_ram();
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->ram = lib_calloc(1, BQ4830Y_RAM_SIZE);
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, BQ4830Y_REG_SIZE);
    }
    memcpy(retval->old_ram, retval->ram, BQ4830Y_RAM_SIZE);
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, BQ4830Y_REG_SIZE);

    retval->device = lib_stralloc(device);

    return retval;
}

void bq4830y_destroy(rtc_bq4830y_t *context, int save)
{
    if (save) {
        if (memcmp(context->ram, context->old_ram, BQ4830Y_RAM_SIZE) ||
            memcmp(context->clock_regs, context->old_clock_regs, BQ4830Y_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(context->ram, BQ4830Y_RAM_SIZE, context->clock_regs, BQ4830Y_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->ram);
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static void bq4830y_latch_write_regs(rtc_bq4830y_t *context)
{
    int i;

    context->clock_regs[BQ4830Y_REG_SECONDS & 7] &= 0x80;
    context->clock_regs[BQ4830Y_REG_SECONDS & 7] |= rtc_get_second(context->latch, 1);
    context->clock_regs[BQ4830Y_REG_MINUTES & 7] &= 0x80;
    context->clock_regs[BQ4830Y_REG_MINUTES & 7] |= rtc_get_minute(context->latch, 1);
    context->clock_regs[BQ4830Y_REG_HOURS & 7] &= 0xc0;
    context->clock_regs[BQ4830Y_REG_HOURS & 7] |= rtc_get_hour(context->latch, 1);
    context->clock_regs[BQ4830Y_REG_DAYS_OF_WEEK & 7] &= 0xf8;
    context->clock_regs[BQ4830Y_REG_DAYS_OF_WEEK & 7] |= rtc_get_weekday(context->latch) + 1;
    context->clock_regs[BQ4830Y_REG_DAYS_OF_MONTH & 7] &= 0xc0;
    context->clock_regs[BQ4830Y_REG_DAYS_OF_MONTH & 7] |= rtc_get_day_of_month(context->latch, 1);
    context->clock_regs[BQ4830Y_REG_MONTHS & 7] &= 0xe0;
    context->clock_regs[BQ4830Y_REG_MONTHS & 7] |= rtc_get_month(context->latch, 1);
    context->clock_regs[BQ4830Y_REG_YEARS & 7] = rtc_get_year(context->latch, 1);
    for (i = 0; i < 8; i++) {
        context->clock_regs_changed[i] = 0;
    }
}

static void bq4830y_write_clock_data(rtc_bq4830y_t *context)
{
    int val;

    if (context->clock_halt) {
        if (context->clock_regs_changed[BQ4830Y_REG_YEARS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_YEARS & 7];
            context->clock_halt_latch = rtc_set_latched_year(val, context->clock_halt_latch, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_MONTHS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_MONTHS & 7] & 0x1f;
            context->clock_halt_latch = rtc_set_latched_month(val, context->clock_halt_latch, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_DAYS_OF_MONTH & 7]) {
            val = context->clock_regs[BQ4830Y_REG_DAYS_OF_MONTH & 7] & 0x3f;
            context->clock_halt_latch = rtc_set_latched_day_of_month(val, context->clock_halt_latch, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_DAYS_OF_WEEK & 7]) {
            val = (context->clock_regs[BQ4830Y_REG_DAYS_OF_WEEK & 7] & 7) - 1;
            context->clock_halt_latch = rtc_set_latched_weekday(val, context->clock_halt_latch);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_HOURS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_HOURS & 7] & 0x3f;
            context->clock_halt_latch = rtc_set_latched_hour(val, context->clock_halt_latch, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_MINUTES & 7]) {
            val = context->clock_regs[BQ4830Y_REG_MINUTES & 7] & 0x7f;
            context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_SECONDS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_SECONDS & 7] & 0x7f;
            context->clock_halt_latch = rtc_set_latched_second(val, context->clock_halt_latch, 1);
        }
    } else {
        if (context->clock_regs_changed[BQ4830Y_REG_YEARS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_YEARS & 7];
            context->offset = rtc_set_year(val, context->offset, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_MONTHS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_MONTHS & 7] & 0x1f;
            context->offset = rtc_set_month(val, context->offset, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_DAYS_OF_MONTH & 7]) {
            val = context->clock_regs[BQ4830Y_REG_DAYS_OF_MONTH & 7] & 0x3f;
            context->offset = rtc_set_day_of_month(val, context->offset, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_DAYS_OF_WEEK & 7]) {
            val = (context->clock_regs[BQ4830Y_REG_DAYS_OF_WEEK & 7] & 7) - 1;
            context->offset = rtc_set_weekday(val, context->offset);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_HOURS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_HOURS & 7] & 0x3f;
            context->offset = rtc_set_hour(val, context->offset, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_MINUTES & 7]) {
            val = context->clock_regs[BQ4830Y_REG_MINUTES & 7] & 0x7f;
            context->offset = rtc_set_minute(val, context->offset, 1);
        }
        if (context->clock_regs_changed[BQ4830Y_REG_SECONDS & 7]) {
            val = context->clock_regs[BQ4830Y_REG_SECONDS & 7] & 0x7f;
            context->offset = rtc_set_second(val, context->offset, 1);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void bq4830y_store(rtc_bq4830y_t *context, WORD address, BYTE val)
{
    int latch_state = context->read_latch | (context->write_latch << 1);

    switch (address & 0x7fff) {
        case BQ4830Y_REG_MINUTES:
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            } else {
                context->clock_regs[address & 7] &= 0x7f;
                context->clock_regs[address & 7] |= val & 0x80;
            }
            break;
        case BQ4830Y_REG_HOURS:
        case BQ4830Y_REG_DAYS_OF_MONTH:
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            } else {
                context->clock_regs[address & 7] &= 0x3f;
                context->clock_regs[address & 7] |= val & 0xc0;
            }
            break;
        case BQ4830Y_REG_DAYS_OF_WEEK:
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            } else {
                context->clock_regs[address & 7] &= 7;
                context->clock_regs[address & 7] |= val & 0xf8;
            }
            break;
        case BQ4830Y_REG_MONTHS:
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            } else {
                context->clock_regs[address & 7] &= 0x1f;
                context->clock_regs[address & 7] |= val & 0xe0;
            }
            break;
        case BQ4830Y_REG_YEARS:
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            }
            break;
        case BQ4830Y_REG_SECONDS:
            context->clock_regs[address & 7] &= 0x7f;
            context->clock_regs[address & 7] |= val & 0x80;
            if (context->write_latch) {
                context->clock_regs[address & 7] = val;
                context->clock_regs_changed[address & 7] = 1;
            } else {
                context->clock_regs[address & 7] &= 0x7f;
                context->clock_regs[address & 7] |= val & 0x80;
            }
            if ((val >> 7) != context->clock_halt) {
                if (val & 0x80) {
                    context->clock_halt_latch = rtc_get_latch(context->offset);
                    context->clock_halt = 1;
                } else {
                    context->offset = context->offset - (rtc_get_latch(0) - (context->clock_halt_latch - context->offset));
                    context->clock_halt = 0;
                }
            }
            break;
        case BQ4830Y_REG_CONTROL:
            context->clock_regs[address & 7] &= 0xc0;
            context->clock_regs[address & 7] |= val & 0x3f;
            switch (val >> 6) {
                case LATCH_NONE:
                    switch (latch_state) {
                        case READ_LATCH:
                            context->read_latch = 0;
                            break;
                        case WRITE_LATCH:
                            bq4830y_write_clock_data(context);
                            context->write_latch = 0;
                            break;
                        case READ_WRITE_LATCH:
                            bq4830y_write_clock_data(context);
                            context->read_latch = 0;
                            context->write_latch = 0;
                            break;
                    }
                    break;
                case READ_LATCH:
                    switch (latch_state) {
                        case WRITE_LATCH:
                            bq4830y_write_clock_data(context);
                            context->write_latch = 0;
                        /* fall through */
                        case LATCH_NONE:
                            if (context->clock_halt) {
                                context->latch = context->clock_halt_latch;
                            } else {
                                context->latch = rtc_get_latch(context->offset);
                            }
                            context->read_latch = 1;
                            break;
                        case READ_WRITE_LATCH:
                            bq4830y_write_clock_data(context);
                            context->write_latch = 0;
                            break;
                    }
                    break;
                case WRITE_LATCH:
                    switch (latch_state) {
                        case READ_LATCH:
                            context->read_latch = 0;
                        /* fall through */
                        case LATCH_NONE:
                            if (context->clock_halt) {
                                context->latch = context->clock_halt_latch;
                            } else {
                                context->latch = rtc_get_latch(context->offset);
                            }
                            bq4830y_latch_write_regs(context);
                            context->write_latch = 1;
                            break;
                        case READ_WRITE_LATCH:
                            context->read_latch = 0;
                            break;
                    }
                    break;
                case READ_WRITE_LATCH:
                    switch (latch_state) {
                        case LATCH_NONE:
                            if (context->clock_halt) {
                                context->latch = context->clock_halt_latch;
                            } else {
                                context->latch = rtc_get_latch(context->offset);
                            }
                            context->read_latch = 1;
                            bq4830y_latch_write_regs(context);
                            context->write_latch = 1;
                            break;
                        case READ_LATCH:
                            bq4830y_latch_write_regs(context);
                            context->write_latch = 1;
                            break;
                        case WRITE_LATCH:
                            context->read_latch = 1;
                            break;
                    }
                    break;
            }
            break;
        default:
            context->ram[address] = val;
            break;
    }
}

BYTE bq4830y_read(rtc_bq4830y_t *context, WORD address)
{
    BYTE retval;
    int latch_state = context->read_latch | (context->write_latch << 1) | (context->clock_halt << 2);
    time_t latch;

    if (latch_state != LATCH_NONE) {
        if (!context->clock_halt) {
            latch = context->latch;
        } else {
            latch = context->clock_halt_latch;
        }
    } else {
        latch = rtc_get_latch(context->offset);
    }

    switch (address & 0x7fff) {
        case BQ4830Y_REG_MINUTES:
            retval = context->clock_regs[address & 7] & 0x80;
            retval |= rtc_get_minute(latch, 1);
            break;
        case BQ4830Y_REG_HOURS:
            retval = context->clock_regs[address & 7] & 0xc0;
            retval |= rtc_get_hour(latch, 1);
            break;
        case BQ4830Y_REG_DAYS_OF_WEEK:
            retval = context->clock_regs[address & 7] & 0xf8;
            retval |= rtc_get_weekday(latch) + 1;
            break;
        case BQ4830Y_REG_DAYS_OF_MONTH:
            retval = context->clock_regs[address & 7] & 0xc0;
            retval |= rtc_get_day_of_month(latch, 1);
            break;
        case BQ4830Y_REG_MONTHS:
            retval = context->clock_regs[address & 7] & 0xe0;
            retval |= rtc_get_month(latch, 1);
            break;
        case BQ4830Y_REG_YEARS:
            retval = rtc_get_year(latch, 1);
            break;
        case BQ4830Y_REG_CONTROL:
            retval = context->clock_regs[address & 7] & 0x3f;
            retval |= (context->write_latch << 7);
            retval |= (context->read_latch << 6);
            break;
        case BQ4830Y_REG_SECONDS:
            retval = context->clock_halt << 7;
            retval |= rtc_get_second(latch, 1);
            break;
        default:
            retval = context->ram[address];
    }
    return retval;
}

/* ---------------------------------------------------------------------------------------------------- */

/* RTC_BQ4830Y snapshot module format:

   type   | name                | description
   ------------------------------------------
   BYTE   | clock halt          | clock halt flag
   DWORD  | clock halt latch hi | high DWORD of clock halt offset
   DWORD  | clock halt latch lo | low DWORD of clock halt offset
   BYTE   | read latch          | read latch flag
   BYTE   | write latch         | write latch flag
   DWORD  | latch hi            | high DWORD of the read/write offset
   DWORD  | latch lo            | low DWORD of the read/write offset
   DWORD  | offset hi           | high DWORD of the RTC offset
   DWORD  | offset lo           | low DWORD of the RTC offset
   DWORD  | old offset hi       | high DWORD of the old RTC offset
   DWORD  | old offset lo       | low DWORD of the old RTC offset
   ARRAY  | clock regs          | 8 BYTES of register data
   ARRAY  | old clock regs      | 8 BYTES of old register data
   ARRAY  | clock regs changed  | 8 BYTES of changed register data
   ARRAY  | RAM                 | 32768 BYTES of RAM data
   ARRAY  | old RAM             | 32768 BYTES of old RAM data
   STRING | device              | device name STRING
 */

static char snap_module_name[] = "RTC_BQ4830Y";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int bq4830y_write_snapshot(rtc_bq4830y_t *context, snapshot_t *s)
{
    DWORD clock_halt_latch_hi = 0;
    DWORD clock_halt_latch_lo = 0;
    DWORD latch_lo = 0;
    DWORD latch_hi = 0;
    DWORD offset_lo = 0;
    DWORD offset_hi = 0;
    DWORD old_offset_lo = 0;
    DWORD old_offset_hi = 0;
    snapshot_module_t *m;

    /* time_t can be either 32bit or 64bit, so we save as 64bit */
#if (SIZE_OF_TIME_T == 8)
    clock_halt_latch_hi = (DWORD)(context->clock_halt_latch >> 32);
    clock_halt_latch_lo = (DWORD)(context->clock_halt_latch & 0xffffffff);
    latch_hi = (DWORD)(context->latch >> 32);
    latch_lo = (DWORD)(context->latch & 0xffffffff);
    offset_hi = (DWORD)(context->offset >> 32);
    offset_lo = (DWORD)(context->offset & 0xffffffff);
    old_offset_hi = (DWORD)(context->old_offset >> 32);
    old_offset_lo = (DWORD)(context->old_offset & 0xffffffff);
#else
    clock_halt_latch_lo = (DWORD)context->clock_halt_latch;
    latch_lo = (DWORD)context->latch;
    offset_lo = (DWORD)context->offset;
    old_offset_lo = (DWORD)context->old_offset;
#endif

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)context->clock_halt) < 0
        || SMW_DW(m, clock_halt_latch_hi) < 0
        || SMW_DW(m, clock_halt_latch_lo) < 0
        || SMW_B(m, (BYTE)context->read_latch) < 0
        || SMW_B(m, (BYTE)context->write_latch) < 0
        || SMW_DW(m, latch_hi) < 0
        || SMW_DW(m, latch_lo) < 0
        || SMW_DW(m, offset_hi) < 0
        || SMW_DW(m, offset_lo) < 0
        || SMW_DW(m, old_offset_hi) < 0
        || SMW_DW(m, old_offset_lo) < 0
        || SMW_BA(m, context->clock_regs, BQ4830Y_REG_SIZE) < 0
        || SMW_BA(m, context->old_clock_regs, BQ4830Y_REG_SIZE) < 0
        || SMW_BA(m, context->clock_regs_changed, BQ4830Y_REG_SIZE) < 0
        || SMW_BA(m, context->ram, BQ4830Y_RAM_SIZE) < 0
        || SMW_BA(m, context->old_ram, BQ4830Y_RAM_SIZE) < 0
        || SMW_STR(m, context->device) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

int bq4830y_read_snapshot(rtc_bq4830y_t *context, snapshot_t *s)
{
    DWORD clock_halt_latch_hi = 0;
    DWORD clock_halt_latch_lo = 0;
    DWORD latch_lo = 0;
    DWORD latch_hi = 0;
    DWORD offset_lo = 0;
    DWORD offset_hi = 0;
    DWORD old_offset_lo = 0;
    DWORD old_offset_hi = 0;
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B_INT(m, &context->clock_halt) < 0
        || SMR_DW(m, &clock_halt_latch_hi) < 0
        || SMR_DW(m, &clock_halt_latch_lo) < 0
        || SMR_B_INT(m, &context->read_latch) < 0
        || SMR_B_INT(m, &context->write_latch) < 0
        || SMR_DW(m, &latch_hi) < 0
        || SMR_DW(m, &latch_lo) < 0
        || SMR_DW(m, &offset_hi) < 0
        || SMR_DW(m, &offset_lo) < 0
        || SMR_DW(m, &old_offset_hi) < 0
        || SMR_DW(m, &old_offset_lo) < 0
        || SMR_BA(m, context->clock_regs, BQ4830Y_REG_SIZE) < 0
        || SMR_BA(m, context->old_clock_regs, BQ4830Y_REG_SIZE) < 0
        || SMR_BA(m, context->clock_regs_changed, BQ4830Y_REG_SIZE) < 0
        || SMR_BA(m, context->ram, BQ4830Y_RAM_SIZE) < 0
        || SMR_BA(m, context->old_ram, BQ4830Y_RAM_SIZE) < 0
        || SMR_STR(m, &context->device) < 0) {
        goto fail;
    }

#if (SIZE_OF_TIME_T == 8)
    context->clock_halt_latch = (time_t)(clock_halt_latch_hi) << 32;
    context->clock_halt_latch |= clock_halt_latch_lo;
    context->latch = (time_t)(latch_hi) << 32;
    context->latch |= latch_lo;
    context->offset = (time_t)(offset_hi) << 32;
    context->offset |= offset_lo;
    context->old_offset = (time_t)(old_offset_hi) << 32;
    context->old_offset |= old_offset_lo;
#else
    context->clock_halt_latch = clock_halt_latch_lo;
    context->latch = latch_lo;
    context->offset = offset_lo;
    context->old_offset = old_offset_lo;
#endif

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
