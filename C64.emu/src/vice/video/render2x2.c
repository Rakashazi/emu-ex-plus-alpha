/*
 * render2x2.c - 2x2 renderers
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

#include "render2x2.h"
#include "types.h"
#include <string.h>


/* 16 color 2x2 renderers */

void render_08_2x2_04(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint16_t *tmptrg;
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
        tmptrg = (uint16_t *)trg;
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, (width << 1) + wfirst + wlast);
            } else {
                if (wfirst) {
                    *((uint8_t *)tmptrg) = (uint8_t)colortab[*tmpsrc++];
                    tmptrg = (uint16_t *)(((uint8_t *)tmptrg) + 1);
                }
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
                if (wlast) {
                    *((uint8_t *)tmptrg) = (uint8_t)colortab[*tmpsrc];
                    tmptrg = (uint16_t *)(((uint8_t *)tmptrg) + 1);
                }
            }
        } else {
            memset(trg, (uint8_t)colortab[0], ((width << 1) + wfirst + wlast) << 1);
        }
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_16_2x2_04(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    uint32_t color;
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
        tmptrg = (uint32_t *)trg;
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, ((width << 1) + wfirst + wlast) << 1);
            } else {
                if (wfirst) {
                    *((uint16_t *)tmptrg) = (uint16_t)colortab[*tmpsrc++];
                    tmptrg = (uint32_t *)(((uint16_t *)tmptrg) + 1);
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
                    *((uint16_t *)tmptrg) = (uint16_t)colortab[*tmpsrc];
                }
            }
        } else {
            if (readable && y > yys + 1) { /* copy 2 lines before */
                memcpy(trg, trg - pitcht * 2, ((width << 1) + wfirst + wlast) << 1);
            } else {
                color = colortab[0];
                if (wfirst) {
                    *((uint16_t *)tmptrg) = (uint16_t)color;
                    tmptrg = (uint32_t *)(((uint16_t *)tmptrg) + 1);
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
                    *((uint16_t *)tmptrg) = (uint16_t)color;
                }
            }
        }
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_24_2x2_04(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint8_t *tmptrg;
    unsigned int x, y, wlast, yys;
    register uint32_t color;
    register uint32_t tcolor;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt * 3);
    yys = (ys << 1) | (yt & 1);
    wlast = width & 1;
    width >>= 1;
    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = trg;
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, ((width << 1) + wlast) * 3);
            } else {
                for (x = 0; x < width; x++) {
                    color = colortab[*tmpsrc++];
                    tmptrg[3] = tmptrg[0] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[4] = tmptrg[1] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[5] = tmptrg[2] = (uint8_t)color;
                    tmptrg += 6;
                }
                if (wlast) {
                    color = colortab[*tmpsrc];
                    tmptrg[0] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[1] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[2] = (uint8_t)color;
                }
            }
        } else {
            if (readable && y > yys + 1) { /* copy 2 lines before */
                memcpy(trg, trg - pitcht * 2, ((width << 1) + wlast) * 3);
            } else {
                color = colortab[0];
                for (x = 0; x < width; x++) {
                    tcolor = color;
                    tmptrg[3] = tmptrg[0] = (uint8_t)tcolor;
                    tcolor >>= 8;
                    tmptrg[4] = tmptrg[1] = (uint8_t)tcolor;
                    tcolor >>= 8;
                    tmptrg[5] = tmptrg[2] = (uint8_t)tcolor;
                    tmptrg += 6;
                }
                if (wlast) {
                    tmptrg[0] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[1] = (uint8_t)color;
                    color >>= 8;
                    tmptrg[2] = (uint8_t)color;
                }
            }
        }
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_32_2x2_04(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint32_t color;
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
        tmptrg = (uint32_t *)trg;
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
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
            if (readable && y > yys + 1) { /* copy 2 lines before */
                memcpy(trg, trg - pitcht * 2, ((width << 1) + wfirst + wlast) << 2);
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
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

/*****************************************************************************/
/*****************************************************************************/

/* 256 color 2x2 renderers */

#if 0
void render_08_2x2_08(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *pre;
    const uint8_t *tmppre;
    const uint8_t *tmpsrc;
    uint16_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint16_t color;

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
    pre = src - pitchs - 1;
    for (y = yys; y < (yys + height); y++) {
        tmppre = pre;
        tmpsrc = src;
        tmptrg = (uint16_t *)trg;
        if ((y & 1) || doublescan) {
            if (wfirst) {
                *((uint8_t *)tmptrg) = (uint8_t)colortab[*tmpsrc++ | (*tmppre++ << 4)];
                tmptrg = (uint16_t *)(((uint8_t *)tmptrg) + 1);
            }
            for (x = 0; x < wstart; x++) {
                *tmptrg++ = (uint16_t)colortab[*tmpsrc++ | (*tmppre++ << 4)];
            }
            for (x = 0; x < wfast; x++) {
                tmptrg[0] = (uint16_t)colortab[tmpsrc[0] | (tmppre[0] << 4)];
                tmptrg[1] = (uint16_t)colortab[tmpsrc[1] | (tmppre[1] << 4)];
                tmptrg[2] = (uint16_t)colortab[tmpsrc[2] | (tmppre[2] << 4)];
                tmptrg[3] = (uint16_t)colortab[tmpsrc[3] | (tmppre[3] << 4)];
                tmptrg[4] = (uint16_t)colortab[tmpsrc[4] | (tmppre[4] << 4)];
                tmptrg[5] = (uint16_t)colortab[tmpsrc[5] | (tmppre[5] << 4)];
                tmptrg[6] = (uint16_t)colortab[tmpsrc[6] | (tmppre[6] << 4)];
                tmptrg[7] = (uint16_t)colortab[tmpsrc[7] | (tmppre[7] << 4)];
                tmpsrc += 8;
                tmppre += 8;
                tmptrg += 8;
            }
            for (x = 0; x < wend; x++) {
                *tmptrg++ = (uint16_t)colortab[*tmpsrc++ | (*tmppre++ << 4)];
            }
            if (wlast) {
                *((uint8_t *)tmptrg) = (uint8_t)colortab[*tmpsrc | (*tmppre << 4)];
            }
            if (y & 1) {
                pre = src - 1;
                src += pitchs;
            }
        } else {
            color = (uint16_t)colortab[0];
            if (wfirst) {
                *((uint8_t *)tmptrg) = (uint8_t)color;
                tmptrg = (uint16_t *)(((uint8_t *)tmptrg) + 1);
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
                *((uint8_t *)tmptrg) = (uint8_t)color;
            }
        }
        trg += pitcht;
    }
}

void render_16_2x2_08(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *pre;
    const uint8_t *tmppre;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint32_t color;

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
        wstart = (unsigned int)(8 - (vice_ptr_to_uint(trg) & 7));
        wfast = (width - wstart) >> 3; /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07; /* do not forget the rest*/
    }
    pre = src - pitchs - 1;
    for (y = yys; y < (yys + height); y++) {
        tmppre = pre;
        tmpsrc = src;
        tmptrg = (uint32_t *)trg;
        if ((y & 1) || doublescan) {
            if (wfirst) {
                *((uint16_t *)tmptrg) = (uint16_t)colortab[*tmpsrc++ | (*tmppre++ << 4)];
                tmptrg = (uint32_t *)(((uint16_t *)tmptrg) + 1);
            }
            for (x = 0; x < wstart; x++) {
                *tmptrg++ = colortab[*tmpsrc++ | (*tmppre++ << 4)];
            }
            for (x = 0; x < wfast; x++) {
                tmptrg[0] = colortab[tmpsrc[0] | (tmppre[0] << 4)];
                tmptrg[1] = colortab[tmpsrc[1] | (tmppre[1] << 4)];
                tmptrg[2] = colortab[tmpsrc[2] | (tmppre[2] << 4)];
                tmptrg[3] = colortab[tmpsrc[3] | (tmppre[3] << 4)];
                tmptrg[4] = colortab[tmpsrc[4] | (tmppre[4] << 4)];
                tmptrg[5] = colortab[tmpsrc[5] | (tmppre[5] << 4)];
                tmptrg[6] = colortab[tmpsrc[6] | (tmppre[6] << 4)];
                tmptrg[7] = colortab[tmpsrc[7] | (tmppre[7] << 4)];
                tmpsrc += 8;
                tmppre += 8;
                tmptrg += 8;
            }
            for (x = 0; x < wend; x++) {
                *tmptrg++ = colortab[*tmpsrc++ | (*tmppre++ << 4)];
            }
            if (wlast) {
                *((uint16_t *)tmptrg) = (uint16_t)colortab[*tmpsrc | (*tmppre << 4)];
            }
            if (y & 1) {
                pre = src - 1;
                src += pitchs;
            }
        } else {
            color = colortab[0];
            if (wfirst) {
                *((uint16_t *)tmptrg) = (uint16_t)color;
                tmptrg = (uint32_t *)(((uint16_t *)tmptrg) + 1);
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
                *((uint16_t *)tmptrg) = (uint16_t)color;
            }
        }
        trg += pitcht;
    }
}

void render_24_2x2_08(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *pre;
    const uint8_t *tmppre;
    const uint8_t *tmpsrc;
    uint8_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint32_t color;
    register uint32_t tcolor;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt * 3);
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
    width >>= 1;
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
    pre = src - pitchs - 1;
    for (y = yys; y < (yys + height); y++) {
        tmppre = pre;
        tmpsrc = src;
        tmptrg = trg;
        if ((y & 1) || doublescan) {
            if (wfirst) {
                color = colortab[*tmpsrc++ | (*tmppre++ << 4)];
                *tmptrg++ = (uint8_t)color;
                color >>= 8;
                *tmptrg++ = (uint8_t)color;
                color >>= 8;
                *tmptrg++ = (uint8_t)color;
            }
            for (x = 0; x < wstart; x++) {
                color = colortab[*tmpsrc++ | (*tmppre++ << 4)];
                tcolor = color;
                tmptrg[0] = (uint8_t)color;
                color >>= 8;
                tmptrg[1] = (uint8_t)color;
                color >>= 8;
                tmptrg[2] = (uint8_t)color;
                tmptrg[3] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[4] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[5] = (uint8_t)tcolor;
                tmptrg += 6;
            }
            for (x = 0; x < wfast; x++) {
                color = colortab[tmpsrc[0] | (tmppre[0] << 4)];
                tcolor = color;
                tmptrg[0] = (uint8_t)color;
                color >>= 8;
                tmptrg[1] = (uint8_t)color;
                color >>= 8;
                tmptrg[2] = (uint8_t)color;
                tmptrg[3] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[4] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[5] = (uint8_t)tcolor;
                color = colortab[tmpsrc[1] | (tmppre[1] << 4)];
                tcolor = color;
                tmptrg[6] = (uint8_t)color;
                color >>= 8;
                tmptrg[7] = (uint8_t)color;
                color >>= 8;
                tmptrg[8] = (uint8_t)color;
                tmptrg[9] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[10] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[11] = (uint8_t)tcolor;
                color = colortab[tmpsrc[2] | (tmppre[2] << 4)];
                tcolor = color;
                tmptrg[12] = (uint8_t)color;
                color >>= 8;
                tmptrg[13] = (uint8_t)color;
                color >>= 8;
                tmptrg[14] = (uint8_t)color;
                tmptrg[15] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[16] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[17] = (uint8_t)tcolor;
                color = colortab[tmpsrc[3] | (tmppre[3] << 4)];
                tcolor = color;
                tmptrg[18] = (uint8_t)color;
                color >>= 8;
                tmptrg[19] = (uint8_t)color;
                color >>= 8;
                tmptrg[20] = (uint8_t)color;
                tmptrg[21] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[22] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[23] = (uint8_t)tcolor;
                tmpsrc += 4;
                tmppre += 4;
                tmptrg += 24;
            }
            for (x = 0; x < wend; x++) {
                color = colortab[*tmpsrc++ | (*tmppre++ << 4)];
                tcolor = color;
                tmptrg[0] = (uint8_t)color;
                color >>= 8;
                tmptrg[1] = (uint8_t)color;
                color >>= 8;
                tmptrg[2] = (uint8_t)color;
                tmptrg[3] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[4] = (uint8_t)tcolor;
                tcolor >>= 8;
                tmptrg[5] = (uint8_t)tcolor;
                tmptrg += 6;
            }
            if (wlast) {
                color = colortab[*tmpsrc | (*tmppre << 4)];
                tmptrg[0] = (uint8_t)color;
                color >>= 8;
                tmptrg[1] = (uint8_t)color;
                color >>= 8;
                tmptrg[2] = (uint8_t)color;
            }
            if (y & 1) {
                pre = src - 1;
                src += pitchs;
            }
        } else {
            if (wfirst) {
                *tmptrg++ = 0;
                *tmptrg++ = 0;
                *tmptrg++ = 0;
            }
            for (x = 0; x < wstart; x++) {
                tmptrg[0] = 0;
                tmptrg[1] = 0;
                tmptrg[2] = 0;
                tmptrg[3] = 0;
                tmptrg[4] = 0;
                tmptrg[5] = 0;
                tmptrg += 6;
            }
            for (x = 0; x < wfast; x++) {
                tmptrg[0] = 0;
                tmptrg[1] = 0;
                tmptrg[2] = 0;
                tmptrg[3] = 0;
                tmptrg[4] = 0;
                tmptrg[5] = 0;
                tmptrg[6] = 0;
                tmptrg[7] = 0;
                tmptrg[8] = 0;
                tmptrg[9] = 0;
                tmptrg[10] = 0;
                tmptrg[11] = 0;
                tmptrg[12] = 0;
                tmptrg[13] = 0;
                tmptrg[14] = 0;
                tmptrg[15] = 0;
                tmptrg[16] = 0;
                tmptrg[17] = 0;
                tmptrg[18] = 0;
                tmptrg[19] = 0;
                tmptrg[20] = 0;
                tmptrg[21] = 0;
                tmptrg[22] = 0;
                tmptrg[23] = 0;
                tmptrg += 24;
            }
            for (x = 0; x < wend; x++) {
                tmptrg[0] = 0;
                tmptrg[1] = 0;
                tmptrg[2] = 0;
                tmptrg[3] = 0;
                tmptrg[4] = 0;
                tmptrg[5] = 0;
                tmptrg += 6;
            }
            if (wlast) {
                tmptrg[0] = 0;
                tmptrg[1] = 0;
                tmptrg[2] = 0;
            }
        }
        trg += pitcht;
    }
}

void render_32_2x2_08(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *pre;
    const uint8_t *tmppre;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint32_t color;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;
    width >>= 1;
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
    pre = src - pitchs - 1;
    for (y = yys; y < (yys + height); y++) {
        tmppre = pre;
        tmpsrc = src;
        tmptrg = (uint32_t *)trg;
        if ((y & 1) || doublescan) {
            if (wfirst) {
                *tmptrg++ = colortab[*tmpsrc++ | (*tmppre++ << 4)];
            }
            for (x = 0; x < wstart; x++) {
                color = colortab[*tmpsrc++ | (*tmppre++ << 4)];
                *tmptrg++ = color;
                *tmptrg++ = color;
            }
            for (x = 0; x < wfast; x++) {
                color = colortab[tmpsrc[0] | (tmppre[0] << 4)];
                tmptrg[0] = color;
                tmptrg[1] = color;
                color = colortab[tmpsrc[1] | (tmppre[1] << 4)];
                tmptrg[2] = color;
                tmptrg[3] = color;
                color = colortab[tmpsrc[2] | (tmppre[2] << 4)];
                tmptrg[4] = color;
                tmptrg[5] = color;
                color = colortab[tmpsrc[3] | (tmppre[3] << 4)];
                tmptrg[6] = color;
                tmptrg[7] = color;
                tmpsrc += 4;
                tmppre += 4;
                tmptrg += 8;
            }
            for (x = 0; x < wend; x++) {
                color = colortab[*tmpsrc++ | (*tmppre++ << 4)];
                *tmptrg++ = color;
                *tmptrg++ = color;
            }
            if (wlast) {
                *tmptrg = colortab[*tmpsrc | (*tmppre << 4)];
            }
            if (y & 1) {
                pre = src - 1;
                src += pitchs;
            }
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
                tmptrg += 8;
            }
            for (x = 0; x < wend; x++) {
                *tmptrg++ = color;
                *tmptrg++ = color;
            }
            if (wlast) {
                *tmptrg = color;
            }
        }
        trg += pitcht;
    }
}
#endif
