/*
 * vic20rom.h -- VIC20 memory handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Memory configuration handling by
 *  Alexander Lehmann <alex@mathematik.th-darmstadt.de>
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

#ifndef VICE_VIC20ROM_H
#define VICE_VIC20ROM_H

int vic20rom_load_kernal(const char *rom_name);
int vic20rom_load_basic(const char *rom_name);
int vic20rom_load_chargen(const char *rom_name);

int vic20rom_kernal_checksum(void);
int vic20rom_basic_checksum(void);

#define VIC20_BASIC_ROM_SIZE    0x2000
#define VIC20_BASIC_CHECKSUM    33073

#define VIC20_BASIC_NAME        "basic-901486-01.bin"

#define VIC20_KERNAL_ROM_SIZE   0x2000

#define VIC20_KERNAL_REV2_CHECKSUM  27238   /* 901486-02 */
#define VIC20_KERNAL_REV6_CHECKSUM  38203   /* 901486-06 */
#define VIC20_KERNAL_REV7_CHECKSUM  38203   /* 901486-07 */

#define VIC20_KERNAL_REV2_NAME  "kernal.901486-02.bin"  /* japanese NTSC machines */
#define VIC20_KERNAL_REV6_NAME  "kernal.901486-06.bin"  /* NTSC machines */
#define VIC20_KERNAL_REV7_NAME  "kernal.901486-07.bin"  /* PAL machines */

#define VIC20_CHARGEN_ROM_SIZE  0x1000
#define VIC20_CHARGEN_NAME      "chargen-901460-03.bin"
#define VIC20_CHARGEN_JAP_NAME  "chargen-901460-02.bin"  /* japanese NTSC machines */

#endif
