/*
 * c128model.h - C64 model detection and setting.
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

#ifndef VICE_C128MODEL_H
#define VICE_C128MODEL_H

#include "types.h"

enum {
    C128MODEL_C128_PAL = 0,
    C128MODEL_C128D_PAL,
    C128MODEL_C128DCR_PAL,

    C128MODEL_C128_NTSC,
    C128MODEL_C128D_NTSC,
    C128MODEL_C128DCR_NTSC,

/* This entry always needs to be at the end */
    C128MODEL_NUM
};

#define C128MODEL_UNKNOWN 99

#define BOARD_C128  0
#define BOARD_C128D 1

#define OLD_CIA   0
#define NEW_CIA   1

#define OLD_SID   0
#define NEW_SID   1

#define VDC16K   0
#define VDC64K   1

int c128model_get(void);
void c128model_set(int model);

#endif
