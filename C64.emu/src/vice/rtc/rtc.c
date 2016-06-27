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

#include <string.h>

#include "archapi.h"
#include "ioutil.h"
#include "lib.h"
#include "machine.h"
#include "rtc.h"
#include "util.h"

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
    if (((year % 4) == 0) && ((year % 100) == 0) && ((year % 400) != 0)) {
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
    if (((year % 4) == 0) && ((year % 100) == 0) && ((year % 400) != 0)) {
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
    if (((year % 4) == 0) && ((year % 100) == 0) && ((year % 400) != 0)) {
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
    if (((year % 4) == 0) && ((year % 100) == 0) && ((year % 400) != 0)) {
        is_leap_year = 1;
    }

    /* sanity check */
    if (day < 0 || day > (364 + is_leap_year)) {
        return latch;
    }
    return latch + ((day - local->tm_yday) * 24 * 60 * 60);
}

/* ---------------------------------------------------------------------- */

static char *rtc_ram_to_string(BYTE *ram, int size)
{
    char *temp = lib_malloc((size * 2) + 1);
    int i;

    memset(temp, 0, (size * 2) + 1);
    for (i = 0; i < size; i++) {
        temp[i * 2] = (ram[i] >> 4) + 'a';
        temp[(i * 2) + 1] = (ram[i] & 0xf) + 'a';
    }
    return temp;
}

static BYTE *rtc_string_to_ram(char *str, int size)
{
    BYTE *ram = lib_malloc(size);
    int i;

    for (i = 0; i < size; i++) {
        ram[i] = ((str[i * 2] - 'a') << 4) | (str[(i * 2) + 1] - 'a');
    }
    return ram;
}

static int rtc_is_empty(BYTE *array, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (array[i]) {
            return 0;
        }
    }
    return 1;
}

static void rtc_write_data(FILE *outfile, BYTE *ram, int ram_size, BYTE *regs, int reg_size, char *device, time_t offset)
{
    char *ram_string = NULL;
    char *reg_string = NULL;

    fprintf(outfile, "[%s]\n", machine_name);
    fprintf(outfile, "(%s)\n", device);
    fprintf(outfile, "{%d}\n", (int)offset);
    if (ram_size) {
        if (rtc_is_empty(ram, ram_size)) {
            fprintf(outfile, "<x>\n");
        } else {
            ram_string = rtc_ram_to_string(ram, ram_size);
            fprintf(outfile, "<%s>\n", ram_string);
        }
    } else {
        fprintf(outfile, "<x>\n");
    }
    if (reg_size) {
        if (rtc_is_empty(regs, reg_size)) {
            fprintf(outfile, "\"x\"\n\n");
        } else {
            reg_string = rtc_ram_to_string(regs, reg_size);
            fprintf(outfile, "\"%s\"\n\n", reg_string);
        }
    } else {
        fprintf(outfile, "\"x\"\n");
    }
    if (ram_string) {
        lib_free(ram_string);
    }
    if (reg_string) {
        lib_free(reg_string);
    }
}

static void rtc_write_direct(FILE *outfile, char *ram, char *regs, char *emulator, char *device, char *offset)
{
    fprintf(outfile, "[%s]\n", emulator);
    fprintf(outfile, "(%s)\n", device);
    fprintf(outfile, "{%s}\n", offset);
    fprintf(outfile, "<%s>\n", ram);
    fprintf(outfile, "\"%s\"\n\n", regs);
}

typedef struct rtc_item_s {
    char *emulator;
    char *device;
    char *offset;
    char *ram_data;
    char *reg_data;
} rtc_item_t;

static rtc_item_t rtc_items[RTC_MAX];

#define SEARCH_CHAR(x)                   \
    while (buf[0] != 0 && buf[0] != x) { \
        buf++;                           \
    }                                    \
    if (buf[0] == 0) {                   \
        return 0;                        \
    }

static int rtc_parse_buffer(char *buffer)
{
    int i = 0;
    char *buf = buffer;

    while (buf[0]) {
        SEARCH_CHAR('[')
        buf++;
        rtc_items[i].emulator = buf;
        SEARCH_CHAR(']')
        buf[0] = 0;
        buf++;
        SEARCH_CHAR('(')
        buf++;
        rtc_items[i].device = buf;
        SEARCH_CHAR(')')
        buf[0] = 0;
        buf++;
        SEARCH_CHAR('{')
        buf++;
        rtc_items[i].offset = buf;
        SEARCH_CHAR('}')
        buf[0] = 0;
        buf++;
        SEARCH_CHAR('<')
        buf++;
        rtc_items[i].ram_data = buf;
        SEARCH_CHAR('>')
        buf[0] = 0;
        buf++;
        SEARCH_CHAR('\"')
        buf++;
        rtc_items[i].reg_data = buf;
        SEARCH_CHAR('\"')
        buf[0] = 0;
        buf++;
        while (buf[0] != 0 && buf[0] != '[') {
            buf++;
        }
        if (buf[0] == 0) {
            rtc_items[i + 1].emulator = NULL;
            return 1;
        }
        i++;
        if (i >= RTC_MAX) {
            return 0;
        }
    }
    return 0;
}

void rtc_save_context(BYTE *ram, int ram_size, BYTE *regs, int reg_size, char *device, time_t offset)
{
    FILE *outfile = NULL;
    FILE *infile = NULL;
    char *filename;
    char *indata = NULL;
    size_t len = 0;
    int ok = 0;
    int i;
    char *savedir;

    filename = archdep_default_rtc_file_name();

    /* create the directory where the context should be written first */
    util_fname_split(filename, &savedir, NULL);
    ioutil_mkdir(savedir, IOUTIL_MKDIR_RWXU);
    lib_free(savedir);

    if (util_file_exists(filename)) {
        infile = fopen(filename, "rb");
        if (infile) {
            len = util_file_length(infile);
            indata = lib_malloc(len + 1);
            memset(indata, 0, len + 1);
            if (fread(indata, 1, len, infile) == len) {
                ok = rtc_parse_buffer(indata);
            }
            fclose(infile);
        }
    }
    outfile = fopen(filename, "wb");
    if (outfile) {
        if (ok) {
            for (i = 0; rtc_items[i].emulator; i++) {
                if (!strcmp(machine_name, rtc_items[i].emulator) && !strcmp(device, rtc_items[i].device)) {
                    rtc_write_data(outfile, ram, ram_size, regs, reg_size, device, offset);
                    ok = 0;
                } else {
                    rtc_write_direct(outfile, rtc_items[i].ram_data, rtc_items[i].reg_data, rtc_items[i].emulator, rtc_items[i].device, rtc_items[i].offset);
                }
            }
            if (ok) {
                rtc_write_data(outfile, ram, ram_size, regs, reg_size, device, offset);
            }
        } else {
            rtc_write_data(outfile, ram, ram_size, regs, reg_size, device, offset);
        }
        fclose(outfile);
    }
    if (indata) {
        lib_free(indata);
    }
    lib_free(filename);
}

static BYTE *loaded_ram = NULL;
static BYTE *loaded_regs = NULL;
static time_t loaded_offset = 0;

int rtc_load_context(char *device, int ram_size, int reg_size)
{
    FILE *infile = NULL;
    char *filename = archdep_default_rtc_file_name();
    char *indata = NULL;
    size_t len = 0;
    int ok = 0;
    int i;

    loaded_ram = NULL;
    loaded_regs = NULL;
    loaded_offset = 0;

    if (util_file_exists(filename)) {
        infile = fopen(filename, "rb");
        if (infile) {
            len = util_file_length(infile);
            indata = lib_malloc(len + 1);
            memset(indata, 0, len + 1);
            if (fread(indata, 1, len, infile) == len) {
                ok = rtc_parse_buffer(indata);
            }
            fclose(infile);
            if (!ok) {
                lib_free(indata);
                return 0;
            }
            for (i = 0; rtc_items[i].emulator; i++) {
                if (!strcmp(machine_name, rtc_items[i].emulator) && !strcmp(device, rtc_items[i].device)) {
                    if (ram_size) {
                        if (rtc_items[i].ram_data[0] == 'x') {
                            loaded_ram = lib_malloc(ram_size);
                            memset(loaded_ram, 0, ram_size);
                        } else {
                            loaded_ram = rtc_string_to_ram(rtc_items[i].ram_data, ram_size);
                        }
                    }
                    if (reg_size) {
                        if (rtc_items[i].reg_data[0] == 'x') {
                            loaded_regs = lib_malloc(reg_size);
                            memset(loaded_regs, 0, reg_size);
                        } else {
                            loaded_regs = rtc_string_to_ram(rtc_items[i].reg_data, reg_size);
                        }
                    }
                    loaded_offset = atoi(rtc_items[i].offset);
                    ok = 0;
                }
            }
            lib_free(indata);
            if (ok) {
                return 0;
            } else {
                return 1;
            }
        }
    }
    return 0;
}

BYTE *rtc_get_loaded_ram(void)
{
    return loaded_ram;
}

BYTE *rtc_get_loaded_clockregs(void)
{
    return loaded_regs;
}

time_t rtc_get_loaded_offset(void)
{
    return loaded_offset;
}
