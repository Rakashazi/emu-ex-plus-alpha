/*
 * clockport_mp3at64.h - ClockPort MP3@64 emulation.
 *
 * Written by
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

#ifndef VICE_CLOCKPORT_MP3AT64_H
#define VICE_CLOCKPORT_MP3AT64_H

#include "clockport.h"

extern int clockport_mp3at64_init(void);
extern void clockport_mp3at64_shutdown(void);
extern clockport_device_t *clockport_mp3at64_open_device(char *owner);

extern void clockport_mp3at64_sound_chip_init(void);

#endif
