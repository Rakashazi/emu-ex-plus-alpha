/*
 * ds1307.h - DS1307 RTC emulation.
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

#ifndef VICE_DS1307_H
#define VICE_DS1307_H

#include "snapshot.h"
#include "types.h"

typedef struct rtc_ds1307_s rtc_ds1307_t;

extern rtc_ds1307_t *ds1307_init(char *device);
extern void ds1307_destroy(rtc_ds1307_t *context, int save);

extern void ds1307_set_clk_line(rtc_ds1307_t *context, BYTE data);
extern void ds1307_set_data_line(rtc_ds1307_t *context, BYTE data);

extern BYTE ds1307_read_data_line(rtc_ds1307_t *context);

extern int ds1307_write_snapshot(rtc_ds1307_t *context, snapshot_t *s);
extern int ds1307_read_snapshot(rtc_ds1307_t *context, snapshot_t *s);

#endif
