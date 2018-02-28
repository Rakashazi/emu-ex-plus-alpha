/*
 * pcf8583.c - PCF8583 RTC emulation.
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

#include "lib.h"
#include "pcf8583.h"
#include "rtc.h"
#include "snapshot.h"

#include <time.h>
#include <string.h>

/* The PCF8583 is an I2C based RTC, it has the following features:
 * - Real-Time Clock Counts Seconds, Minutes, Hours, Date of the Month,
 *   Month, Day of the Week, and Year
 * - 56 x 8 Battery-Backed General-Purpose RAM
 * - 24/12h mode with AM/PM indicator in 12h mode
 * - Clock Halt flag
 * - 240 × 8-bit RAM
 * - Clock function with four year calendar
 * - Universal timer with alarm and overflow indication
 * - Programmable alarm, timer, and interrupt function
 */

/* The PCF8583 has the following clock registers:
 *
 * register  0 : bit  7   Clock Halt
 *               bits 6   hold last count flag, emulated as RAM bit
 *               bits 5-4 function mode, emulated as RAM bit
 *                        00 = clock mode 32.768 khz
 *                        01 = clock mode 50 hz
 *                        10 = event counter mode
 *                        11 = test modes
 *               bit  3   mask flag, emulated as RAM bit
 *               bit  2   alarm enable, emulated as RAM bit
 *               bit  1   alarm flag, emulated as RAM bit
 *               bit  0   timer flag, emulated as RAM bit
 *
 * register  1 : bits 7-4 1/10 seconds
 *               bits 3-0 1/100 seconds
 *
 * register  2 : bits 7-4 10 seconds
 *               bits 3-0 seconds
 *
 * register  3 : bit  7   0
 *               bits 6-4 10 minutes
 *               bits 3-0 minutes
 *
 * register  4 : bit  7   12/24h indicator
 *               bits 6   PM/AM flag
 *               bits 5-4 10 hours
 *               bits 3-0 hours
 *
 * register  5 : bits 7-6 years
 *               bits 5-4 10 days of month
 *               bits 3-0 days of month
 *
 * register  6 : bits 7-5 days of week
 *               bit  4   10 months
 *               bits 3-0 months
 *
 * register  7 : timer days, not emulated, always returns 0
 *
 * register  8 : alarm control, emulated as RAM
 *
 * register  9 : 100th of seconds alarm, emulated as RAM
 *
 * register 10 : seconds alarm, emulated as RAM
 *
 * register 11 : minutes alarm, emulated as RAM
 *
 * register 12 : hours alarm, emulated as RAM
 *
 * register 13 : date alarm, emulated as RAM
 *
 * register 14 : month alarm, emulated as RAM
 *
 * register 15 : timer alarm, emulated as RAM
 *
 * registers 16-255: RAM
 */

/* This module is currently used in the following emulated hardware:
   CP CLock F83
 */

/* ---------------------------------------------------------------------------------------------------- */

#define PCF8583_REG_CONTROL              0
#define PCF8583_REG_100TH_SECONDS        1
#define PCF8583_REG_SECONDS              2
#define PCF8583_REG_MINUTES              3
#define PCF8583_REG_HOURS                4
#define PCF8583_REG_YEARS_MONTH_DAYS     5
#define PCF8583_REG_WEEK_DAYS_MONTHS     6
#define PCF8583_REG_TIMER_DAYS           7
#define PCF8583_REG_ALARM_CONTROL        8
#define PCF8583_REG_100TH_SECONDS_ALARM  9
#define PCF8583_REG_SECONDS_ALARM       10
#define PCF8583_REG_MINUTES_ALARM       11
#define PCF8583_REG_HOURS_ALARM         12
#define PCF8583_REG_DATE_ALARM          13
#define PCF8583_REG_MONTH_ALARM         14
#define PCF8583_REG_TIMER_ALARM         15

/* ---------------------------------------------------------------------------------------------------- */

rtc_pcf8583_t *pcf8583_init(char *device, int read_bit_shift)
{
    rtc_pcf8583_t *retval = lib_calloc(1, sizeof(rtc_pcf8583_t));
    int loaded = rtc_load_context(device, PCF8583_RAM_SIZE, PCF8583_REG_SIZE);

    if (loaded) {
        retval->ram = rtc_get_loaded_ram();
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->ram = lib_calloc(1, PCF8583_RAM_SIZE);
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, PCF8583_REG_SIZE);
    }
    memcpy(retval->old_ram, retval->ram, PCF8583_RAM_SIZE);
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, PCF8583_REG_SIZE);

    retval->device = lib_stralloc(device);
    retval->state = PCF8583_IDLE;
    retval->sclk_line = 1;
    retval->data_line = 1;
    retval->reg_ptr = 0;
    retval->read_bit_shift = read_bit_shift;

    return retval;
}

void pcf8583_destroy(rtc_pcf8583_t *context, int save)
{
    if (save) {
        if (memcmp(context->ram, context->old_ram, PCF8583_RAM_SIZE) ||
            memcmp(context->clock_regs, context->old_clock_regs, PCF8583_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(context->ram, PCF8583_RAM_SIZE, context->clock_regs, PCF8583_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->ram);
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static BYTE register_train[9 * 20];

static void make_read_register_train(rtc_pcf8583_t *context)
{
    int i, j;

    for (i = 0; i < 9 * 20; ++i) {
        register_train[i] = 0;
    }

    for (i = 0; i < 16; ++i) {
        for (j = 7; j >= 0; --j) {
            if (((i * 9) + (7 - j) + context->read_bit_shift) >= 0) {
                register_train[(i * 9) + (7 - j) + context->read_bit_shift] = (context->clock_regs_for_read[i] & (1 << j)) ? 1 : 0;
            }
        }
        register_train[(i * 9) + 8 + context->read_bit_shift] = 0;
    }
}

static void pcf8584_next_train_bit(rtc_pcf8583_t *context)
{
    ++context->bit;
    if (context->bit == 9) {
        ++context->reg_ptr;
        context->reg_ptr &= 0x1f;
        context->bit = 0;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

static void pcf8583_i2c_start(rtc_pcf8583_t *context)
{
    BYTE tmp;
    time_t latch = (context->clock_halt) ? context->clock_halt_latch : rtc_get_latch(context->offset);
    int i;

    context->clock_regs_for_read[PCF8583_REG_CONTROL] = context->clock_regs[PCF8583_REG_CONTROL];
    context->clock_regs_for_read[PCF8583_REG_100TH_SECONDS] = rtc_get_centisecond(1);
    context->clock_regs_for_read[PCF8583_REG_SECONDS] = rtc_get_second(latch, 1);
    context->clock_regs_for_read[PCF8583_REG_MINUTES] = rtc_get_minute(latch, 1);
    tmp = context->am_pm << 7;
    if (context->am_pm) {
        tmp |= rtc_get_hour_am_pm(latch, 1);
        if (tmp & 0x20) {
            tmp &= 0xdf;
            tmp |= 0x40;
        }
    } else {
        tmp |= rtc_get_hour(latch, 1);
    }
    context->clock_regs_for_read[PCF8583_REG_HOURS] = tmp;
    tmp = rtc_get_year(latch, 1);
    tmp &= 3;
    tmp <<= 6;
    tmp |= rtc_get_day_of_month(latch, 1);
    context->clock_regs_for_read[PCF8583_REG_YEARS_MONTH_DAYS] = tmp;
    tmp = rtc_get_weekday(latch) + 1;
    tmp &= 7;
    tmp <<= 5;
    tmp |= rtc_get_month(latch, 1);
    context->clock_regs_for_read[PCF8583_REG_WEEK_DAYS_MONTHS] = tmp;
    context->clock_regs_for_read[PCF8583_REG_TIMER_DAYS] = 0;
    for (i = PCF8583_REG_ALARM_CONTROL; i <= PCF8583_REG_TIMER_ALARM; ++i) {
        context->clock_regs_for_read[i] = context->clock_regs[i];
    }
    if (context->read_bit_shift) {
        make_read_register_train(context);
    }
}

static BYTE pcf8583_read_register(rtc_pcf8583_t *context, BYTE addr)
{
    if (addr < PCF8583_REG_SIZE) {
        return context->clock_regs_for_read[addr];
    }
    return context->ram[addr - PCF8583_REG_SIZE];
}

static void pcf8583_write_register(rtc_pcf8583_t *context, BYTE addr, BYTE val)
{
    switch (addr) {
        case PCF8583_REG_MINUTES:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_minute(val, context->offset, 1);
            }
            break;
        case PCF8583_REG_CONTROL:
            if (context->clock_halt) {
                if (!(val & 0x80)) {
                    context->offset = context->offset - (rtc_get_latch(0) - (context->clock_halt_latch - context->offset));
                    context->clock_halt = 0;
                }
            } else {
                if (val & 0x80) {
                    context->clock_halt = 1;
                    context->clock_halt_latch = rtc_get_latch(context->offset);
                }
            }
            context->clock_regs[PCF8583_REG_CONTROL] = val;
            break;
        case PCF8583_REG_100TH_SECONDS:
        case PCF8583_REG_TIMER_DAYS:
            break;
        case PCF8583_REG_SECONDS:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_second(val, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_second(val, context->offset, 1);
            }
            break;
        case PCF8583_REG_HOURS:
            if (val & 0x80) {
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
        case PCF8583_REG_YEARS_MONTH_DAYS:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_year((val & 0xc0) >> 6, context->clock_halt_latch, 1);
                context->clock_halt_latch = rtc_set_latched_day_of_month(val & 0x3f, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_year((val & 0xc0) >> 6, context->offset, 1);
                context->offset = rtc_set_day_of_month(val & 0x3f, context->offset, 1);
            }
            break;
        case PCF8583_REG_WEEK_DAYS_MONTHS:
            if (context->clock_halt) {
                context->clock_halt_latch = rtc_set_latched_weekday(((val & 0xe0) >> 5) - 1, context->clock_halt_latch);
                context->clock_halt_latch = rtc_set_latched_month(val & 0x1f, context->clock_halt_latch, 1);
            } else {
                context->offset = rtc_set_weekday(((val & 0xe0) >> 5) - 1, context->offset);
                context->offset = rtc_set_month(val & 0x1f, context->offset, 1);
            }
            break;
        case PCF8583_REG_ALARM_CONTROL:
        case PCF8583_REG_100TH_SECONDS_ALARM:
        case PCF8583_REG_SECONDS_ALARM:
        case PCF8583_REG_MINUTES_ALARM:
        case PCF8583_REG_HOURS_ALARM:
        case PCF8583_REG_DATE_ALARM:
        case PCF8583_REG_MONTH_ALARM:
        case PCF8583_REG_TIMER_ALARM:
            context->clock_regs[addr] = val;
            break;
        default:
            context->ram[addr - PCF8583_REG_SIZE] = val;
    }
}

static void pcf8583_next_address_bit(rtc_pcf8583_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        if (context->reg == 0xa0) {
            context->state = PCF8583_ADDRESS_WRITE_ACK;
        } else if (context->reg == 0xa1) {
            if (context->read_bit_shift) {
                context->state = PCF8583_READ_REGS_TRAIN;
                context->bit = 0;
            } else {
                context->state = PCF8583_ADDRESS_READ_ACK;
            }
        } else {
            context->state = PCF8583_IDLE;
        }
    }
}

static void pcf8583_next_reg_nr_bit(rtc_pcf8583_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        context->state = PCF8583_REG_NR_ACK;
        context->reg_ptr = context->reg;
    }
}

static void pcf8583_next_read_bit(rtc_pcf8583_t *context)
{
    ++context->bit;
    if (context->bit == 8) {
        context->state = PCF8583_READ_ACK;
    }
}

static void pcf8583_next_write_bit(rtc_pcf8583_t *context)
{
    context->reg |= (context->data_line << (7 - context->bit));
    ++context->bit;
    if (context->bit == 8) {
        pcf8583_write_register(context, context->reg_ptr, context->reg);
        context->state = PCF8583_WRITE_ACK;
        ++context->reg_ptr;
    }
}

static void pcf8583_validate_read_ack(rtc_pcf8583_t *context)
{
    if (!context->data_line) {
        context->state = PCF8583_READ_REGS;
        context->bit = 0;
        ++context->reg_ptr;
        context->reg = pcf8583_read_register(context, context->reg_ptr);
    } else {
        context->state = PCF8583_IDLE;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void pcf8583_set_clk_line(rtc_pcf8583_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->sclk_line == val) {
        return;
    }

    if (val) {
        switch (context->state) {
            case PCF8583_READ_REGS_TRAIN:
                pcf8584_next_train_bit(context);
                break;
            case PCF8583_GET_ADDRESS:
                pcf8583_next_address_bit(context);
                break;
            case PCF8583_GET_REG_NR:
                pcf8583_next_reg_nr_bit(context);
                break;
            case PCF8583_READ_REGS:
                pcf8583_next_read_bit(context);
                break;
            case PCF8583_WRITE_REGS:
                pcf8583_next_write_bit(context);
                break;
            case PCF8583_READ_ACK:
                pcf8583_validate_read_ack(context);
                break;
            case PCF8583_ADDRESS_READ_ACK:
                context->state = PCF8583_READ_REGS;
                context->reg = pcf8583_read_register(context, context->reg_ptr);
                context->bit = 0;
                break;
            case PCF8583_REG_NR_ACK:
            case PCF8583_WRITE_ACK:
                context->state = PCF8583_WRITE_REGS;
                context->reg = 0;
                context->bit = 0;
                break;
            case PCF8583_ADDRESS_WRITE_ACK:
                context->state = PCF8583_GET_REG_NR;
                context->reg = 0;
                context->bit = 0;
                break;
        }
    }
    context->sclk_line = val;
}

void pcf8583_set_data_line(rtc_pcf8583_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->data_line == val) {
        return;
    }

    if (context->sclk_line) {
        if (val) {
            context->state = PCF8583_IDLE;
        } else {
            pcf8583_i2c_start(context);
            context->state = PCF8583_GET_ADDRESS;
            context->reg = 0;
            context->bit = 0;
        }
    }
    context->data_line = val;
}

BYTE pcf8583_read_data_line(rtc_pcf8583_t *context)
{
	switch (context->state) {
        case PCF8583_READ_REGS:
            return (context->reg & (1 << (7 - context->bit))) >> (7 - context->bit);
        case PCF8583_READ_REGS_TRAIN:
            return register_train[(context->reg_ptr * 9) + context->bit];
        case PCF8583_ADDRESS_READ_ACK:
        case PCF8583_ADDRESS_WRITE_ACK:
        case PCF8583_REG_NR_ACK:
        case PCF8583_READ_ACK:
        case PCF8583_WRITE_ACK:
            return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------------------------------------- */

/* RTC_PCF8583 snapshot module format:

   type   | name                | description
   ------------------------------------------
   BYTE   | clock halt          | clock halt flag
   DWORD  | clock halt latch hi | high DWORD of clock halt offset
   DWORD  | clock halt latch lo | low DWORD of clock halt offset
   BYTE   | am pm               | AM/PM flag
   DWORD  | read bit shift      | special case bit pattern shift
   DWORD  | latch hi            | high DWORD of latch offset
   DWORD  | latch lo            | low DWORD of latch offset
   DWORD  | offset hi           | high DWORD of RTC offset
   DWORD  | offset lo           | low DWORD of RTC offset
   DWORD  | old offset hi       | high DWORD of old RTC offset
   DWORD  | old offset lo       | low DWORD of old RTC offset
   ARRAY  | clock regs          | 16 BYTES of register data
   ARRAY  | old clock regs      | 16 BYTES of old register data
   ARRAY  | clock regs for read | 16 BYTES of clock read register data
   ARRAY  | RAM                 | 240 BYTES of RAM data
   ARRAY  | old RAM             | 240 BYTES of old RAM data
   BYTE   | state               | current state
   BYTE   | reg                 | current register
   BYTE   | reg ptr             | current register pointer
   BYTE   | bit                 | current bit
   BYTE   | io byte             | current I/O BYTE
   BYTE   | sclk                | SCLK line state
   BYTE   | data                | DATA line state
   BYTE   | clock register      | current clock register
   STRING | device              | device name STRING
 */

static char snap_module_name[] = "RTC_PCF8583";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int pcf8583_write_snapshot(rtc_pcf8583_t *context, snapshot_t *s)
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
        || SMW_DW(m, (DWORD)context->read_bit_shift) < 0
        || SMW_DW(m, latch_hi) < 0
        || SMW_DW(m, latch_lo) < 0
        || SMW_DW(m, offset_hi) < 0
        || SMW_DW(m, offset_lo) < 0
        || SMW_DW(m, old_offset_hi) < 0
        || SMW_DW(m, old_offset_lo) < 0
        || SMW_BA(m, context->clock_regs, PCF8583_REG_SIZE) < 0
        || SMW_BA(m, context->old_clock_regs, PCF8583_REG_SIZE) < 0
        || SMW_BA(m, context->clock_regs_for_read, PCF8583_REG_SIZE) < 0
        || SMW_BA(m, context->ram, PCF8583_RAM_SIZE) < 0
        || SMW_BA(m, context->old_ram, PCF8583_RAM_SIZE) < 0
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

int pcf8583_read_snapshot(rtc_pcf8583_t *context, snapshot_t *s)
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
        || SMR_DW_INT(m, &context->read_bit_shift) < 0
        || SMR_DW(m, &latch_hi) < 0
        || SMR_DW(m, &latch_lo) < 0
        || SMR_DW(m, &offset_hi) < 0
        || SMR_DW(m, &offset_lo) < 0
        || SMR_DW(m, &old_offset_hi) < 0
        || SMR_DW(m, &old_offset_lo) < 0
        || SMR_BA(m, context->clock_regs, PCF8583_REG_SIZE) < 0
        || SMR_BA(m, context->old_clock_regs, PCF8583_REG_SIZE) < 0
        || SMR_BA(m, context->clock_regs_for_read, PCF8583_REG_SIZE) < 0
        || SMR_BA(m, context->ram, PCF8583_RAM_SIZE) < 0
        || SMR_BA(m, context->old_ram, PCF8583_RAM_SIZE) < 0
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
