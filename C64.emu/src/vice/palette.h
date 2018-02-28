/*
 * palette.h - Palette handling.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_PALETTE_H
#define VICE_PALETTE_H

#include "types.h"

typedef struct palette_entry_s {
    char *name;
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE dither;
} palette_entry_t;

typedef struct palette_s {
    unsigned int num_entries;
    palette_entry_t *entries;
} palette_t;

extern void palette_init(void);
extern palette_t *palette_create(unsigned int num_entries, const char *entry_names[]);
extern void palette_free(palette_t *p);
extern int palette_load(const char *file_name, palette_t *palette_return);
extern int palette_save(const char *file_name, const palette_t *palette);

/* palette info for GUIs */
typedef struct {
    char *chip; /* chip this palette belongs to */
    char *name; /* name to be used in menus */
    char *file; /* filename of the palette file */
} palette_info_t;

/* returns pointer to palette_info_t entries. may return an empty list. */
extern palette_info_t *palette_get_info_list(void);

#endif
