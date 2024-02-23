/*
 * rtc-58321a.h - RTC-58321A RTC emulation.
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

#ifndef VICE_RTC58321A_H
#define VICE_RTC58321A_H

#include <time.h>

#include "snapshot.h"
#include "types.h"

typedef struct rtc_58321a_s {
    int stop;
    int hour24;
    uint8_t address;
    time_t latch;
    time_t offset;
    time_t old_offset;
    char *device;
} rtc_58321a_t;

enum {
    RTC58321A_REGISTER_SECONDS = 0,
    RTC58321A_REGISTER_10SECONDS,
    RTC58321A_REGISTER_MINUTES,
    RTC58321A_REGISTER_10MINUTES,
    RTC58321A_REGISTER_HOURS,
    RTC58321A_REGISTER_10HOURS,
    RTC58321A_REGISTER_WEEKDAYS,
    RTC58321A_REGISTER_MONTHDAYS,
    RTC58321A_REGISTER_10MONTHDAYS,
    RTC58321A_REGISTER_MONTHS,
    RTC58321A_REGISTER_10MONTHS,
    RTC58321A_REGISTER_YEARS,
    RTC58321A_REGISTER_10YEARS,
    RTC58321A_REGISTER_RESET,
    RTC58321A_REGISTER_SS0,
    RTC58321A_REGISTER_SS1
};

rtc_58321a_t *rtc58321a_init(char *device);
void rtc58321a_destroy(rtc_58321a_t *context, int save);

uint8_t rtc58321a_read(rtc_58321a_t *context);
void rtc58321a_write_address(rtc_58321a_t *context, uint8_t address);
void rtc58321a_write_data(rtc_58321a_t *context, uint8_t data);

void rtc58321a_stop_clock(rtc_58321a_t *context);
void rtc58321a_start_clock(rtc_58321a_t *context);

int rtc58321a_write_snapshot(rtc_58321a_t *context, snapshot_t *s);
int rtc58321a_read_snapshot(rtc_58321a_t *context, snapshot_t *s);

#endif
