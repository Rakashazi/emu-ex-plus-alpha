/*
 * raster-cache-fill-1fff.h - Raster line cache.
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

#ifndef VICE_RASTER_CACHE_FILL_1FFF_H
#define VICE_RASTER_CACHE_FILL_1FFF_H

#include <string.h>

#include "types.h"

inline static int raster_cache_data_fill_1fff(BYTE *dest,
                                              const BYTE *src_base_low,
                                              const BYTE *src_base_high,
                                              int src_cnt,
                                              const unsigned int length,
                                              unsigned int *xs,
                                              unsigned int *xe,
                                              int no_check)
{
    unsigned int i = 0;
    const BYTE *src;

    if (no_check) {
        *xs = 0;
        *xe = length - 1;

        src = (src_cnt & 0x1000) ? src_base_high : src_base_low;
        src_cnt &= 0xfff;
        if (src_cnt + length * 8 >= 0x1000) {
            for (; src_cnt < 0x1000; i++, src_cnt += 8) {
                dest[i] = src[src_cnt];
            }
            src = (src == src_base_low) ? src_base_high : src_base_low;
            src_cnt &= 0xfff;
        }
        for (; i < length; i++, src_cnt += 8) {
            dest[i] = src[src_cnt];
        }
        return 1;
    } else {
        unsigned int x = 0, i = 0;

        src = (src_cnt & 0x1000) ? src_base_high : src_base_low;
        src_cnt &= 0xfff;
        if (src_cnt + length * 8 >= 0x1000) {
            for (; src_cnt < 0x1000; i++, src_cnt += 8) {
                if (dest[i] != src[src_cnt]) {
                    if (*xs > i) {
                        *xs = i;
                    }
                    for (; src_cnt < 0x1000; i++, src_cnt += 8) {
                        if (dest[i] != src[src_cnt]) {
                            dest[i] = src[src_cnt];
                            x = i;
                        }
                    }
                    src = (src == src_base_low) ? src_base_high : src_base_low;
                    src_cnt &= 0xfff;
                    for (; i < length; i++, src_cnt += 8) {
                        if (dest[i] != src[src_cnt]) {
                            dest[i] = src[src_cnt];
                            x = i;
                        }
                    }
                    if (*xe < x) {
                        *xe = x;
                    }
                    return 1;
                }
            }
            src = (src == src_base_low) ? src_base_high : src_base_low;
            src_cnt &= 0xfff;
        }

        for (; i < length; i++, src_cnt += 8) {
            if (dest[i] != src[src_cnt]) {
                if (*xs > i) {
                    *xs = i;
                }
                for (; i < length; i++, src_cnt += 8) {
                    if (dest[i] != src[src_cnt]) {
                        dest[i] = src[src_cnt];
                        x = i;
                    }
                }
                if (*xe < x) {
                    *xe = x;
                }
                return 1;
            }
        }
    }
    return 0;
}

#endif
