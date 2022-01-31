/*
 * render-common.c - inlined functions common to (some) renderers
 *
 * Written by
 *  John Selck <graham@cruise.de>
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

#include "types.h"
#include "video.h"

#ifndef VICE_RENDER_COMMON_H
#error Do not compile this file directly, just include render-common.h
#endif

static inline void render_source_line(uint32_t *tmptrg, const uint8_t *tmpsrc, const uint32_t *colortab,
                                      unsigned int wstart, unsigned int wfast, unsigned int wend)
{
    unsigned int x;
    
    for (x = 0; x < wstart; x++) {
        *tmptrg++ = colortab[*tmpsrc++];
    }
    for (x = 0; x < wfast; x++) {
        tmptrg[0] = colortab[tmpsrc[0]];
        tmptrg[1] = colortab[tmpsrc[1]];
        tmptrg[2] = colortab[tmpsrc[2]];
        tmptrg[3] = colortab[tmpsrc[3]];
        tmptrg[4] = colortab[tmpsrc[4]];
        tmptrg[5] = colortab[tmpsrc[5]];
        tmptrg[6] = colortab[tmpsrc[6]];
        tmptrg[7] = colortab[tmpsrc[7]];
        tmpsrc += 8;
        tmptrg += 8;
    }
    for (x = 0; x < wend; x++) {
        *tmptrg++ = colortab[*tmpsrc++];
    }
}

static inline void render_source_line_2x(uint32_t *tmptrg, const uint8_t *tmpsrc, const uint32_t *colortab,
                                         unsigned int wstart, unsigned int wfast, unsigned int wend,
                                         unsigned int wfirst, unsigned int wlast)
{
    unsigned int x;
    uint32_t color;
    
    if (wfirst) {
        *tmptrg++ = colortab[*tmpsrc++];
    }
    for (x = 0; x < wstart; x++) {
        color = colortab[*tmpsrc++];
        *tmptrg++ = color;
        *tmptrg++ = color;
    }
    for (x = 0; x < wfast; x++) {
        color = colortab[tmpsrc[0]];
        tmptrg[0] = color;
        tmptrg[1] = color;
        color = colortab[tmpsrc[1]];
        tmptrg[2] = color;
        tmptrg[3] = color;
        color = colortab[tmpsrc[2]];
        tmptrg[4] = color;
        tmptrg[5] = color;
        color = colortab[tmpsrc[3]];
        tmptrg[6] = color;
        tmptrg[7] = color;
        color = colortab[tmpsrc[4]];
        tmptrg[8] = color;
        tmptrg[9] = color;
        color = colortab[tmpsrc[5]];
        tmptrg[10] = color;
        tmptrg[11] = color;
        color = colortab[tmpsrc[6]];
        tmptrg[12] = color;
        tmptrg[13] = color;
        color = colortab[tmpsrc[7]];
        tmptrg[14] = color;
        tmptrg[15] = color;
        tmpsrc += 8;
        tmptrg += 16;
    }
    for (x = 0; x < wend; x++) {
        color = colortab[*tmpsrc++];
        *tmptrg++ = color;
        *tmptrg++ = color;
    }
    if (wlast) {
        *tmptrg = colortab[*tmpsrc];
    }
}

static inline void render_solid_line(uint32_t *tmptrg, const uint8_t *tmpsrc, const uint32_t color,
                                     unsigned int wstart, unsigned int wfast, unsigned int wend)
{
    unsigned int x;
    
    for (x = 0; x < wstart; x++) {
        *tmptrg++ = color;
    }
    for (x = 0; x < wfast; x++) {
        tmptrg[0] = color;
        tmptrg[1] = color;
        tmptrg[2] = color;
        tmptrg[3] = color;
        tmptrg[4] = color;
        tmptrg[5] = color;
        tmptrg[6] = color;
        tmptrg[7] = color;
        tmptrg += 8;
    }
    for (x = 0; x < wend; x++) {
        *tmptrg++ = color;
    }
}

static inline void render_solid_line_2x(uint32_t *tmptrg, const uint8_t *tmpsrc, const uint32_t color,
                                        unsigned int wstart, unsigned int wfast, unsigned int wend,
                                        unsigned int wfirst, unsigned int wlast)
{
    unsigned int x;
    
    if (wfirst) {
        *tmptrg++ = color;
    }
    for (x = 0; x < wstart; x++) {
        *tmptrg++ = color;
        *tmptrg++ = color;
    }
    for (x = 0; x < wfast; x++) {
        tmptrg[0] = color;
        tmptrg[1] = color;
        tmptrg[2] = color;
        tmptrg[3] = color;
        tmptrg[4] = color;
        tmptrg[5] = color;
        tmptrg[6] = color;
        tmptrg[7] = color;
        tmptrg[8] = color;
        tmptrg[9] = color;
        tmptrg[10] = color;
        tmptrg[11] = color;
        tmptrg[12] = color;
        tmptrg[13] = color;
        tmptrg[14] = color;
        tmptrg[15] = color;
        tmptrg += 16;
    }
    for (x = 0; x < wend; x++) {
        *tmptrg++ = color;
        *tmptrg++ = color;
    }
    if (wlast) {
        *tmptrg = color;
    }
}
