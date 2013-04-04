/*
 * bq4830y.h - BQ4830Y RTC emulation.
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

#ifndef VICE_BQ4830Y_H
#define VICE_BQ4830Y_H

#include <time.h>

#include "types.h"

typedef struct rtc_bq4830y_s {
    int clock_halt;
    time_t clock_halt_latch;
    int read_latch;
    int write_latch;
    time_t latch;
    time_t *offset;
    BYTE clock_regs[8];
    BYTE clock_regs_changed[8];
    BYTE *ram;
} rtc_bq4830y_t;

#define BQ4830Y_REG_CONTROL         0x7FF8
#define BQ4830Y_REG_SECONDS         0x7FF9
#define BQ4830Y_REG_MINUTES         0x7FFA
#define BQ4830Y_REG_HOURS           0x7FFB
#define BQ4830Y_REG_DAYS_OF_WEEK    0x7FFC
#define BQ4830Y_REG_DAYS_OF_MONTH   0x7FFD
#define BQ4830Y_REG_MONTHS          0x7FFE
#define BQ4830Y_REG_YEARS           0x7FFF

#define LATCH_NONE               0
#define READ_LATCH               1
#define WRITE_LATCH              2
#define READ_WRITE_LATCH         3
#define CLOCK_LATCH              4
#define CLOCK_READ_LATCH         5
#define CLOCK_WRITE_LATCH        6
#define CLOCK_READ_WRITE_LATCH   7

extern rtc_bq4830y_t *bq4830y_init(BYTE *ram, time_t *offset);
extern void bq4830y_destroy(rtc_bq4830y_t *context);

extern void bq4830y_store(rtc_bq4830y_t *context, WORD address, BYTE val);
extern BYTE bq4830y_read(rtc_bq4830y_t *context, WORD address);

#endif
