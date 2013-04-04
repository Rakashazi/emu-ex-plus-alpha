/*
 * raster-cache-text-ext.h - Raster line cache.
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

#ifndef VICE_RASTER_CACHE_TEXT_EXT_H
#define VICE_RASTER_CACHE_TEXT_EXT_H

#include <string.h>

#include "types.h"

inline static int raster_cache_data_fill_text_ext(BYTE *dest,
                                                  BYTE *destbg,
                                                  const BYTE *src,
                                                  BYTE *char_mem,
                                                  const int bytes_per_char,
                                                  const unsigned int length,
                                                  const int l,
                                                  unsigned int *xs,
                                                  unsigned int *xe,
                                                  int no_check)
{
#define GET_CHAR_DATA(c, l) char_mem[(((c) & 0x3f) * bytes_per_char) + (l)]
    if (no_check) {
        unsigned int i;

        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++, src++) {
            dest[i] = GET_CHAR_DATA(src[0], l);
            destbg[i] = src[0] >> 6;
        }
        return 1;
    } else {
        BYTE b;
        unsigned int i;

        for (i = 0;
             i < length && dest[i] == GET_CHAR_DATA(src[0], l)
             && destbg[i] == src[0] >> 6;
             i++, src++) {
            /* do nothing */
        }

        if (i < length) {
            *xs = *xe = i;

            for (; i < length; i++, src++) {
                if (dest[i] != (b = GET_CHAR_DATA(src[0], l))
                    || destbg[i] != src[0] >> 6) {
                    dest[i] = b;
                    destbg[i] = src[0] >> 6;
                    *xe = i;
                }
            }

            return 1;
        } else {
            return 0;
        }
    }
#undef GET_CHAR_DATA
}

#endif
