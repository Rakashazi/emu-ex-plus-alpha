/*
 * rtc-72421.h - RTC-72421 RTC emulation.
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

#ifndef VICE_RTC72421_H
#define VICE_RTC72421_H

#include <time.h>

#include "snapshot.h"
#include "types.h"

typedef struct rtc_72421_s {
    int stop;
    int hour24;
    time_t latch;
    time_t offset;
    time_t old_offset;
    char *device;
} rtc_72421_t;

#define RTC72421_REGISTER_SECONDS       0
#define RTC72421_REGISTER_10SECONDS     1
#define RTC72421_REGISTER_MINUTES       2
#define RTC72421_REGISTER_10MINUTES     3
#define RTC72421_REGISTER_HOURS         4
#define RTC72421_REGISTER_10HOURS       5
#define RTC72421_REGISTER_MONTHDAYS     6
#define RTC72421_REGISTER_10MONTHDAYS   7
#define RTC72421_REGISTER_MONTHS        8
#define RTC72421_REGISTER_10MONTHS      9
#define RTC72421_REGISTER_YEARS         10
#define RTC72421_REGISTER_10YEARS       11
#define RTC72421_REGISTER_WEEKDAYS      12
#define RTC72421_REGISTER_CTRL0         13
#define RTC72421_REGISTER_CTRL1         14
#define RTC72421_REGISTER_CTRL2         15

extern rtc_72421_t *rtc72421_init(char *device);
extern void rtc72421_destroy(rtc_72421_t *context, int save);

extern BYTE rtc72421_read(rtc_72421_t *context, BYTE address);
extern void rtc72421_write(rtc_72421_t *context, BYTE address, BYTE data);

extern int rtc72421_write_snapshot(rtc_72421_t *context, snapshot_t *s);
extern int rtc72421_read_snapshot(rtc_72421_t *context, snapshot_t *s);

#endif
