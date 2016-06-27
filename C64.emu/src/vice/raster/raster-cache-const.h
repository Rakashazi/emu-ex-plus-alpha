/*
 * raster-cache-const.h - Raster line cache.
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

#ifndef VICE_RASTER_CACHE_CONST_H
#define VICE_RASTER_CACHE_CONST_H

#include <string.h>

#include "types.h"

inline static int raster_cache_data_fill_const(BYTE *dest,
                                               const BYTE data,
                                               const unsigned int length,
                                               unsigned int *xs,
                                               unsigned int *xe,
                                               int no_check)
{
    if (no_check) {
        *xs = 0;
        *xe = length - 1;
        memset(dest, data, (size_t)length);
        return 1;
    } else {
        unsigned int x = 0, i;

        for (i = 0; i < length && dest[i] == data; i++) {
            /* do nothing */
        }

        if (i < length) {
            if (*xs > i) {
                *xs = i;
            }

            for (; i < length; i++) {
                if (dest[i] != data) {
                    dest[i] = data;
                    x = i;
                }
            }

            if (*xe < x) {
                *xe = x;
            }

            return 1;
        } else {
            return 0;
        }
    }
}

#endif
