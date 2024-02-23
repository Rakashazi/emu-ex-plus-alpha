/*
 * plus4rom.h
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

#ifndef VICE_PLUS4ROM_H
#define VICE_PLUS4ROM_H

int plus4rom_load_kernal(const char *rom_name);
int plus4rom_load_basic(const char *rom_name);

#define PLUS4_BASIC_NAME            "basic-318006-01.bin"

#define PLUS4_KERNAL_NTSC_REV1_NAME "kernal-318004-01.bin" /* V232 prototype */
#define PLUS4_KERNAL_PAL_REV5_NAME  "kernal-318004-05.bin"
#define PLUS4_KERNAL_NTSC_REV5_NAME "kernal-318005-05.bin"
#define PLUS4_KERNAL_NTSC_364_NAME  "kernal-364.bin"       /* 364 prototype */

#define PLUS4_3PLUS1LO_NAME         "3plus1-317053-01.bin"
#define PLUS4_3PLUS1HI_NAME         "3plus1-317054-01.bin"

#define PLUS4_C2LO_NAME             "c2lo-364.bin"         /* 364 prototype */

#endif
