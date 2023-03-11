/*
 * mouse_1351.h - C1351-like mouse handling (header)
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_1351_H
#define VICE_1351_H

extern int mouse_1351_register(void);

/* commodore 1351 */

extern void mouse_1351_button_left(int pressed);
extern void mouse_1351_button_right(int pressed);

/* micromys */

extern uint8_t micromys_mouse_read(void);
extern void micromys_mouse_button_middle(int pressed);
extern void micromys_mouse_button_up(int pressed);
extern void micromys_mouse_button_down(int pressed);

/* smartmouse */

extern void smart_mouse_store(int port, uint8_t val);
extern uint8_t smart_mouse_read(void);
extern int smart_mouse_resources_init(void);
extern int smart_mouse_cmdline_options_init(void);
extern void smart_mouse_shutdown(void);

#endif
