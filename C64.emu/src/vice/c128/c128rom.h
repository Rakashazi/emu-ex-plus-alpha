/*
 * c128rom.h
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

#ifndef VICE_C128ROM_H
#define VICE_C128ROM_H

extern int c128rom_load_kernal_int(const char *rom_name);
extern int c128rom_load_kernal_de(const char *rom_name);
extern int c128rom_load_kernal_fi(const char *rom_name);
extern int c128rom_load_kernal_fr(const char *rom_name);
extern int c128rom_load_kernal_it(const char *rom_name);
extern int c128rom_load_kernal_no(const char *rom_name);
extern int c128rom_load_kernal_se(const char *rom_name);
extern int c128rom_load_kernal_ch(const char *rom_name);
extern int c128rom_load_basiclo(const char *rom_name);
extern int c128rom_load_basichi(const char *rom_name);
extern int c128rom_load_chargen_int(const char *rom_name);
extern int c128rom_load_chargen_de(const char *rom_name);
extern int c128rom_load_chargen_fr(const char *rom_name);
extern int c128rom_load_chargen_se(const char *rom_name);
extern int c128rom_load_chargen_ch(const char *rom_name);
extern int c128rom_load_kernal64(const char *rom_name, BYTE *cartkernal);
extern int c128rom_load_basic64(const char *rom_name);

extern int c128rom_basic_checksum(void);
extern int c128rom_kernal_checksum(void);
extern int c128rom_kernal_setup(void);
extern int c128rom_chargen_setup(void);

#endif
