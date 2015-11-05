/*
 * crtc-draw.c - A line-based CRTC emulation (under construction).
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include <stdio.h>
#include <string.h>

#include "crtc-draw.h"
#include "crtc.h"
#include "crtctypes.h"
#include "raster-modes.h"
#include "types.h"


/*
 * Bit expansion table: expands 4 bits to 4 bytes,
 * placing each bit in the lsb of the bytes.
 * The msb of the input is mapped to the lowest-address byte.
 *
 * The table maps nybbles at a time, since mapping a whole byte
 * would take 2 tables, each 16 times as big, and that would
 * not be so nice on the cpu cache.
 */
DWORD dwg_table[16];

static void init_drawing_tables(void)
{
    int byte, p;
    BYTE msk;

    for (byte = 0; byte < 0x10; byte++) {
        for (msk = 0x08, p = 0; p < 4; msk >>= 1, p++) {
            *((BYTE *)(dwg_table + byte) + p)
                = (byte & msk ? 1 : 0);
        }
    }
}


/***************************************************************************/

static void draw_standard_background(unsigned int start_pixel,
                                     unsigned int end_pixel)
{
    memset(crtc.raster.draw_buffer_ptr + start_pixel,
           0,
           end_pixel - start_pixel + 1);
}

/***************************************************************************/

/* inline function... */
static inline void DRAW(int reverse_flag, int offset, int scr_rel,
                        int xs, int xc, int xe)
{
    /* FIXME: `p' has to be aligned on a 4 byte boundary!
              Is there a better way than masking `offset'?  */
    BYTE *p = crtc.raster.draw_buffer_ptr + (offset & ~3);
    DWORD *pw = (DWORD *)p;
    BYTE *chargen_ptr, *screen_ptr;
    int screen_rel;
    int i, d;
    /* pointer to current chargen line */
    chargen_ptr = crtc.chargen_base
                  + crtc.chargen_rel
                  + (crtc.raster.ycounter & 0x0f);
    /* pointer to current screen line */
    screen_ptr = crtc.screen_base;
    screen_rel = ((scr_rel) + (xs));

    if (crtc.crsrmode && crtc.cursor_lines && crtc.crsrstate) {
        int crsrrel = ((crtc.regs[14] << 8) | crtc.regs[15]) & crtc.vaddr_mask;

        for (i = (xs); i < (xc); i++) {
            d = *(chargen_ptr
                  + (screen_ptr[screen_rel & crtc.vaddr_mask] << 4));

            /* FIXME: mask with 0x3fff (screen_rel must be expanded) */
            if (screen_rel == crsrrel) {
                d ^= 0xff;
            }

            screen_rel++;

            if ((reverse_flag)) {
                d ^= 0xff;
            }

            *pw++ = dwg_table[d >> 4];
            *pw++ = dwg_table[d & 0x0f];
        }
    } else {
        for (i = (xs); i < (xc); i++) {
            /* we use 16 bytes/char character generator */
            d = *(chargen_ptr
                  + (screen_ptr[screen_rel & crtc.vaddr_mask] << 4));
            screen_rel++;

            if ((reverse_flag)) {
                d ^= 0xff;
            }

            *pw++ = dwg_table[d >> 4];
            *pw++ = dwg_table[d & 0x0f];
        }
    }

    /* blank the rest */
    for (; i < (xe); i++) {
        *pw++ = 0;
        *pw++ = 0;
    }

    if (crtc.hires_draw_callback) {
        (crtc.hires_draw_callback)(p, xs, xc, scr_rel + xs, crtc.raster.ycounter);
    }
}

static void draw_standard_line(void)
{
    int rl_pos = crtc.xoffset + crtc.hjitter;
/*
    if (crtc.current_line == 1)
        printf("rl_pos=%d, scr_rel=%d, hw_cols=%d, rl_vis=%d, rl_len=%d\n",
               rl_pos, crtc.screen_rel, crtc.hw_cols, crtc.rl_visible,
               crtc.rl_len);
*/
    /* FIXME: check the ends against the maximum line length */
    /* the first part is left of rl_pos. Data is taken from prev. rl */
    if (rl_pos > 8) {
        DRAW(0,
             rl_pos % 8,
             crtc.prev_screen_rel,
             (crtc.prev_rl_len + 1) * crtc.hw_cols - (rl_pos / 8),
             crtc.prev_rl_visible * crtc.hw_cols,
             (crtc.prev_rl_len + 1) * crtc.hw_cols);
    }

    /* this is the "normal" part of the rasterline */
    DRAW(0,
         rl_pos,
         crtc.screen_rel,
         0,
         crtc.rl_visible * crtc.hw_cols,
         (crtc.rl_len + 1) * crtc.hw_cols);
}

static void draw_reverse_line(void)
{
    int rl_pos = crtc.xoffset + crtc.hjitter;

    /* the first part is left of rl_pos. Data is taken from prev. rl */
    if (rl_pos > 8) {
        DRAW(1,
             rl_pos % 8,
             crtc.prev_screen_rel,
             (crtc.prev_rl_len + 1) * crtc.hw_cols - (rl_pos / 8),
             crtc.prev_rl_visible * crtc.hw_cols,
             (crtc.prev_rl_len + 1) * crtc.hw_cols);
    }

    /* this is the "normal" part of the rasterline */
    DRAW(1,
         rl_pos,
         crtc.screen_rel,
         0,
         crtc.rl_visible * crtc.hw_cols,
         (crtc.rl_len + 1) * crtc.hw_cols);
}

static int get_std_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    *xs = 0;
    *xe = (crtc.rl_len + 1) * crtc.hw_cols;

    return 1;
}

static void draw_std_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    draw_standard_line();
}

static int get_rev_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    *xs = 0;
    *xe = (crtc.rl_len + 1) * crtc.hw_cols;

    return 1;
}

static void draw_rev_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    draw_reverse_line();
}

/***************************************************************************/

static void setup_modes(void)
{
    raster_modes_set(crtc.raster.modes, CRTC_STANDARD_MODE,
                     get_std_text,
                     draw_std_text_cached,
                     draw_standard_line,
                     draw_standard_background,
                     NULL /* draw_std_text_foreground */ );

    raster_modes_set(crtc.raster.modes, CRTC_REVERSE_MODE,
                     get_rev_text,
                     draw_rev_text_cached,
                     draw_reverse_line,
                     draw_standard_background,
                     NULL /* draw_rev_text_foreground*/);
}


void crtc_draw_init(void)
{
    init_drawing_tables();

    setup_modes();
}
