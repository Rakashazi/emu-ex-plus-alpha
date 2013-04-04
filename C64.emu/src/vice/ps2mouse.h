/*
 * ps2mouse.h - PS/2 mouse on userport emulation
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on code by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_PS2MOUSE_H
#define VICE_PS2MOUSE_H

#include "types.h"

extern void ps2mouse_reset(void);

extern BYTE ps2mouse_read(void);
extern void ps2mouse_store(BYTE value);

extern int ps2mouse_resources_init(void);
extern int ps2mouse_cmdline_options_init(void);

extern int ps2mouse_enabled;

extern int mouse_resources_init(void);
extern int mouse_cmdline_options_init(void);
extern void mouse_init(void);
extern void mouse_shutdown(void);

extern void mouse_button_left(int pressed);
extern void mouse_button_right(int pressed);

extern BYTE mouse_get_x(void);
extern BYTE mouse_get_y(void);

extern int _mouse_enabled;

#endif
