/*
 * lightpen.h - Lightpen/gun emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifndef VICE_LIGHTPEN_H
#define VICE_LIGHTPEN_H

#include "types.h"
#include "joyport.h"

void lightpen_init(void);

int lightpen_u_joyport_register(void);
int lightpen_l_joyport_register(void);
int lightpen_datel_joyport_register(void);
int lightgun_y_joyport_register(void);
int lightgun_l_joyport_register(void);
int lightpen_inkwell_joyport_register(void);
int lightgun_gunstick_joyport_register(void);

extern int lightpen_enabled;

enum {
    LIGHTPEN_TYPE_PEN_U = 0,
    LIGHTPEN_TYPE_PEN_L,
    LIGHTPEN_TYPE_PEN_DATEL,
    LIGHTPEN_TYPE_GUN_Y,
    LIGHTPEN_TYPE_GUN_L,
    LIGHTPEN_TYPE_INKWELL,
#ifdef JOYPORT_EXPERIMENTAL_DEVICES
    LIGHTPEN_TYPE_GUNSTICK,
#endif
    LIGHTPEN_TYPE_NUM
};

typedef CLOCK lightpen_timing_callback_t(int x, int y);
typedef lightpen_timing_callback_t *lightpen_timing_callback_ptr_t;
int lightpen_register_timing_callback(lightpen_timing_callback_ptr_t timing_callback, int window);

typedef void lightpen_trigger_callback_t(CLOCK mclk);
typedef lightpen_trigger_callback_t *lightpen_trigger_callback_ptr_t;
int lightpen_register_trigger_callback(lightpen_trigger_callback_ptr_t trigger_callback);

void lightpen_update(int window, int x, int y, int buttons);

/* Host mouse button bitmasks. (the value 4 for the right mouse button comes from SDL) */
#define LP_HOST_BUTTON_1    1
#define LP_HOST_BUTTON_2    4

#endif
