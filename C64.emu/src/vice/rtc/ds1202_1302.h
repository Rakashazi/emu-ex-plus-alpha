/*
 * ds1202_1302.h - DS1202/1302 RTC emulation.
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

#ifndef VICE_DS1202_1302_H
#define VICE_DS1202_1302_H

#include "snapshot.h"
#include "types.h"

typedef struct rtc_ds1202_1302_s rtc_ds1202_1302_t;

extern void ds1202_1302_reset(rtc_ds1202_1302_t *context);
extern rtc_ds1202_1302_t *ds1202_1302_init(char *device, int rtc_type);
extern void ds1202_1302_destroy(rtc_ds1202_1302_t *context, int save);

extern void ds1202_1302_set_lines(rtc_ds1202_1302_t *context, unsigned int ce_line, unsigned int sclk_line, unsigned int input_bit);
extern BYTE ds1202_1302_read_data_line(rtc_ds1202_1302_t *context);

extern int ds1202_1302_dump(rtc_ds1202_1302_t *context);

extern int ds1202_1302_write_snapshot(rtc_ds1202_1302_t *context, snapshot_t *s);
extern int ds1202_1302_read_snapshot(rtc_ds1202_1302_t *context, snapshot_t *s);

#endif
