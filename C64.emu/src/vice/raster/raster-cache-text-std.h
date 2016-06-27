/*
 * raster-cache-text-std.h - Raster line cache.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_RASTER_CACHE_TEXT_STD_H
#define VICE_RASTER_CACHE_TEXT_STD_H

#include "types.h"

inline static int raster_cache_data_fill_text(BYTE *dest,
                                              const BYTE *src,
                                              const BYTE *char_mem,
                                              const unsigned int length,
                                              unsigned int *xs,
                                              unsigned int *xe,
                                              int no_check)
{
    if (no_check) {
        unsigned int i;

        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++) {
            dest[i] = char_mem[src[i] * 8];
        }
        return 1;
    } else {
        BYTE b;
        unsigned int i;

        for (i = 0; i < length && dest[i] == char_mem[src[i] * 8]; i++) {
            /* do nothing */
        }

        if (i < length) {
            *xs = *xe = i;

            for (; i < length; i++) {
                if (dest[i] != (b = char_mem[src[i] * 8])) {
                    dest[i] = b;
                    *xe = i;
                }
            }

            return 1;
        } else {
            return 0;
        }
    }
}

#endif
