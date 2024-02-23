/*
 * ds1216.h - DS1216E RTC emulation.
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

#ifndef VICE_DS1216E_H
#define VICE_DS1216E_H

#include <time.h>

#include "snapshot.h"
#include "types.h"

#define DS1216E_REG_SIZE   8

typedef struct rtc_ds1216e_s {
    int reset;
    int inactive;
    int hours12;
    int pattern_pos;
    int pattern_ignore;
    int output;
    int output_pos;
    time_t latch;
    time_t offset;
    time_t old_offset;
    uint8_t *clock_regs;
    uint8_t old_clock_regs[DS1216E_REG_SIZE];
    uint8_t clock_regs_changed[DS1216E_REG_SIZE];
    char *device;
} rtc_ds1216e_t;

enum {
    DS1216E_REGISTER_CENTISECONDS = 0,
    DS1216E_REGISTER_SECONDS,
    DS1216E_REGISTER_MINUTES,
    DS1216E_REGISTER_HOURS,
    DS1216E_REGISTER_WEEKDAYS,
    DS1216E_REGISTER_MONTHDAYS,
    DS1216E_REGISTER_MONTHS,
    DS1216E_REGISTER_YEARS
};

rtc_ds1216e_t *ds1216e_init(char *device);
void ds1216e_destroy(rtc_ds1216e_t *context, int save);

uint8_t ds1216e_read(rtc_ds1216e_t *context, uint16_t address, uint8_t original_read);

int ds1216e_write_snapshot(rtc_ds1216e_t *context, snapshot_t *s);
int ds1216e_read_snapshot(rtc_ds1216e_t *context, snapshot_t *s);

#endif
