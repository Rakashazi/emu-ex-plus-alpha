/*
 * renderscale2x.c - scale2x renderers
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
 * Original Scale2x code by
 *  Andrea Mazzoleni <amadvance@users.sourceforge.net>
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

#include "renderscale2x.h"
#include "types.h"

static uint32_t scale2x(const uint32_t *colortab, const uint8_t **srcx1,
                     const uint8_t **srcx2, const uint8_t **srcy1,
                     const uint8_t **srcy2, const uint8_t **srce)
{
    uint32_t colx1, coly1, cole;

    colx1 = **srcx1;
    coly1 = **srcy1;
    cole = colortab[colx1 == coly1 && **srcx2 != coly1 && colx1 != **srcy2 ? colx1 : **srce];

    if (*srcx1 < *srcx2) {
        *srcx1 += 2;
        *srcx2 -= 2;
    } else {
        *srcx2 = *srcx1 + 1;
        (*srcx1)--;
        (*srcy1)++;
        (*srcy2)++;
        (*srce)++;
    }

    return cole;
}

void render_32_scale2x(const video_render_color_tables_t *color_tab,
                       const uint8_t *src, uint8_t *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    const uint8_t *srcx1, *srcx2, *srcy1, *srcy2;
    unsigned int x, y, yys;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (uint32_t *)trg;
        srcx1 = (xt & 1 ? tmpsrc + 1 : tmpsrc - 1);
        srcx2 = (xt & 1 ? tmpsrc - 1 : tmpsrc + 1);
        srcy1 = (y & 1 ? tmpsrc + pitchs : tmpsrc - pitchs);
        srcy2 = (y & 1 ? tmpsrc - pitchs : tmpsrc + pitchs);

        for (x = 0; x < width; x++) {
            *tmptrg++ = scale2x(colortab, &srcx1, &srcx2, &srcy1, &srcy2, &tmpsrc);
        }

        if (y & 1) {
            src += pitchs;
        }

        trg += pitcht;
    }
}
