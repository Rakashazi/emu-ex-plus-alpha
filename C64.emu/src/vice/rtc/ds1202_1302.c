/*
 * ds1202_1302.c - DS1202/1302 RTC emulation.
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

#include "ds1202_1302.h"
#include "lib.h"
#include "rtc.h"
#include "snapshot.h"

#include <time.h>
#include <string.h>

/* The DS1202 and DS1302 are serial line based RTCs, they have the following features:
 * - Real-Time Clock Counts Seconds, Minutes, Hours, Date of the Month,
 *   Month, Day of the Week, and Year with Leap-Year,
 *   Compensation Valid Up to 2100
 * - 31 x 8 Battery-Backed General-Purpose RAM
 * - Single-Byte or Multiple-Byte (Burst Mode) Data Transfer for Read
 *   or Write of Clock or RAM Data
 * - 24/12h mode with AM/PM indicator in 12h mode
 * - Clock Halt flag
 * - Write Protect flag
 * - All clock registers are in BCD format
 */

/* The DS1202 and DS1302 have the following clock registers:
 *
 * register 0 : bit  7   Clock Halt
 *              bits 6-4 10 seconds
 *              bits 3-0 seconds
 *
 * register 1 : bit  7   ??
 *              bits 6-4 10 minutes
 *              bits 3-0 minutes
 *
 * register 2 : bit  7   12/24 hour mode flag, 0 = 24 hour mode, 1 = 12 hour mode
 *              bit  6   0
 *              bit  5   in 12 hour mode this is the AM/PM flag, 0 = AM, 1 = PM
 *                       in 24 hour mode this is the msb of the 10 hours
 *              bit  4   lsb of 10 hours
 *              bits 3-0 hours
 *
 * register 3 : bits 7-6 0
 *              bits 5-4 10 days (of month)
 *              bits 3-0 days (of month)
 *
 * register 4 : bits 7-5 0
 *              bit  4   10 months
 *              bits 3-0 months
 *
 * register 5 : bits 7-3 0
 *              bits 2-0 days (of week)
 *
 * register 6 : bits 7-4 10 years
 *              bits 3-0 years
 *
 * register 7 : bit 7    Write Protect flag
 *              bits 6-0 0
 *
 * DS1202 : registers 8-30: There are no registers at these locations (emulated as returning
 *                          0 upon read)
 *
 * DS1302 : register 8 : bits 7-0 Trickle Charge register (emulated as a dummy RAM byte)
 * DS1202 : registers 9-30: There are no registers at these locations (emulated as returning
 *                          0 upon read)
 *
 * register 31 : Clock Burst mode, all 9 registers will be read from or written to in sequence.
 *               When writing in burst mode the first 8 registers need to be transfered before
 *               the registers are accepted
 */

/* RAM location 31 is used for RAM burst mode */

/* This module is currently used in the following emulated hardware:
   - C64/C128 IDE64 cartridge (1302)
   - CMD Smartmouse (1202)
 */

/* ---------------------------------------------------------------------------------------------------- */

#define DS1202_1302_RAM_SIZE   32
#define DS1202_1302_REG_SIZE   8

struct rtc_ds1202_1302_s {
    int rtc_type;
    int clock_halt;
    time_t clock_halt_latch;
    int am_pm;
    int write_protect;
    time_t latch;
    time_t offset;
    time_t old_offset;
    BYTE *clock_regs;
    BYTE old_clock_regs[DS1202_1302_REG_SIZE];
    BYTE trickle_charge;
    BYTE *ram;
    BYTE old_ram[DS1202_1302_RAM_SIZE];
    BYTE state;
    BYTE reg;
    BYTE bit;
    BYTE output_bit;
    BYTE io_byte;
    BYTE sclk_line;
    BYTE clock_register;
    char *device;
};

#define DS1202_1302_REG_SECONDS_CH       0
#define DS1202_1302_REG_MINUTES          1
#define DS1202_1302_REG_HOURS            2
#define DS1202_1302_REG_DAYS_OF_MONTH    3
#define DS1202_1302_REG_MONTHS           4
#define DS1202_1302_REG_DAYS_OF_WEEK     5
#define DS1202_1302_REG_YEARS            6
#define DS1202_1302_REG_WRITE_PROTECT    7
#define DS1302_REG_TRICKLE_CHARGE        8

#define DS1202_1302_BURST   31

#define DS1202_1302_INPUT_COMMAND_BITS        0
#define DS1202_1302_INPUT_SINGLE_DATA_BITS    1
#define DS1202_1302_INPUT_BURST_DATA_BITS     2
#define DS1202_1302_OUTPUT_SINGLE_DATA_BITS   3
#define DS1202_1302_OUTPUT_BURST_DATA_BITS    4

/* ---------------------------------------------------------------------------------------------------- */

void ds1202_1302_reset(rtc_ds1202_1302_t *context)
{
    context->state = DS1202_1302_INPUT_COMMAND_BITS;
    context->bit = 0;
    context->io_byte = 0;
}

rtc_ds1202_1302_t *ds1202_1302_init(char *device, int rtc_type)
{
    rtc_ds1202_1302_t *retval = lib_calloc(1, sizeof(rtc_ds1202_1302_t));
    int loaded = rtc_load_context(device, DS1202_1302_RAM_SIZE, DS1202_1302_REG_SIZE);

    if (loaded) {
        retval->ram = rtc_get_loaded_ram();
        retval->offset = rtc_get_loaded_offset();
        retval->clock_regs = rtc_get_loaded_clockregs();
    } else {
        retval->ram = lib_calloc(1, DS1202_1302_RAM_SIZE);
        retval->offset = 0;
        retval->clock_regs = lib_calloc(1, DS1202_1302_REG_SIZE);
    }
    memcpy(retval->old_ram, retval->ram, DS1202_1302_RAM_SIZE);
    retval->old_offset = retval->offset;
    memcpy(retval->old_clock_regs, retval->clock_regs, DS1202_1302_REG_SIZE);

    retval->rtc_type = rtc_type;
    retval->device = lib_stralloc(device);

    return retval;
}

void ds1202_1302_destroy(rtc_ds1202_1302_t *context, int save)
{
    if (save) {
        if (memcmp(context->ram, context->old_ram, DS1202_1302_RAM_SIZE) ||
            memcmp(context->clock_regs, context->old_clock_regs, DS1202_1302_REG_SIZE) ||
            context->offset != context->old_offset) {
            rtc_save_context(context->ram, DS1202_1302_RAM_SIZE, context->clock_regs, DS1202_1302_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->ram);
    lib_free(context->clock_regs);
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static BYTE ds1202_1302_get_clock_register(rtc_ds1202_1302_t *context, int reg, time_t offset, int latched)
{
    BYTE retval;
    time_t latch = (latched) ? offset : rtc_get_latch(offset);

    switch (reg) {
        case DS1202_1302_REG_SECONDS_CH:
            retval = context->clock_halt << 7;
            retval |= rtc_get_second(latch, 1);
            break;
        case DS1202_1302_REG_MINUTES:
            retval = rtc_get_minute(latch, 1);
            break;
        case DS1202_1302_REG_HOURS:
            retval = context->am_pm << 7;
            if (context->am_pm) {
                retval |= rtc_get_hour_am_pm(latch, 1);
            } else {
                retval |= rtc_get_hour(latch, 1);
            }
            break;
        case DS1202_1302_REG_DAYS_OF_MONTH:
            retval = rtc_get_day_of_month(latch, 1);
            break;
        case DS1202_1302_REG_MONTHS:
            retval = rtc_get_month(latch, 1);
            break;
        case DS1202_1302_REG_DAYS_OF_WEEK:
            retval = rtc_get_weekday(latch) + 1;
            break;
        case DS1202_1302_REG_YEARS:
            retval = rtc_get_year(latch, 1);
            break;
        case DS1202_1302_REG_WRITE_PROTECT:
            retval = context->write_protect << 7;
            break;
        case DS1302_REG_TRICKLE_CHARGE:
            if (context->rtc_type == 1302) {
                retval = context->trickle_charge;
            } else {
                retval = 0;
            }
            break;
        default:
            retval = 0;
    }
    return retval;
}

static void ds1202_1302_decode_command(rtc_ds1202_1302_t *context)
{
    int burst = 0;
    int read = 0;
    int clock_reg = 0;
    int latched = 0;
    time_t offset;
    BYTE command = context->io_byte;

    /* is bit 7 set ? */
    if (command < 0x80) {
        ds1202_1302_reset(context);
        return;
    }

    /* check the type of register that is being addressed */
    if (!(command & 0x40)) {
        context->clock_register = 1;
        clock_reg = 1;
    } else {
        context->clock_register = 0;
    }

    /* get the register nr */
    context->reg = (command & 0x3e) >> 1;

    /* check burst */
    if (context->reg == 31) {
        burst = 1;
    }

    /* check read */
    if (command & 1) {
        read = 1;
    }

    /* check for DS1202_1302_INPUT_SINGLE_DATA_BITS */
    if (!read && !burst) {
        context->state = DS1202_1302_INPUT_SINGLE_DATA_BITS;
        context->io_byte = 0;
        context->bit = 0;
    }

    /* check for DS1202_1302_INPUT_BURST_DATA_BITS */
    if (!read && burst) {
        context->state = DS1202_1302_INPUT_BURST_DATA_BITS;
        context->io_byte = 0;
        context->bit = 0;
        context->reg = 0;
    }

    /* check for DS1202_1302_OUTPUT_SINGLE_DATA_BITS and clock */
    if (read && !burst && clock_reg) {
        context->state = DS1202_1302_OUTPUT_SINGLE_DATA_BITS;
        context->bit = 0;
        latched = context->clock_halt;
        offset = (latched) ? context->clock_halt_latch : context->offset;
        context->io_byte = ds1202_1302_get_clock_register(context, context->reg, offset, latched);
    }

    /* check for DS1202_1302_OUTPUT_SINGLE_DATA_BITS and RAM */
    if (read && !burst && !clock_reg) {
        context->state = DS1202_1302_OUTPUT_SINGLE_DATA_BITS;
        context->bit = 0;
        context->io_byte = context->ram[context->reg];
    }

    /* check for DS1202_1302_OUTPUT_BURST_DATA_BITS and clock */
    if (read && burst && clock_reg) {
        context->state = DS1202_1302_OUTPUT_BURST_DATA_BITS;
        context->reg = 0;
        context->bit = 0;
        if (context->clock_halt) {
            context->latch = context->clock_halt_latch;
        } else {
            context->latch = rtc_get_latch(context->offset);
        }
        context->io_byte = ds1202_1302_get_clock_register(context, 0, context->latch, 1);
    }

    /* check for DS1202_1302_OUTPUT_BURST_DATA_BITS and RAM */
    if (read && burst && !clock_reg) {
        context->state = DS1202_1302_OUTPUT_BURST_DATA_BITS;
        context->reg = 0;
        context->bit = 0;
        context->io_byte = context->ram[0];
    }
}

static void ds1202_1302_write_command_bit(rtc_ds1202_1302_t *context, unsigned int input_bit)
{
    context->io_byte |= (input_bit << context->bit);
    context->bit++;
    if (context->bit == 8) {
        ds1202_1302_decode_command(context);
    }
}

static BYTE ds1202_1302_read_burst_data_bit(rtc_ds1202_1302_t *context)
{
    BYTE retval;

    retval = (context->io_byte >> context->bit) & 1;
    context->bit++;
    if (context->bit == 8) {
        context->reg++;
        if (context->clock_register) {
            if (context->reg == 8) {
                context->state = DS1202_1302_INPUT_COMMAND_BITS;
                context->bit = 0;
                context->io_byte = 0;
            } else {
                context->bit = 0;
                context->io_byte = ds1202_1302_get_clock_register(context, context->reg, context->latch, 1);
            }
        } else {
            if (context->reg == 32) {
                context->state = DS1202_1302_INPUT_COMMAND_BITS;
                context->bit = 0;
                context->io_byte = 0;
            } else {
                context->bit = 0;
                context->io_byte = context->ram[context->reg];
            }
        }
    }
    return retval;
}

static BYTE ds1202_1302_read_single_data_bit(rtc_ds1202_1302_t *context)
{
    BYTE retval;

    retval = ((context->io_byte) >> context->bit) & 1;
    context->bit++;
    if (context->bit == 8) {
        context->state = DS1202_1302_INPUT_COMMAND_BITS;
        context->bit = 0;
        context->io_byte = 0;
    }
    return retval;
}

static void ds1202_1302_write_burst_data_bit(rtc_ds1202_1302_t *context, unsigned int input_bit)
{
    BYTE val;

    context->io_byte |= (input_bit << context->bit);
    context->bit++;
    if (context->bit == 8) {
        if (context->clock_register) {
            context->clock_regs[context->reg] = context->io_byte;
            context->reg++;
            if (context->reg == 8) {
                context->state = DS1202_1302_INPUT_COMMAND_BITS;
                if (!context->write_protect) {
                    if (context->clock_halt) {
                        val = context->clock_regs[DS1202_1302_REG_YEARS];
                        context->clock_halt_latch = rtc_set_latched_year(val, context->clock_halt_latch, 1);
                        val = context->clock_regs[DS1202_1302_REG_MONTHS];
                        context->clock_halt_latch = rtc_set_latched_month(val, context->clock_halt_latch, 1);
                        val = context->clock_regs[DS1202_1302_REG_DAYS_OF_MONTH];
                        context->clock_halt_latch = rtc_set_latched_day_of_month(val, context->clock_halt_latch, 1);
                        val = context->clock_regs[DS1202_1302_REG_DAYS_OF_WEEK];
                        context->clock_halt_latch = rtc_set_latched_weekday(val - 1, context->clock_halt_latch);
                        val = context->clock_regs[DS1202_1302_REG_HOURS];
                        if (val & 0x80) {
                            context->clock_halt_latch = rtc_set_latched_hour_am_pm(val & 0x7f, context->clock_halt_latch, 1);
                        } else {
                            context->clock_halt_latch = rtc_set_latched_hour(val & 0x7f, context->clock_halt_latch, 1);
                        }
                        val = context->clock_regs[DS1202_1302_REG_MINUTES];
                        context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, 1);
                        val = context->clock_regs[DS1202_1302_REG_SECONDS_CH];
                        context->clock_halt_latch = rtc_set_latched_second(val & 0x7f, context->clock_halt_latch, 1);
                        if (!(val & 0x80)) {
                            context->offset = context->offset - (rtc_get_latch(0) - (context->clock_halt_latch - context->offset));
                            context->clock_halt = 0;
                        }
                    } else {
                        val = context->clock_regs[DS1202_1302_REG_YEARS];
                        context->offset = rtc_set_year(val, context->offset, 1);
                        val = context->clock_regs[DS1202_1302_REG_MONTHS];
                        context->offset = rtc_set_month(val, context->offset, 1);
                        val = context->clock_regs[DS1202_1302_REG_DAYS_OF_MONTH];
                        context->offset = rtc_set_day_of_month(val, context->offset, 1);
                        val = context->clock_regs[DS1202_1302_REG_DAYS_OF_WEEK];
                        context->offset = rtc_set_weekday(val - 1, context->offset);
                        val = context->clock_regs[DS1202_1302_REG_HOURS];
                        if (val & 0x80) {
                            context->offset = rtc_set_hour_am_pm(val & 0x7f, context->offset, 1);
                        } else {
                            context->offset = rtc_set_hour(val & 0x7f, context->offset, 1);
                        }
                        val = context->clock_regs[DS1202_1302_REG_MINUTES];
                        context->offset = rtc_set_minute(val, context->offset, 1);
                        val = context->clock_regs[DS1202_1302_REG_SECONDS_CH];
                        context->offset = rtc_set_second(val & 0x7f, context->offset, 1);
                        if (val & 0x80) {
                            context->clock_halt = 1;
                            context->clock_halt_latch = rtc_get_latch(context->offset);
                        }
                    }
                }
            }
        } else {
            context->ram[context->reg] = context->io_byte;
            context->reg++;
            if (context->reg == 32) {
                context->state = DS1202_1302_INPUT_COMMAND_BITS;
            }
        }
        context->io_byte = 0;
        context->bit = 0;
    }
}

static void ds1202_1302_write_single_data_bit(rtc_ds1202_1302_t *context, unsigned int input_bit)
{
    BYTE val;

    context->io_byte |= (input_bit << context->bit);
    context->bit++;
    if (context->bit == 8) {
        if (context->clock_register) {
            val = context->io_byte;
            switch (context->reg) {
                case DS1202_1302_REG_MINUTES:
                    if (!context->write_protect) {
                        if (context->clock_halt) {
                            context->clock_halt_latch = rtc_set_latched_minute(val, context->clock_halt_latch, 1);
                        } else {
                            context->offset = rtc_set_minute(val, context->offset, 1);
                        }
                    }
                    break;
                case DS1202_1302_REG_DAYS_OF_MONTH:
                    if (!context->write_protect) {
                        if (context->clock_halt) {
                            context->clock_halt_latch = rtc_set_latched_day_of_month(val, context->clock_halt_latch, 1);
                        } else {
                            context->offset = rtc_set_day_of_month(val, context->offset, 1);
                        }
                    }
                    break;
                case DS1202_1302_REG_MONTHS:
                    if (!context->write_protect) {
                        if (context->clock_halt) {
                            context->clock_halt_latch = rtc_set_latched_month(val, context->clock_halt_latch, 1);
                        } else {
                            context->offset = rtc_set_month(val, context->offset, 1);
                        }
                    }
                    break;
                case DS1202_1302_REG_DAYS_OF_WEEK:
                    if (!context->write_protect) {
                        if (context->clock_halt) {
                            context->clock_halt_latch = rtc_set_latched_weekday(val - 1, context->clock_halt_latch);
                        } else {
                            context->offset = rtc_set_weekday(val - 1, context->offset);
                        }
                    }
                    break;
                case DS1202_1302_REG_YEARS:
                    if (!context->write_protect) {
                        if (context->clock_halt) {
                            context->clock_halt_latch = rtc_set_latched_year(val, context->clock_halt_latch, 1);
                        } else {
                            context->offset = rtc_set_year(val, context->offset, 1);
                        }
                    }
                    break;
                case DS1302_REG_TRICKLE_CHARGE:
                    if (context->rtc_type == 1302) {
                        context->trickle_charge = val;
                    }
                    break;
                case DS1202_1302_REG_WRITE_PROTECT:
                    context->write_protect = val >> 7;
                    break;
                case DS1202_1302_REG_HOURS:
                    if (!context->write_protect) {
                        if (val & 0x80) {
                            if (context->clock_halt) {
                                context->clock_halt_latch = rtc_set_latched_hour_am_pm(val & 0x7f, context->clock_halt_latch, 1);
                            } else {
                                context->offset = rtc_set_hour_am_pm(val & 0x7f, context->offset, 1);
                            }
                            context->am_pm = 1;
                        } else {
                            if (context->clock_halt) {
                                context->clock_halt_latch = rtc_set_latched_hour(val & 0x7f, context->clock_halt_latch, 1);
                            } else {
                                context->offset = rtc_set_hour(val & 0x7f, context->offset, 1);
                            }
                            context->am_pm = 0;
                        }
                    }
                    break;
                case DS1202_1302_REG_SECONDS_CH:
                    if (!context->write_protect) {
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
                    }
                    break;
            }
        } else {
            context->ram[context->reg] = context->io_byte;
        }
        context->state = DS1202_1302_INPUT_COMMAND_BITS;
        context->bit = 0;
        context->io_byte = 0;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void ds1202_1302_set_lines(rtc_ds1202_1302_t *context, unsigned int ce_line, unsigned int sclk_line, unsigned int input_bit)
{
    int rising_edge = 0;

    /* is the Chip Enable line low ? */
    if (!ce_line) {
        ds1202_1302_reset(context);
        context->sclk_line = sclk_line;
        return;
    }

    /* has the sclk_line changed ? */
    if (context->sclk_line == sclk_line) {
        return;
    }

    /* is the sclk_line on the rising edge ? */
    if (!context->sclk_line) {
        rising_edge = 1;
    }

    context->sclk_line = sclk_line;

    if (rising_edge) {
        /* handle writing state of rtc */
        switch (context->state) {
            case DS1202_1302_OUTPUT_SINGLE_DATA_BITS:
            case DS1202_1302_OUTPUT_BURST_DATA_BITS:
                break;
            case DS1202_1302_INPUT_COMMAND_BITS:
                ds1202_1302_write_command_bit(context, input_bit & 1u);
                break;
            case DS1202_1302_INPUT_SINGLE_DATA_BITS:
                ds1202_1302_write_single_data_bit(context, input_bit & 1u);
                break;
            case DS1202_1302_INPUT_BURST_DATA_BITS:
                ds1202_1302_write_burst_data_bit(context, input_bit & 1u);
                break;
        }
    } else {
        /* handle reading state of rtc */
        switch (context->state) {
            case DS1202_1302_INPUT_COMMAND_BITS:
            case DS1202_1302_INPUT_SINGLE_DATA_BITS:
            case DS1202_1302_INPUT_BURST_DATA_BITS:
                context->output_bit = input_bit & 1u;
                break;
            case DS1202_1302_OUTPUT_SINGLE_DATA_BITS:
                context->output_bit = ds1202_1302_read_single_data_bit(context);
                break;
            case DS1202_1302_OUTPUT_BURST_DATA_BITS:
                context->output_bit = ds1202_1302_read_burst_data_bit(context);
                break;
        }
    }
}

BYTE ds1202_1302_read_data_line(rtc_ds1202_1302_t *context)
{
    switch (context->state) {
        case DS1202_1302_INPUT_COMMAND_BITS:
        case DS1202_1302_INPUT_SINGLE_DATA_BITS:
        case DS1202_1302_INPUT_BURST_DATA_BITS:
            return 1;
        case DS1202_1302_OUTPUT_SINGLE_DATA_BITS:
        case DS1202_1302_OUTPUT_BURST_DATA_BITS:
            return context->output_bit;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "DS1202_1302"

/* FIXME: implement snapshot support */
int ds1202_1302_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int ds1202_1302_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}
