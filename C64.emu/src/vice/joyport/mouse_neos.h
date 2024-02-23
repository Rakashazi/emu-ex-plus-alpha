/*
 * mouse_neos.h - NEOS mouse handling (header)
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

#ifndef VICE_MOUSE_NEOS_H
#define VICE_MOUSE_NEOS_H

void mouse_neos_init(void);
void mouse_neos_set_enabled(int enabled);
void mouse_neos_button_right(int pressed);
void mouse_neos_button_left(int pressed);

int mouse_neos_register(void);

void neos_mouse_set_machine_parameter(long clock_rate);

void neos_mouse_store(int port, uint8_t val);
uint8_t neos_mouse_read(void);

#endif
