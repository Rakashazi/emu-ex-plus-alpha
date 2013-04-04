/*
 * c64rom.h
 *
 * Written by
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

#ifndef VICE_C64ROM_H
#define VICE_C64ROM_H

extern int c64rom_load_kernal(const char *rom_name, BYTE *new_kernal);
extern int c64rom_load_basic(const char *rom_name);
extern int c64rom_load_chargen(const char *rom_name);

extern int c64rom_get_kernal_checksum(void);
extern int c64rom_get_basic_checksum(void);

extern int c64rom_cartkernal_active;

#endif
