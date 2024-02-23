/*
 * cbm2rom.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_CBM2ROM_H
#define VICE_CBM2ROM_H

int cbm2rom_load_chargen(const char *rom_name);
int cbm2rom_load_kernal(const char *rom_name);
int cbm2rom_load_basic(const char *rom_name);
int cbm2rom_load_cart_1(const char *rom_name);
int cbm2rom_load_cart_2(const char *rom_name);
int cbm2rom_load_cart_4(const char *rom_name);
int cbm2rom_load_cart_6(const char *rom_name);

int cbm2rom_checksum(void);

#define CBM2_CHARGEN500_NAME "chargen-901225-01.bin"
#define CBM2_CHARGEN600_NAME "chargen-901237-01.bin"
#define CBM2_CHARGEN700_NAME "chargen-901232-01.bin"

#define CBM2_BASIC128_NAME   "basic-901242+3-04a.bin"
#define CBM2_BASIC256_NAME   "basic-901240+1-03.bin"
#define CBM2_BASIC500_NAME   "basic-901235+6-02.bin"

#define CBM2_KERNAL_NAME     "kernal-901244-04a.bin"
#define CBM2_KERNAL500_NAME  "kernal-901234-02.bin"

#endif
