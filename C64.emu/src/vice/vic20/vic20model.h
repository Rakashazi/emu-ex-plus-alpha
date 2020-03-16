/*
 * vic20model.h - VIC20 model detection and setting.
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

#ifndef VICE_VIC20MODEL_H
#define VICE_VIC20MODEL_H

#include "types.h"

#define VIC20MODEL_VIC20_PAL        0
#define VIC20MODEL_VIC20_NTSC       1
#define VIC20MODEL_VIC21            2 /* SuperVIC (+16K) */

#define VIC20MODEL_NUM 3

#define VIC20MODEL_UNKNOWN 99

#define NO_EXTRA_RAM   0

extern int vic20model_get(void);
extern void vic20model_set(int model);

#endif
