/*
 * render1x1.c - 1x1 renderers
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

#include "render1x1.h"
#include "types.h"


/* 16 color 1x1 renderers */

void render_08_1x1_04(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint8_t *tmptrg;
    unsigned int x, y, wstart, wfast, wend;

    src += pitchs * ys + xs;
    trg += pitcht * yt + xt;
    if (width < 8) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 8 pixels*/
        wstart = (unsigned int)(8 - (vice_ptr_to_uint(trg) & 7));
        wfast = (width - wstart) >> 3;  /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07; /* do not forget the rest*/
    }
    for (y = 0; y < height; y++) {
        tmpsrc = src;
        tmptrg = trg;
        for (x = 0; x < wstart; x++) {
            *tmptrg++ = (uint8_t)colortab[*tmpsrc++];
        }
        for (x = 0; x < wfast; x++) {
            tmptrg[0] = (uint8_t)colortab[tmpsrc[0]];
            tmptrg[1] = (uint8_t)colortab[tmpsrc[1]];
            tmptrg[2] = (uint8_t)colortab[tmpsrc[2]];
            tmptrg[3] = (uint8_t)colortab[tmpsrc[3]];
            tmptrg[4] = (uint8_t)colortab[tmpsrc[4]];
            tmptrg[5] = (uint8_t)colortab[tmpsrc[5]];
            tmptrg[6] = (uint8_t)colortab[tmpsrc[6]];
            tmptrg[7] = (uint8_t)colortab[tmpsrc[7]];
            tmpsrc += 8;
            tmptrg += 8;
        }
        for (x = 0; x < wend; x++) {
            *tmptrg++ = (uint8_t)colortab[*tmpsrc++];
        }
        src += pitchs;
        trg += pitcht;
    }
}

void render_16_1x1_04(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint16_t *tmptrg;
    unsigned int x, y, wstart, wfast, wend;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 1);
    if (width < 8) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 8 pixels*/
        wstart = (unsigned int)(8 - (vice_ptr_to_uint(trg) & 7));
        wfast = (width - wstart) >> 3;  /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07; /* do not forget the rest*/
    }
    for (y = 0; y < height; y++) {
        tmpsrc = src;
        tmptrg = (uint16_t *)trg;
        for (x = 0; x < wstart; x++) {
            *tmptrg++ = (uint16_t)colortab[*tmpsrc++];
        }
        for (x = 0; x < wfast; x++) {
            tmptrg[0] = (uint16_t)colortab[tmpsrc[0]];
            tmptrg[1] = (uint16_t)colortab[tmpsrc[1]];
            tmptrg[2] = (uint16_t)colortab[tmpsrc[2]];
            tmptrg[3] = (uint16_t)colortab[tmpsrc[3]];
            tmptrg[4] = (uint16_t)colortab[tmpsrc[4]];
            tmptrg[5] = (uint16_t)colortab[tmpsrc[5]];
            tmptrg[6] = (uint16_t)colortab[tmpsrc[6]];
            tmptrg[7] = (uint16_t)colortab[tmpsrc[7]];
            tmpsrc += 8;
            tmptrg += 8;
        }
        for (x = 0; x < wend; x++) {
            *tmptrg++ = (uint16_t)colortab[*tmpsrc++];
        }
        src += pitchs;
        trg += pitcht;
    }
}

void render_24_1x1_04(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint8_t *tmptrg;
    unsigned int x, y, wstart, wfast, wend;
    register uint32_t color;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt * 3);
    if (width < 4) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 4 pixels*/
        wstart = (unsigned int)(4 - (vice_ptr_to_uint(trg) & 3));
        wfast = (width - wstart) >> 2; /* fast loop for 4 pixel segments*/
        wend = (width - wstart) & 0x03; /* do not forget the rest*/
    }
    for (y = 0; y < height; y++) {
        tmpsrc = src;
        tmptrg = trg;
        for (x = 0; x < wstart; x++) {
            color = colortab[*tmpsrc++];
            tmptrg[0] = (uint8_t)color;
            color >>= 8;
            tmptrg[1] = (uint8_t)color;
            color >>= 8;
            tmptrg[2] = (uint8_t)color;
            tmptrg += 3;
        }
        for (x = 0; x < wfast; x++) {
            color = colortab[tmpsrc[0]];
            tmptrg[0] = (uint8_t)color;
            color >>= 8;
            tmptrg[1] = (uint8_t)color;
            color >>= 8;
            tmptrg[2] = (uint8_t)color;
            color = colortab[tmpsrc[1]];
            tmptrg[3] = (uint8_t)color;
            color >>= 8;
            tmptrg[4] = (uint8_t)color;
            color >>= 8;
            tmptrg[5] = (uint8_t)color;
            color = colortab[tmpsrc[2]];
            tmptrg[6] = (uint8_t)color;
            color >>= 8;
            tmptrg[7] = (uint8_t)color;
            color >>= 8;
            tmptrg[8] = (uint8_t)color;
            color = colortab[tmpsrc[3]];
            tmptrg[9] = (uint8_t)color;
            color >>= 8;
            tmptrg[10] = (uint8_t)color;
            color >>= 8;
            tmptrg[11] = (uint8_t)color;
            tmpsrc += 4;
            tmptrg += 12;
        }
        for (x = 0; x < wend; x++) {
            color = colortab[*tmpsrc++];
            tmptrg[0] = (uint8_t)color;
            color >>= 8;
            tmptrg[1] = (uint8_t)color;
            color >>= 8;
            tmptrg[2] = (uint8_t)color;
            tmptrg += 3;
        }
        src += pitchs;
        trg += pitcht;
    }
}

void render_32_1x1_04(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int x, y, wstart, wfast, wend;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    if (width < 8) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 8 pixels*/
        wstart = (unsigned int)(8 - (vice_ptr_to_uint(trg) & 7));
        wfast = (width - wstart) >> 3; /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07; /* do not forget the rest*/
    }
    for (y = 0; y < height; y++) {
        tmpsrc = src;
        tmptrg = (uint32_t *)trg;
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
        src += pitchs;
        trg += pitcht;
    }
}
