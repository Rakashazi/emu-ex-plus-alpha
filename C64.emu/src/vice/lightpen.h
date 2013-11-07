/*
 * lightpen.h - Lightpen/gun emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifndef VICE_LIGHTPEN_H
#define VICE_LIGHTPEN_H

#include "types.h"

extern int lightpen_resources_init(void);
extern int lightpen_cmdline_options_init(void);
extern void lightpen_init(void);

extern int lightpen_enabled;
extern int lightpen_type;
#define LIGHTPEN_TYPE_PEN_U       0
#define LIGHTPEN_TYPE_PEN_L       1
#define LIGHTPEN_TYPE_PEN_DATEL   2
#define LIGHTPEN_TYPE_GUN_Y       3
#define LIGHTPEN_TYPE_GUN_L       4
#define LIGHTPEN_TYPE_INKWELL     5
#define LIGHTPEN_TYPE_NUM         6

typedef CLOCK lightpen_timing_callback_t(int x, int y);
typedef lightpen_timing_callback_t *lightpen_timing_callback_ptr_t;
extern int lightpen_register_timing_callback(lightpen_timing_callback_ptr_t timing_callback, int window);

typedef void lightpen_trigger_callback_t(CLOCK mclk);
typedef lightpen_trigger_callback_t *lightpen_trigger_callback_ptr_t;
extern int lightpen_register_trigger_callback(lightpen_trigger_callback_ptr_t trigger_callback);

extern void lightpen_update(int window, int x, int y, int buttons);
/* Host mouse button bitmasks. (the value 4 for the right mouse button comes from SDL) */
#define LP_HOST_BUTTON_1    1
#define LP_HOST_BUTTON_2    4

extern BYTE lightpen_read_button_y(void);
extern BYTE lightpen_read_button_x(void);

#endif
