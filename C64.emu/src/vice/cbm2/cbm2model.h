/*
 * cbm2model.h - CBM2 model detection and setting.
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

#ifndef VICE_CBM2MODEL_H
#define VICE_CBM2MODEL_H

#include "types.h"

enum {
    CBM2MODEL_510_PAL = 0,
    CBM2MODEL_510_NTSC,
    CBM2MODEL_610_PAL,
    CBM2MODEL_610_NTSC,
    CBM2MODEL_620_PAL,
    CBM2MODEL_620_NTSC,
    CBM2MODEL_620PLUS_PAL,
    CBM2MODEL_620PLUS_NTSC,
    CBM2MODEL_710_NTSC,
    CBM2MODEL_720_NTSC,
    CBM2MODEL_720PLUS_NTSC,

    /* This entry always needs to be at the end */
    CBM2MODEL_NUM
};

#define CBM2MODEL_UNKNOWN 99

#define HAS_CRTC    0
#define HAS_VICII   1

enum {
    LINE_7x0_50HZ = 0,
    LINE_6x0_60HZ,
    LINE_6x0_50HZ
};

int cbm2model_get(void);
void cbm2model_set(int model);

#endif
