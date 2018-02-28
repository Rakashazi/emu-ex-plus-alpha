/*
 * ds1602.h - DS1602 RTC emulation.
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

#ifndef VICE_DS1602_H
#define VICE_DS1602_H

#include "snapshot.h"
#include "types.h"

#include <time.h>

typedef struct rtc_ds1602_s rtc_ds1602_t;

extern rtc_ds1602_t *ds1602_init(char *device, time_t offset0);
extern void ds1602_destroy(rtc_ds1602_t *context, int save);

extern void ds1602_set_reset_line(rtc_ds1602_t *context, BYTE data);
extern void ds1602_set_clk_line(rtc_ds1602_t *context, BYTE data);
extern void ds1602_set_data_line(rtc_ds1602_t *context, BYTE data);

extern BYTE ds1602_read_data_line(rtc_ds1602_t *context);

extern int ds1602_write_snapshot(rtc_ds1602_t *context, snapshot_t *s);
extern int ds1602_read_snapshot(rtc_ds1602_t *context, snapshot_t *s);

#endif
