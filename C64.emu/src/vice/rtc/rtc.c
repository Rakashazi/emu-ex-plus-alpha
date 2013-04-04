/*
 * rtc.c - Time get/set routines for RTC emulation.
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

#include "vice.h"

#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "archapi.h"
#include "rtc.h"

inline static int int_to_bcd(int dec)
{
    return ((dec / 10) << 4) + (dec % 10);
}

inline static int bcd_to_int(int bcd)
{
    return ((bcd >> 4) * 10) + bcd % 16;
}

/* ---------------------------------------------------------------------- */

/* get 1/100 seconds from clock */
BYTE rtc_get_centisecond(int bcd)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval t;

    gettimeofday(&t, NULL);
    return (BYTE)((bcd) ? int_to_bcd(t.tv_usec / 10000) : t.tv_usec / 10000);
#else
    return (BYTE)((bcd) ? int_to_bcd(archdep_rtc_get_centisecond()) : archdep_rtc_get_centisecond());
#endif
}

/* get seconds from time value
   0 - 61 (leap seconds would be 60 and 61) */
BYTE rtc_get_second(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_sec) : local->tm_sec);
}

/* get minutes from time value
   0 - 59 */
BYTE rtc_get_minute(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_min) : local->tm_min);
}

/* get hours from time value
   0 - 23 */
BYTE rtc_get_hour(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_hour) : local->tm_hour);
}

/* get hours from time value
   1 - 12 + AM/PM flag in bit 5 (0 = PM, 1 = AM) */
BYTE rtc_get_hour_am_pm(time_t time_val, int bcd)
{
    BYTE hour;
    int pm = 0;
    time_t now = time_val;
    struct tm *local = localtime(&now);

    hour = local->tm_hour;

    if (hour == 0) {
        hour = 12;
    } else if (hour == 12) {
        pm = 1;
    } else if (hour > 12) {
        hour -= 12;
        pm = 1;
    }
    hour = (BYTE)((bcd) ? int_to_bcd(hour) : hour);
    hour |= (pm << 5);
    return hour;
}

/* get day of month from time value
   1 - 31 */
BYTE rtc_get_day_of_month(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_mday) : local->tm_mday);
}

/* get month from time value
   1 - 12 */
BYTE rtc_get_month(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_mon + 1) : (local->tm_mon + 1));
}

/* get year of the century from time value
   0 - 99 */
BYTE rtc_get_year(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd(local->tm_year % 100) : local->tm_year % 100);
}

/* get the century from time value
   19 - 20 */
BYTE rtc_get_century(time_t time_val, int bcd)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)((bcd) ? int_to_bcd((int)(local->tm_year / 100) + 19) : (int)(local->tm_year / 100) + 19);
}

/* get the day of the week from time value
   0 - 6 (sunday 0, monday 1 ...etc) */
BYTE rtc_get_weekday(time_t time_val)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (BYTE)local->tm_wday;
}

/* get the day of the year from time value
   0 - 365 */
WORD rtc_get_day_of_year(time_t time_val)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return (WORD)local->tm_yday;
}

/* get the DST from time value
   0 - >0 (0 no dst, >0 dst) */
int rtc_get_dst(time_t time_val)
{
    time_t now = time_val;
    struct tm *local = localtime(&now);

    return local->tm_isdst;
}

/* get the current clock based on time + offset so the value can be latched */
time_t rtc_get_latch(time_t offset)
{
    return time(NULL) + offset;
}

/* ---------------------------------------------------------------------- */

/* set seconds and returns new offset
   0 - 59 */
time_t rtc_set_second(int seconds, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_seconds = (bcd) ? bcd_to_int(seconds) : seconds;

    /* sanity check and disregard setting of leap seconds */
    if (real_seconds < 0 || real_seconds > 59) {
        return offset;
    }
    local->tm_sec = real_seconds;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set minutes and returns new offset
   0 - 59 */
time_t rtc_set_minute(int minutes, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_minutes = (bcd) ? bcd_to_int(minutes) : minutes;

    /* sanity check */
    if (real_minutes < 0 || real_minutes > 59) {
        return offset;
    }
    local->tm_min = real_minutes;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set hours and returns new offset
   0 - 23 */
time_t rtc_set_hour(int hours, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_hours = (bcd) ? bcd_to_int(hours) : hours;

    /* sanity check */
    if (real_hours < 0 || real_hours > 23) {
        return offset;
    }
    local->tm_hour = real_hours;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set hours and returns new offset
   1 - 12 and AM/PM indicator */
time_t rtc_set_hour_am_pm(int hours, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_hours = (bcd) ? bcd_to_int(hours & 0x1f) : hours & 0x1f;
    int pm = (hours & 0x20) >> 5;

    if (real_hours == 12 && !pm) {
        real_hours = 0;
    } else if (real_hours == 12 && pm) {
    } else {
        real_hours += 12;
    }

    /* sanity check */
    if (real_hours < 0 || real_hours > 23) {
        return offset;
    }
    local->tm_hour = real_hours;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set day of month and returns new offset
   1 - 31 */
time_t rtc_set_day_of_month(int day, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int is_leap_year = 0;
    int year = local->tm_year + 1900;
    int real_day = (bcd) ? bcd_to_int(day) : day;

    /* sanity check */
    if (((year % 4) == 0) && ((year % 100) != 0)) {
        is_leap_year = 1;
    }
    if (((year % 4) == 0) & ((year % 100) == 0) && ((year % 400) != 0)) {
        is_leap_year = 1;
    }
    switch (local->tm_mon) {
        case RTC_MONTH_JAN:
        case RTC_MONTH_MAR:
        case RTC_MONTH_MAY:
        case RTC_MONTH_JUL:
        case RTC_MONTH_AUG:
        case RTC_MONTH_OCT:
        case RTC_MONTH_DEC:
            if (real_day < 1 || real_day > 31) {
                return offset;
            }
            break;
        case RTC_MONTH_APR:
        case RTC_MONTH_JUN:
        case RTC_MONTH_SEP:
        case RTC_MONTH_NOV:
            if (real_day < 1 || real_day > 30) {
                return offset;
            }
            break;
        case RTC_MONTH_FEB:
            if (real_day < 1 || real_day > (28 + is_leap_year)) {
                return offset;
            }
            break;
    }
    local->tm_mday = real_day;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set month and returns new offset
   1 - 12 */
time_t rtc_set_month(int month, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_month = (bcd) ? bcd_to_int(month) : month;

    /* sanity check */
    if (real_month < 1 || real_month > 12) {
        return offset;
    }
    local->tm_mon = real_month - 1;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set years and returns new offset
   0 - 99 */
time_t rtc_set_year(int year, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_year = (bcd) ? bcd_to_int(year) : year;

    /* sanity check */
    if (real_year < 0 || real_year > 99) {
        return offset;
    }
    local->tm_year = (local->tm_year / 100) * 100;
    local->tm_year += real_year;
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set century and returns new offset
   19 - 20 */
time_t rtc_set_century(int century, time_t offset, int bcd)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_century = (bcd) ? bcd_to_int(century) : century;

    /* sanity check */
    if (real_century != 19 && real_century != 20) {
        return offset;
    }
    local->tm_year %= 100;
    local->tm_year += ((real_century - 19) * 100);
    offset_now = mktime(local);

    return offset + (offset_now - now);
}

/* set weekday and returns new offset
   0 - 6 */
time_t rtc_set_weekday(int day, time_t offset)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);

    /* sanity check */
    if (day < 0 || day > 6) {
        return offset;
    }
    return offset + ((day - local->tm_wday) * 24 * 60 * 60);
}

/* set day of the year and returns new offset
   0 - 365 */
time_t rtc_set_day_of_year(int day, time_t offset)
{
    time_t now = time(NULL) + offset;
    struct tm *local = localtime(&now);
    int is_leap_year = 0;
    int year = local->tm_year + 1900;

    /* sanity check */
    if (((year % 4) == 0) && ((year % 100) != 0)) {
        is_leap_year = 1;
    }
    if (((year % 4) == 0) & ((year % 100) == 0) && ((year % 400) != 0)) {
        is_leap_year = 1;
    }

    /* sanity check */
    if (day < 0 || day > (364 + is_leap_year)) {
        return offset;
    }
    return offset + ((day - local->tm_yday) * 24 * 60 * 60);
}

/* ---------------------------------------------------------------------- */

/* set seconds and returns new latched value
   0 - 59 */
time_t rtc_set_latched_second(int seconds, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_seconds = (bcd) ? bcd_to_int(seconds) : seconds;

    /* sanity check and disregard setting of leap seconds */
    if (real_seconds < 0 || real_seconds > 59) {
        return latch;
    }
    local->tm_sec = real_seconds;
    offset_now = mktime(local);

    return offset_now;
}

/* set minutes and returns new latched value
   0 - 59 */
time_t rtc_set_latched_minute(int minutes, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_minutes = (bcd) ? bcd_to_int(minutes) : minutes;

    /* sanity check */
    if (real_minutes < 0 || real_minutes > 59) {
        return latch;
    }
    local->tm_min = real_minutes;
    offset_now = mktime(local);

    return offset_now;
}

/* set hours and returns new latched value
   0 - 23 */
time_t rtc_set_latched_hour(int hours, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_hours = (bcd) ? bcd_to_int(hours) : hours;

    /* sanity check */
    if (real_hours < 0 || real_hours > 23) {
        return latch;
    }
    local->tm_hour = real_hours;
    offset_now = mktime(local);

    return offset_now;
}

/* set hours and returns new offset
   0 - 23 */
time_t rtc_set_latched_hour_am_pm(int hours, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_hours = (bcd) ? bcd_to_int(hours & 0x1f) : hours & 0x1f;
    int pm = (hours & 0x20) >> 5;

    if (real_hours == 12 && !pm) {
        real_hours = 0;
    } else if (real_hours == 12 && pm) {
    } else {
        real_hours += 12;
    }

    /* sanity check */
    if (real_hours < 0 || real_hours > 23) {
        return latch;
    }
    local->tm_hour = real_hours;
    offset_now = mktime(local);

    return offset_now;
}

/* set day of month and returns new latched value
   1 - 31 */
time_t rtc_set_latched_day_of_month(int day, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int is_leap_year = 0;
    int year = local->tm_year + 1900;
    int real_day = (bcd) ? bcd_to_int(day) : day;

    /* sanity check */
    if (((year % 4) == 0) && ((year % 100) != 0)) {
        is_leap_year = 1;
    }
    if (((year % 4) == 0) & ((year % 100) == 0) && ((year % 400) != 0)) {
        is_leap_year = 1;
    }
    switch (local->tm_mon) {
        case RTC_MONTH_JAN:
        case RTC_MONTH_MAR:
        case RTC_MONTH_MAY:
        case RTC_MONTH_JUL:
        case RTC_MONTH_AUG:
        case RTC_MONTH_OCT:
        case RTC_MONTH_DEC:
            if (real_day < 1 || real_day > 31) {
                return latch;
            }
            break;
        case RTC_MONTH_APR:
        case RTC_MONTH_JUN:
        case RTC_MONTH_SEP:
        case RTC_MONTH_NOV:
            if (real_day < 1 || real_day > 30) {
                return latch;
            }
            break;
        case RTC_MONTH_FEB:
            if (real_day < 1 || real_day > (28 + is_leap_year)) {
                return latch;
            }
            break;
    }
    local->tm_mday = real_day;
    offset_now = mktime(local);

    return offset_now;
}

/* set month and returns new latched value
   1 - 12 */
time_t rtc_set_latched_month(int month, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_month = (bcd) ? bcd_to_int(month) : month;

    /* sanity check */
    if (real_month < 1 || real_month > 12) {
        return latch;
    }
    local->tm_mon = real_month - 1;
    offset_now = mktime(local);

    return offset_now;
}

/* set years and returns new latched value
   0 - 99 */
time_t rtc_set_latched_year(int year, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_year = (bcd) ? bcd_to_int(year) : year;

    /* sanity check */
    if (real_year < 0 || real_year > 99) {
        return latch;
    }
    local->tm_year = (local->tm_year / 100) * 100;
    local->tm_year += real_year;
    offset_now = mktime(local);

    return offset_now;
}

/* set century and returns new latched value
   19 - 20 */
time_t rtc_set_latched_century(int century, time_t latch, int bcd)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    time_t offset_now;
    int real_century = (bcd) ? bcd_to_int(century) : century;

    /* sanity check */
    if (real_century != 19 && real_century != 20) {
        return latch;
    }
    local->tm_year %= 100;
    local->tm_year += ((real_century - 19) * 100);
    offset_now = mktime(local);

    return offset_now;
}

/* set weekday and returns new latched value
   0 - 6 */
time_t rtc_set_latched_weekday(int day, time_t latch)
{
    time_t now = latch;
    struct tm *local = localtime(&now);

    /* sanity check */
    if (day < 0 || day > 6) {
        return latch;
    }
    return latch + ((day - local->tm_wday) * 24 * 60 * 60);
}

/* set day of the year and returns new latched value
   0 - 365 */
time_t rtc_set_latched_day_of_year(int day, time_t latch)
{
    time_t now = latch;
    struct tm *local = localtime(&now);
    int is_leap_year = 0;
    int year = local->tm_year + 1900;

    /* sanity check */
    if (((year % 4) == 0) && ((year % 100) != 0)) {
        is_leap_year = 1;
    }
    if (((year % 4) == 0) & ((year % 100) == 0) && ((year % 400) != 0)) {
        is_leap_year = 1;
    }

    /* sanity check */
    if (day < 0 || day > (364 + is_leap_year)) {
        return latch;
    }
    return latch + ((day - local->tm_yday) * 24 * 60 * 60);
}
