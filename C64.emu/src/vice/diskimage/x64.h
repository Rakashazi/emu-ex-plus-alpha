/*
 * x64.h - x64 related constants.
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

#ifndef VICE_X64_H
#define VICE_X64_H

#define X64_HEADER_MAGIC_OFFSET     0       /* Length 4 bytes */
#define X64_HEADER_MAGIC_1          'C'
#define X64_HEADER_MAGIC_2          (0x15)
#define X64_HEADER_MAGIC_3          (0x41)
#define X64_HEADER_MAGIC_4          (0x64)
#define X64_HEADER_VERSION_OFFSET   4       /* Length 2 bytes */
#define X64_HEADER_VERSION_MAJOR    1
#define X64_HEADER_VERSION_MINOR    2
#define X64_HEADER_FLAGS_OFFSET     6       /* Disk Image Flags */
#define X64_HEADER_FLAGS_LEN        4       /* Disk Image Flags */
#define X64_HEADER_LABEL_OFFSET     32      /* Disk Description */
#define X64_HEADER_LABEL_LEN        31
#define X64_HEADER_LENGTH           64

#endif
