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
    BYTE *clock_regs;
    BYTE old_clock_regs[DS1216E_REG_SIZE];
    BYTE clock_regs_changed[DS1216E_REG_SIZE];
    char *device;
} rtc_ds1216e_t;

#define DS1216E_REGISTER_CENTISECONDS   0
#define DS1216E_REGISTER_SECONDS        1
#define DS1216E_REGISTER_MINUTES        2
#define DS1216E_REGISTER_HOURS          3
#define DS1216E_REGISTER_WEEKDAYS       4
#define DS1216E_REGISTER_MONTHDAYS      5
#define DS1216E_REGISTER_MONTHS         6
#define DS1216E_REGISTER_YEARS          7

extern rtc_ds1216e_t *ds1216e_init(char *device);
extern void ds1216e_destroy(rtc_ds1216e_t *context, int save);

extern BYTE ds1216e_read(rtc_ds1216e_t *context, WORD address, BYTE original_read);

#endif
