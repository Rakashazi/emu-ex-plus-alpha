/*
 * render2x2.c - 2x2 renderers (unfiltered)
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

#include "render-common.h"
#include "render2x2.h"
#include "types.h"
#include <string.h>


/* 16 color 2x2 renderer */
void render_32_2x2_interlaced(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                              unsigned int width, const unsigned int height,
                              const unsigned int xs, const unsigned int ys,
                              const unsigned int xt, const unsigned int yt,
                              const unsigned int pitchs, const unsigned int pitcht,
                              video_render_config_t *config, uint32_t scanline_color)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    uint32_t *scanline = NULL;
    unsigned int y, wstart, wfast, wend, yys;
    unsigned int wfirst, wlast;
    int interlace_odd_frame = config->interlace_field & 1;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);
    wfirst = xt & 1;
    width -= wfirst;
    wlast = width & 1;

    if (width < 8) {
        wstart = width;
        wfast = 0;
        wend = 0;
    } else {
        /* alignment: 8 pixels*/
        wstart = (unsigned int)8 - (vice_ptr_to_uint(trg) & 7);
        wfast = (width - wstart) >> 3; /* fast loop for 8 pixel segments*/
        wend = (width - wstart) & 0x07;  /* do not forget the rest*/
    }

    for (y = yys; y < (yys + height); y++) {
        tmpsrc = src;
        tmptrg = (uint32_t *)trg;

        /*
         * If it's an even line and an even frame, or if it's an odd line
         * and an odd frame, then this line contains new pixels from the video
         * chip. Otherwise it contains a translucent blank line to be alpha
         * blended with the previous frame.
         */
        if ((y & 1) == interlace_odd_frame) {
            /* New pixels */
            render_source_line_2x(tmptrg, tmpsrc, colortab, wstart, wfast, wend, wfirst, wlast);
        } else {
            /* Blank line */
            if (scanline) {
                /* Copy the first blank line we created */
                memcpy(tmptrg, scanline, pitcht);
            } else {
                /* Next time, memcpy this blank line as it's much faster. */
                scanline = tmptrg;
                render_solid_line(tmptrg, tmpsrc, scanline_color, wstart, wfast, wend);
            }
        }
        
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

static inline
void render_32_2x2_non_interlaced(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    unsigned int x, y, wfirst, wstart, wfast, wend, wlast, yys;
    int readable = config->readable;
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

    /*
     * The outer loop is distributed among a sensible number of threads.
     * Because in double scan mode lines are copied, we need to make sure
     * each thread works in chunks of 2 lines, ensuring that copied data
     * has been created at the time it's copied.
     */

    if (yys & 1) {
        /* We also need to make sure we start on an even line. */
        yys--;
        height++;
    }

#ifdef _OPENMP
    #pragma omp parallel for private(y,x) schedule(static,2)
#endif
    for (y = yys; y < (yys + height); y++) {
        const uint8_t *tmpsrc;
        uint32_t *tmptrg;
        uint32_t color;
        
        tmpsrc = (src + pitchs * ys + xs) + ((y - yys) / 2 * pitchs);
        tmptrg = (uint32_t *)((trg + pitcht * yt + (xt << 2)) + (y - yys) * pitcht);
        
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
                memcpy(tmptrg, (uint8_t *)tmptrg - pitcht, ((width << 1) + wfirst + wlast) << 2);
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
            /* Doublescan is disabled */
#ifndef _OPENMP
            if (readable && y > yys + 1) { /* copy 2 lines before, can't do with openmp */
                memcpy(tmptrg, (uint8_t *)tmptrg - pitcht * 2, ((width << 1) + wfirst + wlast) << 2);
            } else {
#endif
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
#ifndef _OPENMP
            }
#endif
        }
    }
}

/*****************************************************************************/
/*****************************************************************************/

/* 256 color 2x2 renderer */

#if 0
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

void render_32_2x2(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                   unsigned int width, const unsigned int height,
                   const unsigned int xs, const unsigned int ys,
                   const unsigned int xt, const unsigned int yt,
                   const unsigned int pitchs, const unsigned int pitcht,
                   const unsigned int doublescan, video_render_config_t *config)
{
    if (config->interlaced) {
        /* interlaced render with completely transparent scanlines */
        render_32_2x2_interlaced(color_tab, src, trg, width, height, xs, ys,
                                 xt, yt, pitchs, pitcht, config, color_tab->physical_colors[0] & 0x00ffffff);
    } else {
        render_32_2x2_non_interlaced(color_tab, src, trg, width, height, xs, ys,
                                     xt, yt, pitchs, pitcht, doublescan, config);
    }
}
