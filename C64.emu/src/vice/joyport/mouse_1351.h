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

int mouse_1351_register(void);
int mouse_micromys_register(void);
int mouse_smartmouse_register(void);

/* commodore 1351 */

void mouse_1351_button_left(int pressed);
void mouse_1351_button_right(int pressed);

/* micromys */

uint8_t micromys_mouse_read(void);
void micromys_mouse_button_middle(int pressed);
void micromys_mouse_button_up(int pressed);
void micromys_mouse_button_down(int pressed);

/* smartmouse */

void smart_mouse_store(int port, uint8_t val);
uint8_t smart_mouse_read(void);
int smart_mouse_resources_init(void);
int smart_mouse_cmdline_options_init(void);
void smart_mouse_shutdown(void);

#endif
