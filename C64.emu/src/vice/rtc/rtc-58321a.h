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

#include "types.h"

typedef struct rtc_58321a_s {
    int stop;
    int hour24;
    BYTE address;
    time_t latch;
    time_t *offset;
} rtc_58321a_t;

#define RTC58321A_REGISTER_SECONDS       0
#define RTC58321A_REGISTER_10SECONDS     1
#define RTC58321A_REGISTER_MINUTES       2
#define RTC58321A_REGISTER_10MINUTES     3
#define RTC58321A_REGISTER_HOURS         4
#define RTC58321A_REGISTER_10HOURS       5
#define RTC58321A_REGISTER_WEEKDAYS      6
#define RTC58321A_REGISTER_MONTHDAYS     7
#define RTC58321A_REGISTER_10MONTHDAYS   8
#define RTC58321A_REGISTER_MONTHS        9
#define RTC58321A_REGISTER_10MONTHS      10
#define RTC58321A_REGISTER_YEARS         11
#define RTC58321A_REGISTER_10YEARS       12
#define RTC58321A_REGISTER_RESET         13
#define RTC58321A_REGISTER_SS0           14
#define RTC58321A_REGISTER_SS1           15

extern rtc_58321a_t *rtc58321a_init(time_t *offset);
extern void rtc58321a_destroy(rtc_58321a_t *context);

extern BYTE rtc58321a_read(rtc_58321a_t *context);
extern void rtc58321a_write_address(rtc_58321a_t *context, BYTE address);
extern void rtc58321a_write_data(rtc_58321a_t *context, BYTE data);

extern void rtc58321a_stop_clock(rtc_58321a_t *context);
extern void rtc58321a_start_clock(rtc_58321a_t *context);

#endif
