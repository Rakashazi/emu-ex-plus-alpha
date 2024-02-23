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

#include "snapshot.h"
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
    uint8_t *clock_regs;
    uint8_t old_clock_regs[DS12C887_REG_SIZE];
    uint8_t clock_regs_changed[DS12C887_REG_SIZE];
    uint8_t ctrl_regs[2];
    uint8_t *ram;
    uint8_t old_ram[DS12C887_RAM_SIZE];
    uint8_t reg;
    uint8_t prev_second;
    char *device;
} rtc_ds12c887_t;

enum {
    DS12C887_REG_SECONDS = 0,
    DS12C887_REG_SECONDS_ALARM,
    DS12C887_REG_MINUTES,
    DS12C887_REG_MINUTES_ALARM,
    DS12C887_REG_HOURS,
    DS12C887_REG_HOURS_ALARM,
    DS12C887_REG_DAY_OF_WEEK,
    DS12C887_REG_DAY_OF_MONTH,
    DS12C887_REG_MONTHS,
    DS12C887_REG_YEARS,
    DS12C887_REG_CTRL_A,
    DS12C887_REG_CTRL_B,
    DS12C887_REG_CTRL_C,
    DS12C887_REG_CTRL_D,
    DS12C887_REG_CENTURIES
};

void ds12c887_reset(rtc_ds12c887_t *context);
rtc_ds12c887_t *ds12c887_init(char *device);
void ds12c887_destroy(rtc_ds12c887_t *context, int save);

/* This function needs to be called at least every 1/10th of a second
 * it returns a 1 if an IRQ was generated */
int ds12c887_update_flags(rtc_ds12c887_t *context);

void ds12c887_store_address(rtc_ds12c887_t *context, uint8_t address);
void ds12c887_store_data(rtc_ds12c887_t *context, uint8_t data);
uint8_t ds12c887_read(rtc_ds12c887_t *context);

int ds12c887_dump(rtc_ds12c887_t *context);

int ds12c887_write_snapshot(rtc_ds12c887_t *context, snapshot_t *s);
int ds12c887_read_snapshot(rtc_ds12c887_t *context, snapshot_t *s);

#endif
