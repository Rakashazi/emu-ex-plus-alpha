/*
 * render1x2.c - 1x2 renderers (unfiltered)
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

#include <string.h>

#include "render-common.h"
#include "render1x2.h"
#include "types.h"
#include "video.h"

/* 16 color 1x2 renderer */

void render_32_1x2_interlaced(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
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
    int interlace_odd_frame = config->interlace_field & 1;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);
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
            render_source_line(tmptrg, tmpsrc, colortab, wstart, wfast, wend);
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
void render_32_1x2_non_interlaced(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                                  unsigned int width, const unsigned int height,
                                  const unsigned int xs, const unsigned int ys,
                                  const unsigned int xt, const unsigned int yt,
                                  const unsigned int pitchs, const unsigned int pitcht,
                                  const unsigned int doublescan, video_render_config_t *config)
{
    const uint32_t *colortab = color_tab->physical_colors;
    const uint8_t *tmpsrc;
    uint32_t *tmptrg;
    unsigned int y, wstart, wfast, wend, yys;
    uint32_t color;
    int readable = config->readable;

    src = src + pitchs * ys + xs;
    trg = trg + pitcht * yt + (xt << 2);
    yys = (ys << 1) | (yt & 1);
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
         * Non-interlace code path, supporting doublescan
         */
        if (!(y & 1) || doublescan) {
            if ((y & 1) && readable && y > yys) { /* copy previous line */
                memcpy(trg, trg - pitcht, width << 2);
            } else {
                render_source_line(tmptrg, tmpsrc, colortab, wstart, wfast, wend);
            }
        } else {
            if (readable && y > yys + 1) { /* copy 2 lines before */
                memcpy(trg, trg - pitcht * 2, width << 2);
            } else {
                color = colortab[0];
                render_solid_line(tmptrg, tmpsrc, color, wstart, wfast, wend);
            }
        }
        
        if (y & 1) {
            src += pitchs;
        }
        trg += pitcht;
    }
}

void render_32_1x2(const video_render_color_tables_t *color_tab, const uint8_t *src, uint8_t *trg,
                   unsigned int width, const unsigned int height,
                   const unsigned int xs, const unsigned int ys,
                   const unsigned int xt, const unsigned int yt,
                   const unsigned int pitchs, const unsigned int pitcht,
                   const unsigned int doublescan, video_render_config_t *config)
{
    if (config->interlaced) {
        /* interlaced render with completely transparent scanlines */
        render_32_1x2_interlaced(color_tab, src, trg, width, height, xs, ys,
                                 xt, yt, pitchs, pitcht, config, color_tab->physical_colors[0] & 0x00ffffff);
    } else {
        render_32_1x2_non_interlaced(color_tab, src, trg, width, height, xs, ys,
                                     xt, yt, pitchs, pitcht, doublescan, config);
    }
}
