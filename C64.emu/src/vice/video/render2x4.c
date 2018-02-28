/*
 * render2x4.c - 2x4 renderers
 *
 * Written by
 *  groepaz <groepaz@gmx.net> based on the renderers written by
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

#include "render2x4.h"
#include "types.h"
#include <string.h>

/* 16 color 2x2 renderers */

void render_08_2x4_04(const video_render_color_tables_t *color_tab,
                      const BYTE *src, BYTE *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    WORD *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + xt;
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
    width >>= 1;
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
    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (WORD *)trg;
        if (!(y & 2) || doublescan) {
            if ((y & 3) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, (width << 1) + wfirst + wlast);
            } else {
                if (wfirst) {
                    *((BYTE *)tmptrg) = (BYTE)colortab[*tmpsrc++];
                    tmptrg = (WORD *)(((BYTE *)tmptrg) + 1);
                }
                for (x = 0; x < wstart; x++) {
                    *tmptrg++ = (WORD)colortab[*tmpsrc++];
                }
                for (x = 0; x < wfast; x++) {
                    tmptrg[0] = (WORD)colortab[tmpsrc[0]];
                    tmptrg[1] = (WORD)colortab[tmpsrc[1]];
                    tmptrg[2] = (WORD)colortab[tmpsrc[2]];
                    tmptrg[3] = (WORD)colortab[tmpsrc[3]];
                    tmptrg[4] = (WORD)colortab[tmpsrc[4]];
                    tmptrg[5] = (WORD)colortab[tmpsrc[5]];
                    tmptrg[6] = (WORD)colortab[tmpsrc[6]];
                    tmptrg[7] = (WORD)colortab[tmpsrc[7]];
                    tmpsrc += 8;
                    tmptrg += 8;
                }
                for (x = 0; x < wend; x++) {
                    *tmptrg++ = (WORD)colortab[*tmpsrc++];
                }
                if (wlast) {
                    *((BYTE *)tmptrg) = (BYTE)colortab[*tmpsrc];
                    tmptrg = (WORD *)(((BYTE *)tmptrg) + 1);
                }
            }
        } else {
            memset(trg, (BYTE)colortab[0], (width << 1) + wfirst + wlast);
        }
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_16_2x4_04(const video_render_color_tables_t *color_tab,
                      const BYTE *src, BYTE *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    DWORD color;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 1);
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
    width >>= 1;
    if (width < 8) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 8 pixels*/
        wstart = (unsigned int)(8 - (vice_ptr_to_int(trg) & 7));
        wfast = (width - wstart) >> 3; /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07; /* do not forget the rest*/
    }
    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (DWORD *)trg;
        if (!(y & 2) || doublescan) {
            if ((y & 3) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, ((width << 1) + wfirst + wlast) << 1);
            } else {
                if (wfirst) {
                    *((WORD *)tmptrg) = (WORD)colortab[*tmpsrc++];
                    tmptrg = (DWORD *)(((WORD *)tmptrg) + 1);
                }
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
                if (wlast) {
                    *((WORD *)tmptrg) = (WORD)colortab[*tmpsrc];
                }
            }
        } else {
            if (readable && y > yys + 3) { /* copy 4 lines before */
                memcpy(trg, trg - pitcht * 4, ((width << 1) + wfirst + wlast) << 1);
            } else {
                color = colortab[0];
                if (wfirst) {
                    *((WORD *)tmptrg) = (WORD)color;
                    tmptrg = (DWORD *)(((WORD *)tmptrg) + 1);
                }
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
                if (wlast) {
                    *((WORD *)tmptrg) = (WORD)color;
                }
            }
        }
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_24_2x4_04(const video_render_color_tables_t *color_tab,
                      const BYTE *src, BYTE *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    BYTE *tmptrg;
    unsigned int x, y, wlast, yys;
    register DWORD color;
    register DWORD tcolor;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt * 3);
    yys = (ys << 1) | (yt & 1);
    wlast = width & 1;
    width >>= 1;
    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = trg;
        if (!(y & 2) || doublescan) {
            if ((y & 3) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, ((width << 1) + wlast) * 3);
            } else {
                for (x = 0; x < width; x++) {
                    color = colortab[*tmpsrc++];
                    tcolor = color;
                    tmptrg[3] = tmptrg[0] = (BYTE)color;
                    color >>= 8;
                    tmptrg[4] = tmptrg[1] = (BYTE)color;
                    color >>= 8;
                    tmptrg[5] = tmptrg[2] = (BYTE)color;
                    tmptrg += 6;
                }
                if (wlast) {
                    color = colortab[*tmpsrc];
                    tmptrg[0] = (BYTE)color;
                    color >>= 8;
                    tmptrg[1] = (BYTE)color;
                    color >>= 8;
                    tmptrg[2] = (BYTE)color;
                }
            }
        } else {
            if (readable && y > yys + 3) { /* copy 4 lines before */
                memcpy(trg, trg - pitcht * 4, ((width << 1) + wlast) * 3);
            } else {
                color = colortab[0];
                for (x = 0; x < width; x++) {
                    tcolor = color;
                    tmptrg[3] = tmptrg[0] = (BYTE)tcolor;
                    tcolor >>= 8;
                    tmptrg[4] = tmptrg[1] = (BYTE)tcolor;
                    tcolor >>= 8;
                    tmptrg[5] = tmptrg[2] = (BYTE)tcolor;
                    tmptrg += 6;
                }
                if (wlast) {
                    tmptrg[0] = (BYTE)color;
                    color >>= 8;
                    tmptrg[1] = (BYTE)color;
                    color >>= 8;
                    tmptrg[2] = (BYTE)color;
                }
            }
        }
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_32_2x4_04(const video_render_color_tables_t *color_tab,
                      const BYTE *src, BYTE *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const DWORD *colortab = color_tab->physical_colors;
    const BYTE *tmpsrc;
    DWORD *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register DWORD color;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
    width >>= 1;
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
    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (DWORD *)trg;
        if (!(y & 2) || doublescan) {
            if ((y & 3) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, ((width << 1) + wfirst + wlast) << 2);
            } else {
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
        } else {
            if (readable && y > yys + 3) { /* copy 4 lines before */
                memcpy(trg, trg - pitcht * 4, ((width << 1) + wfirst + wlast) << 2);
            } else {
                color = colortab[0];
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
        }
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}
