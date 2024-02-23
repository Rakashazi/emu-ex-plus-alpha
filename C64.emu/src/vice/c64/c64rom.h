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

/* filenames used for the BASIC ROMs */
#define C64_BASIC_NAME      "basic-901226-01.bin"

/* unique IDs to refer to a kernal, used for the KernalRev resource */
#define C64_KERNAL_UNKNOWN -1

#define C64_KERNAL_JAP      0       /* 906145-02 */
#define C64_KERNAL_REV1     1       /* 901227-01 */
#define C64_KERNAL_REV2     2       /* 901227-02 */
#define C64_KERNAL_REV3     3       /* 901227-03 */
#define C64_KERNAL_GS64     39      /* 390852-01 */
#define C64_KERNAL_SX64     67      /* 251104-04 */
#define C64_KERNAL_4064     100     /* 901246-01 */

#define C64_KERNAL_REV3SWE  13      /* FIXME */

#define C64_KERNAL_NONE     -2  /* used for the MAX machine, which has no kernal */

/* filenames used for the kernal ROMs */
#define C64_KERNAL_JAP_NAME     "kernal-906145-02.bin"
#define C64_KERNAL_REV1_NAME    "kernal-901227-01.bin"
#define C64_KERNAL_REV2_NAME    "kernal-901227-02.bin"
#define C64_KERNAL_REV3_NAME    "kernal-901227-03.bin"
#define C64_KERNAL_GS64_NAME    "kernal-390852-01.bin"
#define C64_KERNAL_SX64_NAME    "kernal-251104-04.bin"
#define C64_KERNAL_4064_NAME    "kernal-901246-01.bin"

#define C64_KERNAL_NONE_NAME    "kernal-none.bin"    /* dummy, used for the MAX machine, which has no kernal */

/* filenames used for the chargen ROMs */
#define C64_CHARGEN_NAME        "chargen-901225-01.bin"
#define C64_CHARGEN_JAP_NAME    "chargen-906143-02.bin"

/* simple additive checksum */
#define C64_BASIC_CHECKSUM         15702

#define C64_KERNAL_CHECKSUM_JAP    53635       /* 906145-02 */
#define C64_KERNAL_CHECKSUM_R01    54525       /* 901227-01 */
#define C64_KERNAL_CHECKSUM_R02    50955       /* 901227-02 */
#define C64_KERNAL_CHECKSUM_R03    50954       /* 901227-03 */
#define C64_KERNAL_CHECKSUM_GS64   46538       /* 390852-01 (gs) */
#define C64_KERNAL_CHECKSUM_R43    50955       /* 251104-04 (sx) */
#define C64_KERNAL_CHECKSUM_R64    49680       /* 901246-01 (educator) */

#define C64_KERNAL_CHECKSUM_R03swe 50633       /* FIXME */

/* the value located at 0xff80 */
#define C64_KERNAL_ID_JAP    0x00       /* 906145-02 */
#define C64_KERNAL_ID_R01    0xaa       /* 901227-01 */
#define C64_KERNAL_ID_R02    0x00       /* 901227-02 */
#define C64_KERNAL_ID_R03    0x03       /* 901227-03 */
#define C64_KERNAL_ID_GS64   0x03       /* 390852-01 (gs) */
#define C64_KERNAL_ID_R43    0x43       /* 251104-04 (sx) */
#define C64_KERNAL_ID_R64    0x64       /* 901246-01 (educator) */

#define C64_KERNAL_ID_R03swe 0x03       /* FIXME */

/* SHA1 hashes */
#define C64_BASIC_HASH         "79015323128650c742a3694c9429aa91f355905e"

#define C64_KERNAL_HASH_JAP    "4ff0f11e80f4b57430d8f0c3799ed0f0e0f4565d"       /* 906145-02 */
#define C64_KERNAL_HASH_R01    "87cc04d61fc748b82df09856847bb5c2754a2033"       /* 901227-01 */
#define C64_KERNAL_HASH_R02    "0e2e4ee3f2d41f00bed72f9ab588b83e306fdb13"       /* 901227-02 */
#define C64_KERNAL_HASH_R03    "1d503e56df85a62fee696e7618dc5b4e781df1bb"       /* 901227-03 */
#define C64_KERNAL_HASH_GS64   "3ad6cc1837c679a11f551ad1cf1a32dd84ace719"       /* 390852-01 (gs) */
#define C64_KERNAL_HASH_R43    "aa136e91ecf3c5ac64f696b3dbcbfc5ba0871c98"       /* 251104-04 (sx) */
#define C64_KERNAL_HASH_R64    "6c4fa9465f6091b174df27dfe679499df447503c"       /* 901246-01 (educator) */

int c64rom_load_kernal(const char *rom_name, uint8_t *new_kernal);
int c64rom_load_basic(const char *rom_name);
int c64rom_load_chargen(const char *rom_name);

int c64rom_get_kernal_chksum_id(uint16_t *sumout, int *idout, char *sha1hash);

/* print ROM ID, checksum, sha1 hash. returns the revision */
int c64rom_print_kernal_info(void);
int c64rom_print_basic_info(void);

int c64rom_isloaded(void);

extern int c64rom_cartkernal_active;

#endif
