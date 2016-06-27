/*
 * petrom.h
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#ifndef VICE_PETROM_H
#define VICE_PETROM_H

#include "types.h"

extern int petrom_load_chargen(void);
extern int petrom_load_kernal(void);
extern int petrom_load_basic(void);
extern int petrom_load_editor(void);
extern int petrom_load_rom9(void);
extern int petrom_load_romA(void);
extern int petrom_load_romB(void);
extern int petrom_load_6809rom(int num);

extern void petrom_convert_chargen_2k(void);
extern void petrom_convert_chargen(BYTE *charrom);
extern void petrom_get_kernal_checksum(void);
extern void petrom_get_editor_checksum(void);
extern void petrom_checksum(void);

extern void petrom_patch_2001(void);
extern void petrom_unpatch_2001(void);

extern int petrom_9_loaded;
extern int petrom_A_loaded;
extern int petrom_B_loaded;

#endif
