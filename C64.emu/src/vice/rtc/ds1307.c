/*
 * ds1307.c - DS1307 RTC emulation.
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

#include "ds1307.h"
#include "lib.h"
#include "rtc.h"
#include "snapshot.h"

#include <time.h>
#include <string.h>

/* The DS1307 is an I2C based RTC, it has the following features:
 * - Real-Time Clock Counts Seconds, Minutes, Hours, Date of the Month,
 *   Month, Day of the Week, and Year
 * - 56 x 8 Battery-Backed General-Purpose RAM
 * - 24/12h mode with AM/PM indicator in 12h mode
 * - Clock Halt flag
 * - All clock registers are in BCD format
 */

/* The DS1306 has the following clock registers:
 *
 * register 0 : bit  7   Clock Halt
 *              bits 6-4 10 seconds
 *              bits 3-0 seconds
 *
 * register 1 : bit  7   0
 *              bits 6-4 10 minutes
 *              bits 3-0 minutes
 *
 * register 2 : bit  7   0
 *              bit  6   12/24 hour mode flag, 0 = 24 hour mode, 1 = 12 hour mode
 *              bit  5   in 12 hour mode this is the AM/PM flag, 0 = AM, 1 = PM
 *                       in 24 hour mode this is the msb of the 10 hours
 *              bit  4   lsb of 10 hours
 *              bits 3-0 hours
 *
 * register 3 : bits 7-3 0
 *              bits 2-0 days (of week)
 *
 * register 4 : bits 7-6 0
 *              bits 5-4 10 days (of month)
 *              bits 3-0 days (of month)
 *
 * register 5 : bits 7-5 0
 *              bit  4   10 months
 *              bits 3-0 months
 *
 * register 6 : bits 7-4 10 years
 *              bits 3-0 years
 *
 * register 7 : bit 7    OUT (emulated as RAM bit)
 *              bits 6-5 0
 *              bit 4    SQWE (emulated as RAM bit)
 *              bits 3-2 0
 *              bit 1    RS1 (emulated as RAM bit)
 *              bit 0    RS0 (emulated as RAM bit)
 *
 * registers 8-64: RAM
 */

/* This module is currently used in the following emulated hardware:
   - userport RTC (DS1307) device
 */

/* ---------------------------------------------------------------------------------------------------- */

#define DS1307_RAM_SIZE   56
#define DS1307_REG_SIZE   8

struct rtc_ds1307_s {
    int clock_halt;
    time_t clock_halt_latch;
    int am_pm;
    time_t latch;
    time_t offset;
    time_t old_offset;
    BYTE *clock_regs;
    BYTE old_clock_regs[DS1307_REG_SIZE];
    BYTE clock_regs_for_read[DS1307_REG_SIZE];
    BYTE *ram;
    BYTE old_ram[DS1307_RAM_SIZE];
    BYTE state;
    BYTE reg;
    BYTE reg_ptr;
    BYTE bit;
    BYTE io_byte;
    BYTE sclk_line;
    BYTE data_line;
    BYTE clock_register;
    char *device;
};

#define DS1307_REG_SECONDS_CH      0
#define DS1307_REG_MINUTES         1
#define DS1307_REG_HOURS           2
#define DS1307_REG_DAYS_OF_WEEK    3
#define DS1307_REG_DAYS_OF_MONTH   4
#define DS1307_REG_MONTHS          5
#define DS1307_REG_YEARS           6
#define DS1307_REG_CONTROL         7

#define DS1307_IDLE               0
#define DS1307_GET_ADDRESS        1
#define DS1307_GET_REG_NR         2
#define DS1307_READ_REGS          3
#define DS1307_WRITE_REGS         4
#define DS1307_ADDRESS_READ_ACK   5
#define DS1307_ADDRESS_WRITE_ACK  6
#define DS1307_REG_NR_ACK         7
#define DS1307_WRITE_ACK          8
#define DS1307_READ_ACK           9
#define DS1307_START_WAIT         10

/* ---------------------------------------------------------------------------------------------------- */

rtc_ds1307_t *ds1307_init(char *device)
{
    rtc_ds1307_t *retval = lib_calloc(1, sizeof(rtc_ds1307_t));
    int loaded = rtc_load_context(device, DS1307_RAM_SIZE, DS1307_REG_SIZE);

    if (loaded) {
        retval->ram = rtc_get_loaded_ram();
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->ram = lib_calloc(1, DS1307_RAM_SIZE);
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, DS1307_REG_SIZE);
    }
    memcpy(retval->old_ram, retval->ram, DS1307_RAM_SIZE);
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, DS1307_REG_SIZE);

    retval->device = lib_stralloc(device);
    retval->state = DS1307_IDLE;
    retval->sclk_line = 1;
    retval->data_line = 1;
    retval->reg_ptr = 0;

    return retval;
}

void ds1307_destroy(rtc_ds1307_t *context, int save)
{
    if (save) {
        if (memcmp(context->ram, context->old_ram, DS1307_RAM_SIZE) ||
            memcmp(context->clock_regs, context->old_clock_regs, DS1307_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(context->ram, DS1307_RAM_SIZE, context->clock_regs, DS1307_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->ram);
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static void ds1307_i2c_start(rtc_ds1307_t *context)
{
    BYTE tmp;
    time_t latch = (context->clock_halt) ? context->clock_halt_latch : rtc_get_latch(context->offset);

    tmp = context->clock_halt << 7;
    tmp |= rtc_get_second(latch, 1);
    context->clock_regs_for_read[DS1307_REG_SECONDS_CH] = tmp;
    context->clock_regs_for_read[DS1307_REG_MINUTES] = rtc_get_minute(latch, 1);
    tmp = context->am_pm << 6;
    if (context->am_pm) {
        tmp |= rtc_get_hour_am_pm(latch, 1);
    } else {
        tmp |= rtc_get_hour(latch, 1);
    }
    context->clock_regs_for_read[DS1307_REG_HOURS] = tmp;
    context->clock_regs_for_read[DS1307_REG_DAYS_OF_WEEK] = rtc_get_weekday(latch) + 1;
    context->clock_regs_for_read[DS1307_REG_DAYS_OF_MONTH] = rtc_get_day_of_month(latch, 1);
    context->clock_regs_for_read[DS1307_REG_MONTHS] = rtc_get_month(latch, 1);
    context->clock_regs_for_read[DS1307_REG_YEARS] = rtc_get_year(latch, 1);
    context->clock_regs_for_read[DS1307_REG_CONTROL] = context->clock_regs[DS1307_REG_CONTROL];
}

static BYTE ds1307_read_register(rtc_ds1307_t *context, BYTE addr)
{
    if (addr < DS1307_REG_SIZE) {
        return context->clock_regs_for_read[addr];
    }
    return context->ram[addr - DS1307_REG_SIZE];
}

static void ds1307_write_register(rtc_ds1307_t *context, BYTE addr, BYTE val)
{
    switch (addr) {
        case DS1307_REG_MINUTES:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_minute(val, context->offset, 1);
            }
            break;
        case DS1307_REG_DAYS_OF_MONTH:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_day_of_month(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_day_of_month(val, context->offset, 1);
            }
            break;
        case DS1307_REG_MONTHS:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_month(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_month(val, context->offset, 1);
            }
            break;
        case DS1307_REG_DAYS_OF_WEEK:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_weekday(val - 1, context->clock_halt_latch);
            } else {
                context->offset = rtc_set_weekday(val - 1, context->offset);
            }
            break;
        case DS1307_REG_YEARS:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_year(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_year(val, context->offset, 1);
            }
            break;
        case DS1307_REG_CONTROL:
            context->clock_regs[DS1307_REG_CONTROL] = (val & 0x93);
            break;
        case DS1307_REG_HOURS:
            if (val & 0x40) {
                if (context->clock_halt) {
                    context->clock_halt_latch = rtc_set_latched_hour_am_pm(val & 0x3f, context->clock_halt_latch, 1);
                } else {
                    context->offset = rtc_set_hour_am_pm(val & 0x3f, context->offset, 1);
                }
                context->am_pm = 1;
            } else {
                if (context->clock_halt) {
                    context->clock_halt_latch = rtc_set_latched_hour(val & 0x3f, context->clock_halt_latch, 1);
                } else {
                    context->offset = rtc_set_hour(val & 0x3f, context->offset, 1);
                }
                context->am_pm = 0;
            }
            break;
        case DS1307_REG_SECONDS_CH:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_second(val & 0x7f, context->clock_halt_latch, 1);
                if (!(val & 0x80)) {
                    context->offset = context->offset - (rtc_get_latch(0) - (context->clock_halt_latch - context->offset));
                    context->clock_halt = 0;
                }
            } else {
                context->offset = rtc_set_second(val & 0x7f, context->offset, 1);
                if (val & 0x80) {
                    context->clock_halt = 1;
                    context->clock_halt_latch = rtc_get_latch(context->offset);
                }
            }
            break;
        default:
            context->ram[addr - DS1307_REG_SIZE] = val;
    }
}

static void ds1307_next_address_bit(rtc_ds1307_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        if (context->reg == 0xd0) {
            context->state = DS1307_ADDRESS_WRITE_ACK;
        } else if (context->reg == 0xd1) {
            context->state = DS1307_ADDRESS_READ_ACK;
        } else {
            context->state = DS1307_IDLE;
        }
    }
}

static void ds1307_next_reg_nr_bit(rtc_ds1307_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        context->state = DS1307_REG_NR_ACK;
        context->reg_ptr = context->reg & 0x3f;
    }
}

static void ds1307_next_read_bit(rtc_ds1307_t *context)
{
    ++context->bit;
    if (context->bit == 8) {
        context->state = DS1307_READ_ACK;
    }
}

static void ds1307_next_write_bit(rtc_ds1307_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        ds1307_write_register(context, context->reg_ptr, context->reg);
        context->state = DS1307_WRITE_ACK;
        ++context->reg_ptr;
        context->reg_ptr &= 0x3f;
    }
}

static void ds1307_validate_read_ack(rtc_ds1307_t *context)
{
    if (!context->data_line) {
        context->state = DS1307_READ_REGS;
        context->bit = 0;
        ++context->reg_ptr;
        context->reg_ptr &= 0x3f;
        context->reg = ds1307_read_register(context, context->reg_ptr);
    } else {
        context->state = DS1307_IDLE;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void ds1307_set_clk_line(rtc_ds1307_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->sclk_line == val) {
        return;
    }

    if (!val) {
        switch (context->state) {
            case DS1307_START_WAIT:
                context->state = DS1307_GET_ADDRESS;
                break;
            case DS1307_GET_ADDRESS:
                ds1307_next_address_bit(context);
                break;
            case DS1307_GET_REG_NR:
                ds1307_next_reg_nr_bit(context);
                break;
            case DS1307_READ_REGS:
                ds1307_next_read_bit(context);
                break;
            case DS1307_WRITE_REGS:
                ds1307_next_write_bit(context);
                break;
            case DS1307_READ_ACK:
                ds1307_validate_read_ack(context);
                break;
            case DS1307_ADDRESS_READ_ACK:
                context->state = DS1307_READ_REGS;
                context->reg = ds1307_read_register(context, context->reg_ptr);
                context->bit = 0;
                break;
            case DS1307_REG_NR_ACK:
            case DS1307_WRITE_ACK:
                context->state = DS1307_WRITE_REGS;
                context->reg = 0;
                context->bit = 0;
                break;
            case DS1307_ADDRESS_WRITE_ACK:
                context->state = DS1307_GET_REG_NR;
                context->reg = 0;
                context->bit = 0;
                break;
        }
    }
    context->sclk_line = val;
}

void ds1307_set_data_line(rtc_ds1307_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->data_line == val) {
        return;
    }

    if (context->sclk_line) {
        if (val) {
            context->state = DS1307_IDLE;
        } else {
            ds1307_i2c_start(context);
            context->state = DS1307_START_WAIT;
            context->reg = 0;
            context->bit = 0;
        }
    }
    context->data_line = val;
}

BYTE ds1307_read_data_line(rtc_ds1307_t *context)
{
	switch (context->state) {
        case DS1307_READ_REGS:
            return (context->reg & (1 << (7 - context->bit))) >> (7 - context->bit);
        case DS1307_ADDRESS_READ_ACK:
        case DS1307_ADDRESS_WRITE_ACK:
        case DS1307_REG_NR_ACK:
        case DS1307_READ_ACK:
        case DS1307_WRITE_ACK:
            return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------------------------------- */

/* RTC_DS1307 snapshot module format:

   type   | name                | description
   ------------------------------------------
   BYTE   | clock halt          | clock halt flag
   DWORD  | clock halt latch hi | high DWORD of clock halt offset
   DWORD  | clock halt latch lo | low DWORD of clock halt offset
   BYTE   | am pm               | AM/PM flag
   DWORD  | latch hi            | high DWORD of latch offset
   DWORD  | latch lo            | low DWORD of latch offset
   DWORD  | offset hi           | high DWORD of RTC offset
   DWORD  | offset lo           | low DWORD of RTC offset
   DWORD  | old offset hi       | high DWORD of old RTC offset
   DWORD  | old offset lo       | low DWORD of old RTC offset
   ARRAY  | clock regs          | 8 BYTES of register data
   ARRAY  | old clock regs      | 8 BYTES of old register data
   ARRAY  | clock regs for read | 8 BYTES of register read data
   ARRAY  | RAM                 | 56 BYTES of RAM data
   ARRAY  | old RAM             | 56 BYTES of old RAM data
   BYTE   | state               | current state
   BYTE   | reg                 | current register
   BYTE   | reg ptr             | register pointer
   BYTE   | bit                 | current bit
   BYTE   | io byte             | current I/O BYTE
   BYTE   | sclk                | SCLK line state
   BYTE   | data                | DATA line state
   BYTE   | clock register      | clock register flag
   STRING | device              | device name STRING
 */

static char snap_module_name[] = "RTC_DS1307";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ds1307_write_snapshot(rtc_ds1307_t *context, snapshot_t *s)
{
    DWORD clock_halt_latch_lo = 0;
    DWORD clock_halt_latch_hi = 0;
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
        || SMW_B(m, (BYTE)context->am_pm) < 0
        || SMW_DW(m, latch_hi) < 0
        || SMW_DW(m, latch_lo) < 0
        || SMW_DW(m, offset_hi) < 0
        || SMW_DW(m, offset_lo) < 0
        || SMW_DW(m, old_offset_hi) < 0
        || SMW_DW(m, old_offset_lo) < 0
        || SMW_BA(m, context->clock_regs, DS1307_REG_SIZE) < 0
        || SMW_BA(m, context->old_clock_regs, DS1307_REG_SIZE) < 0
        || SMW_BA(m, context->clock_regs_for_read, DS1307_REG_SIZE) < 0
        || SMW_BA(m, context->ram, DS1307_RAM_SIZE) < 0
        || SMW_BA(m, context->old_ram, DS1307_RAM_SIZE) < 0
        || SMW_B(m, context->state) < 0
        || SMW_B(m, context->reg) < 0
        || SMW_B(m, context->reg_ptr) < 0
        || SMW_B(m, context->bit) < 0
        || SMW_B(m, context->io_byte) < 0
        || SMW_B(m, context->sclk_line) < 0
        || SMW_B(m, context->data_line) < 0
        || SMW_B(m, context->clock_register) < 0
        || SMW_STR(m, context->device) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

int ds1307_read_snapshot(rtc_ds1307_t *context, snapshot_t *s)
{
    DWORD clock_halt_latch_lo = 0;
    DWORD clock_halt_latch_hi = 0;
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
        || SMR_B_INT(m, &context->am_pm) < 0
        || SMR_DW(m, &latch_hi) < 0
        || SMR_DW(m, &latch_lo) < 0
        || SMR_DW(m, &offset_hi) < 0
        || SMR_DW(m, &offset_lo) < 0
        || SMR_DW(m, &old_offset_hi) < 0
        || SMR_DW(m, &old_offset_lo) < 0
        || SMR_BA(m, context->clock_regs, DS1307_REG_SIZE) < 0
        || SMR_BA(m, context->old_clock_regs, DS1307_REG_SIZE) < 0
        || SMR_BA(m, context->clock_regs_for_read, DS1307_REG_SIZE) < 0
        || SMR_BA(m, context->ram, DS1307_RAM_SIZE) < 0
        || SMR_BA(m, context->old_ram, DS1307_RAM_SIZE) < 0
        || SMR_B(m, &context->state) < 0
        || SMR_B(m, &context->reg) < 0
        || SMR_B(m, &context->reg_ptr) < 0
        || SMR_B(m, &context->bit) < 0
        || SMR_B(m, &context->io_byte) < 0
        || SMR_B(m, &context->sclk_line) < 0
        || SMR_B(m, &context->data_line) < 0
        || SMR_B(m, &context->clock_register) < 0
        || SMR_STR(m, &context->device) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

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

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
