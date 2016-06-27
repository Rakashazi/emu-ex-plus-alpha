/*
 * raster-cache-fill.h - Raster line cache.
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

#ifndef VICE_RASTER_CACHE_FILL_H
#define VICE_RASTER_CACHE_FILL_H

#include <string.h>

#include "types.h"

inline static int raster_cache_data_fill(BYTE *dest,
                                         const BYTE *src,
                                         const unsigned int length,
                                         unsigned int *xs,
                                         unsigned int *xe,
                                         int no_check)
{
    if (no_check) {
        *xs = 0;
        *xe = length - 1;
        memcpy(dest, src, (size_t)length);
        return 1;
    } else {
        unsigned int x = 0, i;

#if defined(ALLOW_UNALIGNED_ACCESS)
        for (i = 0; i < (length & ~3) && *((DWORD *)(dest + i)) == *((DWORD *)(src + i)); i += 4) {
            /* do nothing */
        }
        if (i == length) {
            return 0;
        }
        if (i < (length & ~1) && *((WORD *)(dest + i)) == *((WORD *)(src + i))) {
            i += 2;
        }
        if (i < length && dest[i] == src[i]) {
            i++;
        }
#else
        for (i = 0; i < length && dest[i] == src[i]; i++) {
            /* do nothing */
        }
#endif
        if (i < length) {
            if (*xs > i) {
                *xs = i;
            }
            for (; i < length; i++) {
                if (dest[i] != src[i]) {
                    dest[i] = src[i];
                    x = i;
                }
            }
            if (*xe < x) {
                *xe = x;
            }
            return 1;
        }
    }
    return 0;
}

#endif
