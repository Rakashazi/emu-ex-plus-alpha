/*
 * rtc.h - Time get/set routines for RTC emulation.
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

#ifndef VICE_RTC_H
#define VICE_RTC_H

#include "types.h"

#define RTC_MONTH_JAN   0
#define RTC_MONTH_FEB   1
#define RTC_MONTH_MAR   2
#define RTC_MONTH_APR   3
#define RTC_MONTH_MAY   4
#define RTC_MONTH_JUN   5
#define RTC_MONTH_JUL   6
#define RTC_MONTH_AUG   7
#define RTC_MONTH_SEP   8
#define RTC_MONTH_OCT   9
#define RTC_MONTH_NOV   10
#define RTC_MONTH_DEC   11

extern BYTE rtc_get_centisecond(int bcd);

extern BYTE rtc_get_second(time_t time_val, int bcd);         /* 0 - 61 (leap seconds would be 60 and 61) */
extern BYTE rtc_get_minute(time_t time_val, int bcd);         /* 0 - 59 */
extern BYTE rtc_get_hour(time_t time_val, int bcd);           /* 0 - 23 */
extern BYTE rtc_get_hour_am_pm(time_t time_val, int bcd);     /* 1 - 12 + AM/PM in bit 5 (0 = AM, 1 = PM) */
extern BYTE rtc_get_day_of_month(time_t time_val, int bcd);   /* 1 - 31 */
extern BYTE rtc_get_month(time_t time_val, int bcd);          /* 1 - 12 (1 = January, 2 = Febuary ...etc) */
extern BYTE rtc_get_year(time_t time_val, int bcd);           /* 0 - 99 */
extern BYTE rtc_get_century(time_t time_val, int bcd);        /* 19 - 20 */
extern BYTE rtc_get_weekday(time_t time_val);                 /* 0 - 6 (sunday 0, monday 1 ...etc) */
extern WORD rtc_get_day_of_year(time_t time_val);             /* 0 - 365 */
extern int rtc_get_dst(time_t time_val);                     /* 0 - >0 (0 no dst, >0 dst) */
extern time_t rtc_get_latch(time_t offset);

/* these functions all return a new offset based on what is changed and the old offset */

extern time_t rtc_set_second(int seconds, time_t offset, int bcd);     /* 0 - 61 (leap seconds would be 60 and 61) */
extern time_t rtc_set_minute(int minutes, time_t offset, int bcd);     /* 0 - 59 */
extern time_t rtc_set_hour(int hours, time_t offset, int bcd);         /* 0 - 23 */
extern time_t rtc_set_hour_am_pm(int hours, time_t offset, int bcd);   /* 1 - 12 + AM/PM in bit 5 (0 = AM, 1 = PM) */
extern time_t rtc_set_day_of_month(int day, time_t offset, int bcd);   /* 1 - 31 */
extern time_t rtc_set_month(int month, time_t offset, int bcd);        /* 1 - 12 */
extern time_t rtc_set_year(int year, time_t offset, int bcd);          /* 0 - 99 */
extern time_t rtc_set_century(int year, time_t offset, int bcd);       /* 19 - 20 */
extern time_t rtc_set_weekday(int day, time_t offset);                 /* 0 - 6 (sunday 0, monday 1 ...etc) */
extern time_t rtc_set_day_of_year(int day, time_t offset);             /* 0 - 365 */

/* these functions all return a new latch based on what is changed and the old latch */
extern time_t rtc_set_latched_second(int seconds, time_t latch, int bcd);     /* 0 - 61 (leap seconds would be 60 and 61) */
extern time_t rtc_set_latched_minute(int minutes, time_t latch, int bcd);     /* 0 - 59 */
extern time_t rtc_set_latched_hour(int hours, time_t latch, int bcd);         /* 0 - 23 */
extern time_t rtc_set_latched_hour_am_pm(int hours, time_t latch, int bcd);   /* 1 - 12 + AM/PM in bit 5 (0 = AM, 1 = PM) */
extern time_t rtc_set_latched_day_of_month(int day, time_t latch, int bcd);   /* 1 - 31 */
extern time_t rtc_set_latched_month(int month, time_t latch, int bcd);        /* 1 - 12 */
extern time_t rtc_set_latched_year(int year, time_t latch, int bcd);          /* 0 - 99 */
extern time_t rtc_set_latched_century(int year, time_t latch, int bcd);       /* 19 - 20 */
extern time_t rtc_set_latched_weekday(int day, time_t latch);                 /* 0 - 6 (sunday 0, monday 1 ...etc) */
extern time_t rtc_set_latched_day_of_year(int day, time_t latch);             /* 0 - 365 */

#endif
