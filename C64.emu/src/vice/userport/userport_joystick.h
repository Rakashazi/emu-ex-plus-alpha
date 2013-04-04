/*
 * userport_joystick.h: Generic userport joystick device emulation.
 *
 * Written by
 *  Marco van den Heuvel <viceteam@t-online.de>
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

#ifndef VICE_USERPORT_JOYSTICK_H
#define VICE_USERPORT_JOYSTICK_H

#include "types.h"

#define USERPORT_JOYSTICK_CGA      0
#define USERPORT_JOYSTICK_PET      1
#define USERPORT_JOYSTICK_HUMMER   2
#define USERPORT_JOYSTICK_OEM      3
#define USERPORT_JOYSTICK_HIT      4
#define USERPORT_JOYSTICK_KINGSOFT 5
#define USERPORT_JOYSTICK_STARBYTE 6

#define USERPORT_JOYSTICK_NUM 7

extern int userport_joystick_enable;
extern int userport_joystick_type;

extern int userport_joystick_resources_init(void);
extern int userport_joystick_cmdline_options_init(void);

extern void userport_joystick_store_pa2(BYTE value);
extern void userport_joystick_store_pbx(BYTE value);
extern void userport_joystick_store_sdr(BYTE value);

extern BYTE userport_joystick_read_pa2(BYTE orig);
extern BYTE userport_joystick_read_pbx(BYTE orig);
extern BYTE userport_joystick_read_sdr(BYTE orig);

#endif
