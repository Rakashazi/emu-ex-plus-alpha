/*
 * userport_wic64.h:
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_USERPORT_WIC64_H
#define VICE_USERPORT_WIC64_H

#include "types.h"

#define WIC64_MAXTRACELEVEL 3   /* adjust if needed more */
/* timezone mapping
   C64 sends just a number 0-31, bcd little endian in commandbuffer.
   offsets can then be calculated.
   TBD, Fixme for day-wraparounds incl. dates
*/
typedef struct tzones
{
    int idx;
    char *tz_name;
    int hour_offs;
    int min_offs;
    int dst;                    /* add DST or not, still not perfekt */
} tzones_t;

int  userport_wic64_resources_init(void);
void userport_wic64_resources_shutdown(void);
int  userport_wic64_cmdline_options_init(void);
void userport_wic64_factory_reset(void);

const tzones_t *userport_wic64_get_timezones(size_t *num_zones);

#endif
