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

int c128rom_load_kernal_int(const char *rom_name);
int c128rom_load_kernal_ch(const char *rom_name);
int c128rom_load_kernal_de(const char *rom_name);
int c128rom_load_kernal_fi(const char *rom_name);
int c128rom_load_kernal_fr(const char *rom_name);
int c128rom_load_kernal_it(const char *rom_name);
int c128rom_load_kernal_no(const char *rom_name);
int c128rom_load_kernal_se(const char *rom_name);

int c128rom_load_basiclo(const char *rom_name);
int c128rom_load_basichi(const char *rom_name);

int c128rom_load_chargen_int(const char *rom_name);
int c128rom_load_chargen_ch(const char *rom_name);
int c128rom_load_chargen_de(const char *rom_name);
int c128rom_load_chargen_fi(const char *rom_name);
int c128rom_load_chargen_fr(const char *rom_name);
int c128rom_load_chargen_it(const char *rom_name);
int c128rom_load_chargen_no(const char *rom_name);
int c128rom_load_chargen_se(const char *rom_name);

int c128rom_load_kernal64(const char *rom_name, uint8_t *cartkernal);
int c128rom_load_basic64(const char *rom_name);

int c128rom_basic_checksum(void);
int c128rom_kernal_checksum(void);
int c128rom_kernal_setup(void);
int c128rom_chargen_setup(void);

/* BASICLO + BASICHI */
#define C128_BASIC_85_CHECKSUM      38592
#define C128_BASIC_86_CHECKSUM      2496

/* 0x0000 - 0x0fff in kernal image */
#define C128_EDITOR_R01_CHECKSUM        56682
#define C128_EDITOR_SE_R01_CHECKSUM     9364
#define C128_EDITOR_DE_R01_CHECKSUM     9619

/* 0x2000 - 0x3fff in kernal image */
#define C128_KERNAL_R01_CHECKSUM        22353
#define C128_KERNAL_SE_R01_CHECKSUM     24139 /* FIXME: 23086 ? */
#define C128_KERNAL_DE_R01_CHECKSUM     22098 /* FIXME: 19680 ? */
#define C128_KERNAL_CH_R01_CHECKSUM     21376

/* C128 chargen */
#define C128_CHARGEN_NAME       "chargen-390059-01.bin"
#define C128_CHARGEN_BE_NAME    "chargen-325167-02.bin" /* italian/french/belgium */
#define C128_CHARGEN_CH_NAME    "chargen-325173-01D.bin"
#define C128_CHARGEN_DE_NAME    "chargen-315079-01.bin"
#define C128_CHARGEN_FI_NAME    "chargen-325181-01.bin" /* same as swedish */
/*#define C128_CHARGEN_FR_NAME    "chargen-325167-01.bin"*/
#define C128_CHARGEN_FR_NAME    "chargen-325167-02.bin" /* italian/french/belgium */
#define C128_CHARGEN_IT_NAME    "chargen-325167-02.bin" /* italian/french/belgium */
#define C128_CHARGEN_NO_NAME    "chargen-325078-02.bin"
#define C128_CHARGEN_SE_NAME    "chargen-325181-01.bin"

/* C128 Kernal (Editor + Z80BIOS + Kernal) */
#define C128_KERNAL_NAME        "kernal-318020-05.bin"
#define C128_KERNAL_CH_NAME     "kernal-325172-01.bin"      /* second half in 318081-01 */
#define C128_KERNAL_DE_NAME     "kernal-315078-03.bin"
#define C128_KERNAL_FI_NAME     "kernalfi"                  /* FIXME: identify */
#define C128_KERNAL_FR_NAME     "kernalfr"                  /* FIXME: identify */
#define C128_KERNAL_IT_NAME     "kernalit"                  /* FIXME: identify */
#define C128_KERNAL_NO_NAME     "kernalno"                  /* FIXME: identify */
#define C128_KERNAL_SE_NAME     "kernal-318034-01.bin"

/* C128 BASIC */
#define C128_BASICLO_NAME       "basiclo-318018-04.bin"     /* BASIC */
#define C128_BASICHI_NAME       "basichi-318019-04.bin"     /* Editor */

/* C64 BASIC */
#define C128_BASIC64_NAME       "basic64-901226-01.bin"     /* first 8k in 318081-01 */

/* C64 Kernal */
#define C128_KERNAL64_NAME      "kernal64-901227-03.bin"    /* second 8k in 318081-01 */
#define C128_KERNAL64_DK_NAME   "kernal64-325176-07.bin"    /* second 8k OF 325176-07 */
#define C128_KERNAL64_NO_NAME   "kernal64-325179-01.bin"    /* second 8k OF 325179-01 */
#define C128_KERNAL64_SE_NAME   "kernal64-325182-01.bin"    /* second 8k OF 325182-01 */

#endif
