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

#define PET_KERNAL1_CHECKSUM    3236
#define PET_KERNAL2_CHECKSUM    31896
#define PET_KERNAL4_CHECKSUM    53017

#define PET_EDIT1G_CHECKSUM     51858
#define PET_EDIT2G_CHECKSUM     64959
#define PET_EDIT2B_CHECKSUM     1514
#define PET_EDIT4G40_CHECKSUM   14162   /* edit-4-40-n-50Hz.901498-01.bin */
#define PET_EDIT4B40_CHECKSUM1  27250   /* edit-4-b-noCRTC.901474-02.bin */
#define PET_EDIT4B40_CHECKSUM2  11897   /* edit-4-40-b-50Hz.ts.bin */
#define PET_EDIT4B80_CHECKSUM   21166

#define PET_CHARGEN1_NAME       "characters-1.901447-08.bin"
#define PET_CHARGEN2_NAME       "characters-2.901447-10.bin"
#define SUPERPET_CHARGEN_NAME   "characters.901640-01.bin"
#define PET_CHARGEN_DE_NAME     "chargen.de"

#define PET_KERNAL1NAME  "kernal-1.901439-04-07.bin"
#define PET_KERNAL2NAME  "kernal-2.901465-03.bin"
#define PET_KERNAL4NAME  "kernal-4.901465-22.bin"

#define PET_BASIC1NAME  "basic-1.901439-09-05-02-06.bin"
#define PET_BASIC2NAME  "basic-2.901465-01-02.bin"
#define PET_BASIC4NAME  "basic-4.901465-23-20-21.bin"

#define PET_EDITOR1G40NAME  "edit-1-n.901439-03.bin"
#define PET_EDITOR2G40NAME  "edit-2-n.901447-24.bin"
#define PET_EDITOR2B40NAME  "edit-2-b.901474-01.bin"
#define PET_EDITOR4G40NAME  "edit-4-40-n-50Hz.901498-01.bin"
#define PET_EDITOR4B80NAME  "edit-4-80-b-50Hz.901474-04_.bin"
/* #define PET_EDITOR4B40NAME  "edit-4-b-noCRTC.901474-02.bin" unused? */ /* no CRTC */
#define PET_EDITOR4B40NAME  "edit-4-40-b-50Hz.ts.bin"

/* edit-4-40-b-60Hz.ts.bin unused? */
/* edit-4-40-n-60Hz.901499-01.bin unused? */
/* edit-4-80-b-50Hz.901474-04.bin unused? */
/* edit-4-80-b-60Hz.901474-03.bin unused? */

/* hre-9000.324992-02.bin unused? (is in hre.vrs) */
/* hre-a000.324993-02.bin unused? (is in hre.vrs) */

#define SUPERPET_6809_A_NAME "waterloo-a000.901898-01.bin"
#define SUPERPET_6809_B_NAME "waterloo-b000.901898-02.bin"
#define SUPERPET_6809_C_NAME "waterloo-c000.901898-03.bin"
#define SUPERPET_6809_D_NAME "waterloo-d000.901898-04.bin"
#define SUPERPET_6809_E_NAME "waterloo-e000.901897-01.bin"
#define SUPERPET_6809_F_NAME "waterloo-f000.901898-05.bin"

int petrom_load_chargen(void);
int petrom_load_kernal(void);
int petrom_load_basic(void);
int petrom_load_editor(void);
int petrom_load_rom9(void);
int petrom_load_romA(void);
int petrom_load_romB(void);
int petrom_load_6809rom(int num);

void petrom_convert_chargen_2k(void);
void petrom_convert_chargen(uint8_t *charrom);
void petrom_get_kernal_checksum(void);
void petrom_get_editor_checksum(void);
void petrom_checksum(void);

void petrom_patch_2001(void);
void petrom_unpatch_2001(void);

extern int petrom_9_loaded;
extern int petrom_A_loaded;
extern int petrom_B_loaded;

#endif
