/*
 * c64dtvembedded.c - Code for embedding c64dtv data files.
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

#include "vice.h"

#ifdef USE_EMBEDDED
#include <string.h>
#include <stdio.h>

#include "c64mem.h"
#include "embedded.h"
#include "machine.h"

static embedded_t c64dtvfiles[] = {
    { "basic", C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE, NULL },
    { "kernal", C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE, NULL },
    { "chargen", C64_CHARGEN_ROM_SIZE, C64_CHARGEN_ROM_SIZE, C64_CHARGEN_ROM_SIZE, NULL },
    { NULL }
};

static size_t embedded_match_file(const char *name, BYTE *dest, int minsize, int maxsize, embedded_t *emb)
{
    int i = 0;

    while (emb[i].name != NULL) {
        if (!strcmp(name, emb[i].name) && minsize == emb[i].minsize && maxsize == emb[i].maxsize) {
            if (emb[i].esrc != NULL) {
                if (emb[i].size != minsize) {
                    memcpy(dest, emb[i].esrc, maxsize);
                } else {
                    memcpy(dest + maxsize - minsize, emb[i].esrc, minsize);
                }
            }
            return emb[i].size;
        }
        i++;
    }
    return 0;
}

size_t embedded_check_file(const char *name, BYTE *dest, int minsize, int maxsize)
{
    size_t retval;

    if ((retval = embedded_check_extra(name, dest, minsize, maxsize)) != 0) {
        return retval;
    }

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, c64dtvfiles)) != 0) {
        return retval;
    }
    return 0;
}

int embedded_palette_load(const char *fname, palette_t *p)
{
    return -1;
}
#endif
