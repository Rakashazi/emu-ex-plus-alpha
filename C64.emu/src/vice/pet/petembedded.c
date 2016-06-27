/*
 * petembedded.c - Code for embedding pet data files.
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

#include "embedded.h"
#include "machine.h"

#include "petbasic1.h"
#include "petbasic2.h"
#include "petbasic4.h"
#include "petchargen.h"
#include "petedit1g.h"
#include "petedit2b.h"
#include "petedit2g.h"
#include "petedit4b40.h"
#include "petedit4b80.h"
#include "petedit4g40.h"
#include "petkernal1.h"
#include "petkernal2.h"
#include "petkernal4.h"
#include "superpet_char.h"
#include "superpet_waterloo_a000.h"
#include "superpet_waterloo_b000.h"
#include "superpet_waterloo_c000.h"
#include "superpet_waterloo_d000.h"
#include "superpet_waterloo_e000.h"
#include "superpet_waterloo_f000.h"

#include "pet_amber_vpl.h"
#include "pet_green_vpl.h"
#include "pet_white_vpl.h"

static embedded_t petfiles[] = {
    { "chargen", 0x800, 0x800, 0x800, petchargen_embedded },
    { "basic4", 0x2000, 0x3000, 0x3000, petbasic4_embedded },
    { "kernal4", 0x1000, 0x1000, 0x1000, petkernal4_embedded },
    { "edit4b80", 0x800, 0x1000, 0x800, petedit4b80_embedded },
    { "kernal1", 0x1000, 0x1000, 0x1000, petkernal1_embedded },
    { "basic1", 0x2000, 0x3000, 0x2000, petbasic1_embedded },
    { "basic2", 0x2000, 0x3000, 0x2000, petbasic2_embedded },
    { "kernal2", 0x1000, 0x1000, 0x1000, petkernal2_embedded },
    { "edit1g", 0x800, 0x1000, 0x800, petedit1g_embedded },
    { "edit2b", 0x800, 0x1000, 0x800, petedit2b_embedded },
    { "edit2g", 0x800, 0x1000, 0x800, petedit2g_embedded },
    { "edit4b40", 0x800, 0x1000, 0x800, petedit4b40_embedded },
    { "edit4g40", 0x800, 0x1000, 0x800, petedit4g40_embedded },
    { "characters.901640-01.bin", 0x1000, 0x1000, 0x1000, superpet_char_embedded },
    { "waterloo-a000.901898-01.bin", 0x1000, 0x1000, 0x1000, superpet_waterloo_a000_embedded },
    { "waterloo-b000.901898-02.bin", 0x1000, 0x1000, 0x1000, superpet_waterloo_b000_embedded },
    { "waterloo-c000.901898-03.bin", 0x1000, 0x1000, 0x1000, superpet_waterloo_c000_embedded },
    { "waterloo-d000.901898-04.bin", 0x1000, 0x1000, 0x1000, superpet_waterloo_d000_embedded },
    { "waterloo-e000.901897-01.bin", 0x800, 0x800, 0x800, superpet_waterloo_e000_embedded },
    { "waterloo-f000.901898-05.bin", 0x1000, 0x1000, 0x1000, superpet_waterloo_f000_embedded },
    { NULL }
};

static embedded_palette_t palette_files[] = {
    { "amber", "amber.vpl", 2, pet_amber_vpl },
    { "green", "green.vpl", 2, pet_green_vpl },
    { "white", "white.vpl", 2, pet_white_vpl },
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

    if ((retval = embedded_match_file(name, dest, minsize, maxsize, petfiles)) != 0) {
        return retval;
    }
    return 0;
}

int embedded_palette_load(const char *fname, palette_t *p)
{
    int i = 0;
    int j;
    unsigned char *entries;

    while (palette_files[i].name1 != NULL) {
        if (!strcmp(palette_files[i].name1, fname) || !strcmp(palette_files[i].name2, fname)) {
            entries = palette_files[i].palette;
            for (j = 0; j < palette_files[i].num_entries; j++) {
                p->entries[j].red = entries[(j * 4) + 0];
                p->entries[j].green = entries[(j * 4) + 1];
                p->entries[j].blue = entries[(j * 4) + 2];
                p->entries[j].dither = entries[(j * 4) + 3];
            }
            return 0;
        }
        i++;
    }
    return -1;
}
#endif
