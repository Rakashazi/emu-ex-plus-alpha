/*
 * ds1602.c - DS1602 RTC emulation.
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

#include "ds1602.h"
#include "lib.h"
#include "rtc.h"
#include "snapshot.h"

#include <time.h>
#include <string.h>

/* The DS1602 is a serial based RTC, it has the following features:
 * - Real-Time Clock Counts Seconds at a 32bit resolution.
 * - Real-Time Clock Counts device attached to power seconds at a 32bit resolution.
 */

/* This module is currently used in the following emulated hardware:
 - joyport bbrtc device
 */

/* ---------------------------------------------------------------------------------------------------- */

#define DS1602_RAM_SIZE   0
#define DS1602_REG_SIZE   0

struct rtc_ds1602_s {
    time_t latch;
    time_t offset0;
    time_t offset;
    time_t old_offset;
    DWORD reg;
    BYTE state;
    BYTE bit;
    BYTE io_byte;
    BYTE rst_line;
    BYTE clk_line;
    BYTE data_line_in;
    BYTE data_line_out;
    char *device;
};

#define DS1602_IDLE               0
#define DS1602_GET_PROTOCOL       1
#define DS1602_GET_SECONDS_WAIT   2
#define DS1602_GET_SECONDS        3
#define DS1602_SET_CONT_SECONDS   4
#define DS1602_SET_ACT_SECONDS    5

/* ---------------------------------------------------------------------------------------------------- */

rtc_ds1602_t *ds1602_init(char *device, time_t offset0)
{
    rtc_ds1602_t *retval = lib_calloc(1, sizeof(rtc_ds1602_t));
    int loaded = rtc_load_context(device, DS1602_RAM_SIZE, DS1602_REG_SIZE);

    if (loaded) {
        retval->offset = rtc_get_loaded_offset();
    } else {
        retval->offset = 0;
    }
    retval->old_offset = retval->offset;
    retval->offset0 = offset0;

    retval->device = lib_stralloc(device);
    retval->state = DS1602_IDLE;
    retval->rst_line = 1;
    retval->clk_line = 1;
    retval->data_line_out = 1;
    retval->data_line_in = 1;
    retval->bit = 0;

    return retval;
}

void ds1602_destroy(rtc_ds1602_t *context, int save)
{
    if (save) {
        if (context->offset != context->old_offset) {
            rtc_save_context(NULL, DS1602_RAM_SIZE, NULL, DS1602_REG_SIZE, context->device, context->offset);
        }
    }
    lib_free(context->device);
    lib_free(context);
}

/* ---------------------------------------------------------------------------------------------------- */

static void ds1602_read_bit(rtc_ds1602_t *context)
{
    if (context->state == DS1602_GET_SECONDS) {
        context->data_line_out = (context->reg & 1 << context->bit) ? 1 : 0;
        ++context->bit;
        if (context->bit == 32) {
            context->state = DS1602_IDLE;
        }
    }
}

static void ds1602_write_protocol_bit(rtc_ds1602_t *context)
{
    DWORD val = context->data_line_in << context->bit;

    context->reg |= val;
    ++context->bit;
    if (context->bit == 8) {
        if ((context->reg & 0xc1) == 0x81) {
            context->state = DS1602_GET_SECONDS;
            context->reg = (DWORD)(rtc_get_latch(context->offset) - context->offset0);
            context->bit = 0;
        } else if ((context->reg & 0xc1) == 0x80) {
            context->state = DS1602_SET_CONT_SECONDS;
            context->reg = 0;
            context->bit = 0;
        } else if ((context->reg & 0xc1) == 0x41) {
            /* FIXME: make active get seconds */
        } else if ((context->reg & 0xc1) == 0x40) {
            context->state = DS1602_SET_ACT_SECONDS;
            context->reg = 0;
            context->bit = 0;
        } else if ((context->reg & 0xc4) == 0x04) {
            context->offset = time(NULL);
            context->state = DS1602_IDLE;
        } else if ((context->reg & 0xc2) == 0x02) {
            /* FIXME: do clear active timer */
            context->state = DS1602_IDLE;
        } else if ((context->reg & 0xc1) == 0xc0) {
            /* FIXME: do set trim bits */
            context->state = DS1602_IDLE;
        } else {
            context->state = DS1602_IDLE;
        }
    }
}

static void ds1602_write_seconds_bit(rtc_ds1602_t *context)
{
    DWORD val = context->data_line_in << context->bit;
    time_t now;

    context->reg |= val;
    ++context->bit;
    if (context->bit == 32) {
        now = time(NULL) + context->offset;
        context->offset = context->offset + ((context->reg + context->offset0) - now);
        context->state = DS1602_IDLE;
    }
}

static void ds1602_write_bit(rtc_ds1602_t *context)
{
    switch (context->state) {
        case DS1602_IDLE:
        case DS1602_GET_SECONDS:
        default:
            break;
        case DS1602_GET_PROTOCOL:
            ds1602_write_protocol_bit(context);
            break;
        case DS1602_SET_CONT_SECONDS:
        case DS1602_SET_ACT_SECONDS:
            ds1602_write_seconds_bit(context);
            break;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void ds1602_set_reset_line(rtc_ds1602_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->rst_line == val) {
        return;
    }

    if (val) {
        context->state = DS1602_GET_PROTOCOL;
        context->bit = 0;
        context->reg = 0;
    } else {
        context->state = DS1602_IDLE;
        context->data_line_out = 1;
    }

    context->rst_line = val;
}

void ds1602_set_clk_line(rtc_ds1602_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->clk_line == val) {
        return;
    }

    if (val) {
        ds1602_write_bit(context);
    } else {
        ds1602_read_bit(context);
    }
    context->clk_line = val;
}

void ds1602_set_data_line(rtc_ds1602_t *context, BYTE data)
{
    BYTE val = data ? 1 : 0;

    if (context->data_line_in == val) {
        return;
    }

    context->data_line_in = val;
}

BYTE ds1602_read_data_line(rtc_ds1602_t *context)
{
    return context->data_line_out;
}

/* ---------------------------------------------------------------------------------------------------- */

/* RTC_DS12C887 snapshot module format:

   type   | name          | description
   ----------------------------------
   DWORD  | latch hi      | high DWORD of latch offset
   DWORD  | latch lo      | low DWORD of latch offset
   DWORD  | offset 0 hi   | high DWORD of RTC offset 0
   DWORD  | offset 0 lo   | low DWORD of RTC offset 0
   DWORD  | offset hi     | high DWORD of RTC offset
   DWORD  | offset lo     | low DWORD of RTC offset
   DWORD  | old offset hi | high DWORD of old RTC offset
   DWORD  | old offset lo | low DWORD of old RTC offset
   DWORD  | reg           | current register
   BYTE   | state         | current state
   BYTE   | bit           | current bit
   BYTE   | io byte       | current I/O BYTE
   BYTE   | rst           | RST line state
   BYTE   | clk           | CLK line state
   BYTE   | data line in  | DATA in line state
   BYTE   | data line out | DATA out line state
   STRING | device        | device name STRING
 */

static char snap_module_name[] = "RTC_DS1602";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ds1602_write_snapshot(rtc_ds1602_t *context, snapshot_t *s)
{
    DWORD latch_lo = 0;
    DWORD latch_hi = 0;
    DWORD offset0_lo = 0;
    DWORD offset0_hi = 0;
    DWORD offset_lo = 0;
    DWORD offset_hi = 0;
    DWORD old_offset_lo = 0;
    DWORD old_offset_hi = 0;
    snapshot_module_t *m;

    /* time_t can be either 32bit or 64bit, so we save as 64bit */
#if (SIZE_OF_TIME_T == 8)
    latch_hi = (DWORD)(context->latch >> 32);
    latch_lo = (DWORD)(context->latch & 0xffffffff);
    offset0_hi = (DWORD)(context->offset0 >> 32);
    offset0_lo = (DWORD)(context->offset0 & 0xffffffff);
    offset_hi = (DWORD)(context->offset >> 32);
    offset_lo = (DWORD)(context->offset & 0xffffffff);
    old_offset_hi = (DWORD)(context->old_offset >> 32);
    old_offset_lo = (DWORD)(context->old_offset & 0xffffffff);
#else
    latch_lo = (DWORD)context->latch;
    offset0_lo = (DWORD)context->offset0;
    offset_lo = (DWORD)context->offset;
    old_offset_lo = (DWORD)context->old_offset;
#endif

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_DW(m, latch_hi) < 0
        || SMW_DW(m, latch_lo) < 0
        || SMW_DW(m, offset0_hi) < 0
        || SMW_DW(m, offset0_lo) < 0
        || SMW_DW(m, offset_hi) < 0
        || SMW_DW(m, offset_lo) < 0
        || SMW_DW(m, old_offset_hi) < 0
        || SMW_DW(m, old_offset_lo) < 0
        || SMW_DW(m, context->reg) < 0
        || SMW_B(m, context->state) < 0
        || SMW_B(m, context->bit) < 0
        || SMW_B(m, context->io_byte) < 0
        || SMW_B(m, context->rst_line) < 0
        || SMW_B(m, context->clk_line) < 0
        || SMW_B(m, context->data_line_in) < 0
        || SMW_B(m, context->data_line_out) < 0
        || SMW_STR(m, context->device) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    return snapshot_module_close(m);
}

int ds1602_read_snapshot(rtc_ds1602_t *context, snapshot_t *s)
{
    DWORD latch_lo = 0;
    DWORD latch_hi = 0;
    DWORD offset0_lo = 0;
    DWORD offset0_hi = 0;
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
        || SMR_DW(m, &latch_hi) < 0
        || SMR_DW(m, &latch_lo) < 0
        || SMR_DW(m, &offset0_hi) < 0
        || SMR_DW(m, &offset0_lo) < 0
        || SMR_DW(m, &offset_hi) < 0
        || SMR_DW(m, &offset_lo) < 0
        || SMR_DW(m, &old_offset_hi) < 0
        || SMR_DW(m, &old_offset_lo) < 0
        || SMR_DW(m, &context->reg) < 0
        || SMR_B(m, &context->state) < 0
        || SMR_B(m, &context->bit) < 0
        || SMR_B(m, &context->io_byte) < 0
        || SMR_B(m, &context->rst_line) < 0
        || SMR_B(m, &context->clk_line) < 0
        || SMR_B(m, &context->data_line_in) < 0
        || SMR_B(m, &context->data_line_out) < 0
        || SMR_STR(m, &context->device) < 0) {
        goto fail;
    }
    snapshot_module_close(m);

#if (SIZE_OF_TIME_T == 8)
    context->latch = (time_t)(latch_hi) << 32;
    context->latch |= latch_lo;
    context->offset0 = (time_t)(offset0_hi) << 32;
    context->offset0 |= offset0_lo;
    context->offset = (time_t)(offset_hi) << 32;
    context->offset |= offset_lo;
    context->old_offset = (time_t)(old_offset_hi) << 32;
    context->old_offset |= old_offset_lo;
#else
    context->latch = latch_lo;
    context->offset = offset_lo;
    context->offset0 = offset0_lo;
    context->old_offset = old_offset_lo;
#endif

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
