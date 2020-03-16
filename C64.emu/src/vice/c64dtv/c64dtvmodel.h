/*
 * c64dtvmodel.h - DTV model detection and setting.
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

#ifndef VICE_DTVMODEL_H
#define VICE_DTVMODEL_H

#include "types.h"

#define DTVMODEL_V2_PAL        0 /* DTV v2 (pal) */
#define DTVMODEL_V2_NTSC       1 /* DTV v2 (ntsc) */
#define DTVMODEL_V3_PAL        2 /* DTV v3 (pal) */
#define DTVMODEL_V3_NTSC       3 /* DTV v3 (ntsc) */
#define DTVMODEL_HUMMER_NTSC   4 /* Hummer (ntsc) */

#define DTVMODEL_NUM 5

#define DTVMODEL_UNKNOWN 99

#define REV_2  2
#define REV_3  3

#define IS_DTV     0
#define IS_HUMMER  1

extern int dtvmodel_get(void);
extern void dtvmodel_set(int model);

#endif
