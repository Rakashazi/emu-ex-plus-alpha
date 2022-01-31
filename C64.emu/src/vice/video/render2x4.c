/*
 * render2x4.c - 2x4 renderers (unfiltered)
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

#include <string.h>

#include "render-common.h"
#include "render2x4.h"
#include "types.h"

/* 16 color 2x4 renderer */

void render_32_2x4_interlaced(const video_render_color_tables_t *color_tab,
                              const uint8_t *src, uint8_t *trg,
                              unsigned int width, const unsigned int height,
                              const unsigned int xs, const unsigned int ys,
                              const unsigned int xt, const unsigned int yt,
                              const unsigned int pitchs, const unsigned int pitcht,
                              video_render_config_t *config, const uint32_t scanline_color)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    uint32_t *copyline = NULL;
    uint32_t *scanline = NULL;
    unsigned int y, wfirst, wstart, wfast, wend, wlast, yys;
    int interlace_odd_frame = config->interlace_field & 1;

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
        
        /*
         * If it's an even source line and an even frame, or if it's an odd source
         * line and an odd frame, then this line contains new pixels from the video
         * chip. Otherwise it contains a translucent blank line to be alpha
         * blended with the previous frame.
         */
        if (((y / 2) & 1) == interlace_odd_frame) {
            /* New pixels */
            if (copyline) {
                /* copy the previously rendered line */
                memcpy(tmptrg, copyline, pitcht);
                copyline = NULL;
            } else {
                /* Next time, memcpy this blank line as it's much faster. */
                copyline = tmptrg;
                render_source_line_2x(tmptrg, tmpsrc, colortab, wstart, wfast, wend, wfirst, wlast);
            }
        } else {
            /* Blank line */
            if (scanline) {
                /* Copy the first blank line we created */
                memcpy(tmptrg, scanline, pitcht);
            } else {
                /* Next time, memcpy this blank line as it's much faster. */
                scanline = tmptrg;
                render_solid_line_2x(tmptrg, tmpsrc, scanline_color, wstart, wfast, wend, wfirst, wlast);
            }
        }
        
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

static void render_32_2x4_non_interlaced(const video_render_color_tables_t *color_tab,
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
    unsigned int y, wfirst, wstart, wfast, wend, wlast, yys;
    register uint32_t color;

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
        if (!(y & 2) || doublescan) {
            /* 1st or 2nd line in a group of four, or doublescan is enabled */
            if ((y & 3) && y > yys) { /* copy previous line */
                /* 2nd, 3rd, or 4th line in a group of four, and not the first line rendered by this invocation */
                memcpy(trg, trg - pitcht, ((width << 1) + wfirst + wlast) << 2);
            } else {
                /* first line in a group of four, or the first line rendered by this invocation */
                render_source_line_2x(tmptrg, tmpsrc, colortab, wstart, wfast, wend, wfirst, wlast);
            }
        } else {
            /* 3rd or 4th line in a group of 4, and doublescan is disabled */
            if (y > yys + 3) { /* copy 4 lines before */
                /* At least 4 lines have been rendered this invocation, safe to copy */
                memcpy(trg, trg - pitcht * 4, ((width << 1) + wfirst + wlast) << 2);
            } else {
                color = colortab[0];
                render_solid_line_2x(tmptrg, tmpsrc, color, wstart, wfast, wend, wfirst, wlast);
            }
        }
        if ((y & 3) == 3) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_32_2x4(const video_render_color_tables_t *color_tab,
                      const uint8_t *src, uint8_t *trg,
                      unsigned int width, const unsigned int height,
                      const unsigned int xs, const unsigned int ys,
                      const unsigned int xt, const unsigned int yt,
                      const unsigned int pitchs, const unsigned int pitcht,
                      const unsigned int doublescan, video_render_config_t *config)
{
    if (config->interlaced) {
        /* interlaced render with completely transparent scanlines */
        render_32_2x4_interlaced(color_tab, src, trg, width, height, xs, ys,
                                    xt, yt, pitchs, pitcht, config, color_tab->physical_colors[0] & 0x00ffffff);
    } else {
        render_32_2x4_non_interlaced(color_tab, src, trg, width, height, xs, ys,
                                        xt, yt, pitchs, pitcht, doublescan, config);
    }
}
