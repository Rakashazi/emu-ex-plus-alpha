/*
 * embedded.h - Code for embedding data files
 *
 * This feature is only active when --enable-embedded is given to the
 * configure script, its main use is to make developing new ports easier
 * and to allow ports for platforms which don't have a filesystem, or a
 * filesystem which is hard/impossible to load data files from.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_EMBEDDED_H
#define VICE_EMBEDDED_H

#include "vice.h"

#include "types.h"
#include "palette.h"

#ifdef USE_EMBEDDED
extern size_t embedded_check_file(const char *name, BYTE *dest, int minsize, int maxsize);
extern size_t embedded_check_extra(const char *name, BYTE *dest, int minsize, int maxsize);
extern int embedded_palette_load(const char *file_name, palette_t *palette_return);
#else
#define embedded_check_file(w, x, y, z) (0)
#define embedded_palette_load(x, y) (-1)
#endif

typedef struct embedded_s {
    char *name;
    int minsize;
    int maxsize;
    size_t size;
    BYTE *esrc;
} embedded_t;

typedef struct embedded_palette_s {
    char *name1;
    char *name2;
    int num_entries;
    unsigned char *palette;
} embedded_palette_t;


/** \brief  Proper terminator for embedded_t lists
 */
#define EMBEDDED_LIST_END { NULL, 0, 0, 0, NULL }

/** \brief  Proper terminator for embedded_palette_t lists
 */
#define EMBEDDED_PALETTE_LIST_END { NULL, NULL, 0, NULL }


#endif
