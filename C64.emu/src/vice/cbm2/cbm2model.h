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

/* TODO: turn into enum so a compiler can check missing cases */
#define CBM2MODEL_510_PAL           0
#define CBM2MODEL_510_NTSC          1
#define CBM2MODEL_610_PAL           2
#define CBM2MODEL_610_NTSC          3
#define CBM2MODEL_620_PAL           4
#define CBM2MODEL_620_NTSC          5
#define CBM2MODEL_620PLUS_PAL       6
#define CBM2MODEL_620PLUS_NTSC      7
#define CBM2MODEL_710_NTSC          8
#define CBM2MODEL_720_NTSC          9
#define CBM2MODEL_720PLUS_NTSC     10

#define CBM2MODEL_NUM 11

#define CBM2MODEL_UNKNOWN 99

#define HAS_CRTC    0
#define HAS_VICII   1

#define LINE_7x0_50HZ  0
#define LINE_6x0_60HZ  1
#define LINE_6x0_50HZ  2

int cbm2model_get(void);
void cbm2model_set(int model);

#endif
