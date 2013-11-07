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

static DWORD scale2x(const DWORD *colortab, const BYTE **srcx1,
                     const BYTE **srcx2, const BYTE **srcy1,
                     const BYTE **srcy2, const BYTE **srce)
{
    register DWORD colx1, colx2, coly1, coly2, cole;

    colx1 = colortab[**srcx1];
    colx2 = colortab[**srcx2];
    coly1 = colortab[**srcy1];
    coly2 = colortab[**srcy2];
    cole = colortab[**srce];

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

    return (colx1 == coly1 && colx2 != coly1 && colx1 != coly2 ? colx1 : cole);
}


void render_08_scale2x(const video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    BYTE *tmptrg;
    unsigned int x, y, yys;
    const BYTE *srcx1, *srcx2, *srcy1, *srcy2;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + xt;
    yys = (ys << 1) | (yt & 1);

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (BYTE *)trg;
        srcx1 = (xt & 1 ? tmpsrc + 1 : tmpsrc - 1);
        srcx2 = (xt & 1 ? tmpsrc - 1 : tmpsrc + 1);
        srcy1 = (y & 1 ? tmpsrc + pitchs : tmpsrc - pitchs);
        srcy2 = (y & 1 ? tmpsrc - pitchs : tmpsrc + pitchs);

        for (x = 0; x < width; x++) {
            *tmptrg++ = (BYTE)scale2x(colortab, &srcx1, &srcx2, &srcy1, &srcy2, &tmpsrc);
        }

        if (y & 1) {
            src += pitchs;
        }

        trg += pitcht;
    }
}


void render_16_scale2x(const video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    WORD *tmptrg;
    unsigned int x, y, yys;
    const BYTE *srcx1, *srcx2, *srcy1, *srcy2;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 1);
    yys = (ys << 1) | (yt & 1);

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (WORD *)trg;
        srcx1 = (xt & 1 ? tmpsrc + 1 : tmpsrc - 1);
        srcx2 = (xt & 1 ? tmpsrc - 1 : tmpsrc + 1);
        srcy1 = (y & 1 ? tmpsrc + pitchs : tmpsrc - pitchs);
        srcy2 = (y & 1 ? tmpsrc - pitchs : tmpsrc + pitchs);

        for (x = 0; x < width; x++) {
            *tmptrg++ = (WORD)scale2x(colortab, &srcx1, &srcx2, &srcy1, &srcy2, &tmpsrc);
        }

        if (y & 1) {
            src += pitchs;
        }

        trg += pitcht;
    }
}


void render_24_scale2x(const video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    BYTE *tmptrg;
    unsigned int x, y, yys;
    const BYTE *srcx1, *srcx2, *srcy1, *srcy2;
    register DWORD color;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt * 3);
    yys = (ys << 1) | (yt & 1);

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = trg;
        srcx1 = (xt & 1 ? tmpsrc + 1 : tmpsrc - 1);
        srcx2 = (xt & 1 ? tmpsrc - 1 : tmpsrc + 1);
        srcy1 = (y & 1 ? tmpsrc + pitchs : tmpsrc - pitchs);
        srcy2 = (y & 1 ? tmpsrc - pitchs : tmpsrc + pitchs);

        for (x = 0; x < width; x++) {
            color = scale2x(colortab, &srcx1, &srcx2, &srcy1, &srcy2, &tmpsrc);
            *tmptrg++ = (BYTE)color;
            color >>= 8;
            *tmptrg++ = (BYTE)color;
            color >>= 8;
            *tmptrg++ = (BYTE)color;
        }

        if (y & 1) {
            src += pitchs;
        }

        trg += pitcht;
    }
}


void render_32_scale2x(const video_render_color_tables_t *color_tab,
                       const BYTE *src, BYTE *trg,
                       unsigned int width, const unsigned int height,
                       const unsigned int xs, const unsigned int ys,
                       const unsigned int xt, const unsigned int yt,
                       const unsigned int pitchs, const unsigned int pitcht)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    const BYTE *srcx1, *srcx2, *srcy1, *srcy2;
    unsigned int x, y, yys;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (DWORD *)trg;
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
