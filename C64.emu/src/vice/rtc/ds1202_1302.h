/*
 * ds1202_1302.h - DS1202/1302 RTC emulation.
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

#ifndef VICE_DS1202_1302_H
#define VICE_DS1202_1302_H

#include <time.h>

#include "types.h"

typedef struct rtc_ds1202_1302_s {
    int rtc_type;
    int clock_halt;
    time_t clock_halt_latch;
    int am_pm;
    int write_protect;
    time_t latch;
    time_t *offset;
    BYTE clock_regs[8];
    BYTE trickle_charge;
    BYTE *ram;
    BYTE state;
    BYTE reg;
    BYTE bit;
    BYTE output_bit;
    BYTE io_byte;
    BYTE sclk_line;
    BYTE clock_register;
} rtc_ds1202_1302_t;

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

extern void ds1202_1302_reset(rtc_ds1202_1302_t *context);
extern rtc_ds1202_1302_t *ds1202_1302_init(BYTE *data, time_t *offset, int rtc_type);
extern void ds1202_1302_destroy(rtc_ds1202_1302_t *context);

extern void ds1202_1302_set_lines(rtc_ds1202_1302_t *context, unsigned int ce_line, unsigned int sclk_line, unsigned int input_bit);
extern BYTE ds1202_1302_read_data_line(rtc_ds1202_1302_t *context);

struct snapshot_s;
extern int ds1202_1302_snapshot_read_module(struct snapshot_s *s);
extern int ds1202_1302_snapshot_write_module(struct snapshot_s *s);

#endif
