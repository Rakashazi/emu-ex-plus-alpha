/*
 * embedded.c - Code for embedding data files.
 *
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

#include "driverom.h"
#include "embedded.h"

#define NL10_ROM_SIZE      0x8000

#include "drivedos1540.h"
#include "drivedos1541.h"
#include "drived1541II.h"

static embedded_t commonfiles[] = {
    { "mps803", 512 * 7, 512 * 7, 512 * 7, NULL },
    { "nl10-cbm", NL10_ROM_SIZE, NL10_ROM_SIZE, NL10_ROM_SIZE, NULL },
    { "dos1540", DRIVE_ROM1540_SIZE, DRIVE_ROM1540_SIZE_EXPANDED, DRIVE_ROM1540_SIZE, drive_rom1540_embedded },
    { "dos1541", DRIVE_ROM1541_SIZE, DRIVE_ROM1541_SIZE_EXPANDED, DRIVE_ROM1541_SIZE, drive_rom1541_embedded },
    { "d1541II", DRIVE_ROM1541II_SIZE, DRIVE_ROM1541II_SIZE_EXPANDED, DRIVE_ROM1541II_SIZE, drive_rom1541ii_embedded },
    { "dos1001", DRIVE_ROM1001_SIZE, DRIVE_ROM1001_SIZE, DRIVE_ROM1001_SIZE, NULL },
    { "dos1570", DRIVE_ROM1570_SIZE, DRIVE_ROM1570_SIZE, DRIVE_ROM1570_SIZE, NULL },
    { "dos1571", DRIVE_ROM1571_SIZE, DRIVE_ROM1571_SIZE, DRIVE_ROM1571_SIZE, NULL },
    { "dos1581", DRIVE_ROM1581_SIZE, DRIVE_ROM1581_SIZE, DRIVE_ROM1581_SIZE, NULL },
    { "dos2000", DRIVE_ROM2000_SIZE, DRIVE_ROM2000_SIZE, DRIVE_ROM2000_SIZE, NULL },
    { "dos4000", DRIVE_ROM4000_SIZE, DRIVE_ROM4000_SIZE, DRIVE_ROM4000_SIZE, NULL },
    { "dos2031", DRIVE_ROM2031_SIZE, DRIVE_ROM2031_SIZE, DRIVE_ROM2031_SIZE, NULL },
    { "dos2040", DRIVE_ROM2040_SIZE, DRIVE_ROM2040_SIZE, DRIVE_ROM2040_SIZE, NULL },
    { "dos3040", DRIVE_ROM3040_SIZE, DRIVE_ROM3040_SIZE, DRIVE_ROM3040_SIZE, NULL },
    { "dos4040", DRIVE_ROM4040_SIZE, DRIVE_ROM4040_SIZE, DRIVE_ROM4040_SIZE, NULL },
    { "dos1551", DRIVE_ROM1551_SIZE, DRIVE_ROM1551_SIZE, DRIVE_ROM1551_SIZE, NULL },
    { "d1571cr", DRIVE_ROM1571CR_SIZE, DRIVE_ROM1571CR_SIZE, DRIVE_ROM1571CR_SIZE, NULL },
    { NULL }
};

static size_t embedded_match_file(const char *name, BYTE *dest, int minsize, int maxsize, embedded_t *emb)
{
    int i = 0;
    int load_at_start;

    if (minsize < 0) {
        minsize = -minsize;
        load_at_start = 1;
    } else {
        load_at_start = 0;
    }

    while (emb[i].name != NULL) {
        if (!strcmp(name, emb[i].name) && minsize == emb[i].minsize && maxsize == emb[i].maxsize) {
            if (emb[i].esrc != NULL) {
                if (emb[i].size != minsize || load_at_start) {
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

size_t embedded_check_extra(const char *name, BYTE *dest, int minsize, int maxsize)
{
    size_t retval;

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, commonfiles)) != 0) {
        return retval;
    }
    return 0;
}
#endif
