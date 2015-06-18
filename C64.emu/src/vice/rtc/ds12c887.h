/*
 * ds12c887.h - DS12C887 RTC emulation.
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

#ifndef VICE_DS12C887_H
#define VICE_DS12C887_H

#include <time.h>

#include "types.h"

#define DS12C887_RAM_SIZE   128
#define DS12C887_REG_SIZE   11

typedef struct rtc_ds12c887_s {
    int clock_halt;
    time_t clock_halt_latch;
    int am_pm;
    int set;
    time_t set_latch;
    time_t offset;
    time_t old_offset;
    int bcd;
    int alarm_flag;
    int end_of_update_flag;
    BYTE *clock_regs;
    BYTE old_clock_regs[DS12C887_REG_SIZE];
    BYTE clock_regs_changed[DS12C887_REG_SIZE];
    BYTE ctrl_regs[2];
    BYTE *ram;
    BYTE old_ram[DS12C887_RAM_SIZE];
    BYTE reg;
    BYTE prev_second;
    char *device;
} rtc_ds12c887_t;

#define DS12C887_REG_SECONDS         0
#define DS12C887_REG_SECONDS_ALARM   1
#define DS12C887_REG_MINUTES         2
#define DS12C887_REG_MINUTES_ALARM   3
#define DS12C887_REG_HOURS           4
#define DS12C887_REG_HOURS_ALARM     5
#define DS12C887_REG_DAY_OF_WEEK     6
#define DS12C887_REG_DAY_OF_MONTH    7
#define DS12C887_REG_MONTHS          8
#define DS12C887_REG_YEARS           9
#define DS12C887_REG_CTRL_A          10
#define DS12C887_REG_CTRL_B          11
#define DS12C887_REG_CTRL_C          12
#define DS12C887_REG_CTRL_D          13
#define DS12C887_REG_CENTURIES       50

extern void ds12c887_reset(rtc_ds12c887_t *context);
extern rtc_ds12c887_t *ds12c887_init(char *device);
extern void ds12c887_destroy(rtc_ds12c887_t *context, int save);

/* This function needs to be called at least every 1/10th of a second
 * it returns a 1 if an IRQ was generated */
extern int ds12c887_update_flags(rtc_ds12c887_t *context);

extern void ds12c887_store_address(rtc_ds12c887_t *context, BYTE address);
extern void ds12c887_store_data(rtc_ds12c887_t *context, BYTE data);
extern BYTE ds12c887_read(rtc_ds12c887_t *context);

#endif
