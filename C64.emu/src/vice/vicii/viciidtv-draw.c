/*
 * viciidtv-draw.c - Rendering for the MOS6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * DTV sections written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  Daniel Kahlin <daniel@kahlin.net>
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

#include "mem.h"
#include "raster-cache-fill.h"
#include "raster-cache-fill-1fff.h"
#include "raster-cache-fill-39ff.h"
#include "raster-cache-text-ext.h"
#include "raster-cache-text-std.h"
#include "raster-cache.h"
#include "raster-modes.h"
#include "raster.h"
#include "types.h"
#include "vicii-draw.h"
#include "viciitypes.h"
#include "viewport.h"


#define GFX_MSK_LEFTBORDER_SIZE ((VICII_MAX_SPRITE_WIDTH - VICII_RASTER_X(0) \
                                  + vicii.screen_leftborderwidth ) / 8 + 1)

/* The following tables are used to speed up the drawing.  We do not use
   multi-dimensional arrays as we can optimize better this way...  */

/* foreground(4) | background(4) | nibble(4) -> 4 pixels.  */
static DWORD hr_table[16 * 16 * 16];

/* mc flag(1) | idx(2) | byte(8) -> index into double-pixel table.  */
static BYTE mc_table[2 * 4 * 256];
static BYTE mcmsktable[512];


/* These functions draw the background from `start_pixel' to `end_pixel'.  */

static void draw_std_background(unsigned int start_pixel,
                                unsigned int end_pixel)
{
    BYTE background_color;
    unsigned int gfxstart, gfxend;

    background_color = (BYTE)vicii.raster.background_color;

    if (VICII_IS_ILLEGAL_MODE(vicii.raster.video_mode)) {
        background_color = 0;
    }

    gfxstart = vicii.raster.geometry->gfx_position.x + vicii.raster.xsmooth;
    gfxend = gfxstart + vicii.raster.geometry->gfx_size.width;

    if (start_pixel < gfxstart) {
        if (end_pixel < gfxstart) {
            memset(vicii.raster.draw_buffer_ptr + start_pixel,
                   vicii.raster.xsmooth_color,
                   end_pixel - start_pixel + 1);
        } else {
            if (end_pixel < gfxend) {
                memset(vicii.raster.draw_buffer_ptr + start_pixel,
                       vicii.raster.xsmooth_color,
                       gfxstart - start_pixel);
                memset(vicii.raster.draw_buffer_ptr + gfxstart,
                       background_color,
                       end_pixel - gfxstart + 1);
            } else {
                memset(vicii.raster.draw_buffer_ptr + start_pixel,
                       vicii.raster.xsmooth_color,
                       gfxstart - start_pixel);
                memset(vicii.raster.draw_buffer_ptr + gfxstart,
                       background_color,
                       gfxend - gfxstart);
                memset(vicii.raster.draw_buffer_ptr + gfxend,
                       vicii.raster.xsmooth_color,
                       end_pixel - gfxend + 1);
            }
        }
    } else {
        if (start_pixel < gfxend) {
            if (end_pixel < gfxend) {
                memset(vicii.raster.draw_buffer_ptr + start_pixel,
                       background_color,
                       end_pixel - start_pixel + 1);
            } else {
                memset(vicii.raster.draw_buffer_ptr + start_pixel,
                       background_color,
                       gfxend - start_pixel);
                memset(vicii.raster.draw_buffer_ptr + gfxend,
                       vicii.raster.xsmooth_color,
                       end_pixel - gfxend + 1);
            }
        } else {
            memset(vicii.raster.draw_buffer_ptr + start_pixel,
                   vicii.raster.xsmooth_color,
                   end_pixel - start_pixel + 1);
        }
    }

    if (vicii.raster.xsmooth_shift_right) {
        int pos;

        pos = (start_pixel - vicii.raster.geometry->gfx_position.x) / 8;

        if (pos >= 0 && pos < VICII_SCREEN_TEXTCOLS) {
            if (vicii.raster.video_mode == VICII_HIRES_BITMAP_MODE) {
                background_color = vicii.vbuf[pos] & 0xf;
            }
            if (vicii.raster.video_mode == VICII_EXTENDED_TEXT_MODE) {
                int bg_idx;

                bg_idx = vicii.vbuf[pos] >> 6;

                if (bg_idx > 0) {
                    background_color = vicii.ext_background_color[bg_idx - 1];
                }
            }
            if (VICII_IS_ILLEGAL_MODE(vicii.raster.video_mode)) {
                background_color = 0;
            }
            memset(vicii.raster.draw_buffer_ptr + start_pixel + 8,
                   background_color, vicii.raster.xsmooth_shift_right);
        }
        vicii.raster.xsmooth_shift_right = 0;
    }
}

static void draw_idle_std_background(unsigned int start_pixel,
                                     unsigned int end_pixel)
{
    memset(vicii.raster.draw_buffer_ptr + start_pixel,
           vicii.raster.idle_background_color,
           end_pixel - start_pixel + 1);
}

/* If unaligned 32-bit access is not allowed, the graphics is stored in a
   temporary aligned buffer, and later copied to the real frame buffer.  This
   is ugly, but should be hopefully faster than accessing 8 bits at a time
   anyway.  */

#ifndef ALLOW_UNALIGNED_ACCESS
static DWORD _aligned_line_buffer[VICII_SCREEN_XPIX / 2 + 1];
static BYTE *const aligned_line_buffer = (BYTE *)_aligned_line_buffer;
#endif


/* Pointer to the start of the graphics area on the frame buffer.  */
#define GFX_PTR() (vicii.raster.draw_buffer_ptr + (vicii.raster.geometry->gfx_position.x + vicii.raster.xsmooth))

#ifdef ALLOW_UNALIGNED_ACCESS
#define ALIGN_DRAW_FUNC(name, xs, xe, gfx_msk_ptr) name(GFX_PTR(), (xs), (xe), (gfx_msk_ptr))
#else
#define ALIGN_DRAW_FUNC(name, xs, xe, gfx_msk_ptr)            \
    do {                                                      \
        name(aligned_line_buffer, (xs), (xe), (gfx_msk_ptr)); \
        memcpy(GFX_PTR() + (xs) * 8,                          \
               aligned_line_buffer + (xs) * 8,                \
               ((xe) - (xs) + 1) * 8);                        \
    } while (0)
#endif


/* Standard text mode.  */

static int get_std_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    int r;

    if (cache->background_data[0] != vicii.raster.background_color
        || cache->chargen_ptr != vicii.chargen_ptr) {
        cache->background_data[0] = vicii.raster.background_color;
        cache->chargen_ptr = vicii.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text(cache->foreground_data,
                                    vicii.vbuf,
                                    vicii.chargen_ptr + vicii.raster.ycounter,
                                    VICII_SCREEN_TEXTCOLS,
                                    xs, xe,
                                    rr);

    r |= raster_cache_data_fill(cache->color_data_1,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_std_text(BYTE *p, unsigned int xs, unsigned int xe,
                                  BYTE *gfx_msk_ptr)
{
    BYTE *char_ptr, *msk_ptr;
    unsigned int i;
    BYTE cmask;
    BYTE fcolor;
    BYTE bcolor;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    for (i = xs; i <= xe; i++) {
        *(msk_ptr + i) = *(char_ptr + vicii.vbuf[i] * 8);
        fcolor = vicii.dtvpalette[vicii.cbuf[i] & cmask];
        bcolor = vicii.raster.background_color;
        *(p + i * 8 + 0) = (*(msk_ptr + i) & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (*(msk_ptr + i) & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (*(msk_ptr + i) & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (*(msk_ptr + i) & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (*(msk_ptr + i) & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (*(msk_ptr + i) & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (*(msk_ptr + i) & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (*(msk_ptr + i) & 0x01) ? fcolor : bcolor;
    }
}

inline static void _draw_std_text_cached(BYTE *p, unsigned int xs,
                                         unsigned int xe,
                                         raster_cache_t *cache)
{
    BYTE *msk_ptr, *foreground_data, *color_data;
    unsigned int i;
    BYTE cmask;
    BYTE fcolor;
    BYTE bcolor;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;
    foreground_data = cache->foreground_data;
    color_data = cache->color_data_1;

    for (i = xs; i <= xe; i++) {
        *(msk_ptr + i) = foreground_data[i];
        fcolor = vicii.dtvpalette[color_data[i] & cmask];
        bcolor = vicii.raster.background_color;
        *(p + i * 8 + 0) = (*(msk_ptr + i) & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (*(msk_ptr + i) & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (*(msk_ptr + i) & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (*(msk_ptr + i) & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (*(msk_ptr + i) & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (*(msk_ptr + i) & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (*(msk_ptr + i) & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (*(msk_ptr + i) & 0x01) ? fcolor : bcolor;
    }
}

static void draw_std_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_std_text_cached, xs, xe, cache);
}

static void draw_std_text(void)
{
    ALIGN_DRAW_FUNC(_draw_std_text, 0, VICII_SCREEN_TEXTCOLS - 1,
                    vicii.raster.gfx_msk);
}

#define DRAW_STD_TEXT_BYTE(p, b, f) \
    do {                            \
        if ((b) & 0x80) {           \
            *(p) = (f);             \
        }                           \
        if ((b) & 0x40) {           \
            *((p) + 1) = (f);       \
        }                           \
        if ((b) & 0x20) {           \
            *((p) + 2) = (f);       \
        }                           \
        if ((b) & 0x10) {           \
            *((p) + 3) = (f);       \
        }                           \
        if ((b) & 0x08) {           \
            *((p) + 4) = (f);       \
        }                           \
        if ((b) & 0x04) {           \
            *((p) + 5) = (f);       \
        }                           \
        if ((b) & 0x02) {           \
            *((p) + 6) = (f);       \
        }                           \
        if ((b) & 0x01) {           \
            *((p) + 7) = (f);       \
        }                           \
    } while (0)

static void draw_std_text_foreground(unsigned int start_char, unsigned int end_char)
{
    unsigned int i;
    BYTE *char_ptr, *msk_ptr, *p;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = vicii.raster.gfx_msk + GFX_MSK_LEFTBORDER_SIZE;
    p = GFX_PTR() + 8 * start_char;

    for (i = start_char; i <= end_char; i++, p += 8) {
        BYTE b, f;

        b = char_ptr[vicii.vbuf[i - vicii.buf_offset] * 8];

        if (vicii.raster.last_video_mode == VICII_EXTENDED_TEXT_MODE) {
            b = char_ptr[(vicii.vbuf[i - vicii.buf_offset] & 0x3f) * 8];
        }

        if (vicii.raster.last_video_mode == VICII_HIRES_BITMAP_MODE) {
            unsigned int j = ((vicii.memptr << 3) + vicii.raster.ycounter + i * 8) & 0x1fff;
            if (j & 0x1000) {
                b = vicii.bitmap_high_ptr[j & 0xfff];
            } else {
                b = vicii.bitmap_low_ptr[j];
            }
        }

        f = vicii.cbuf[i - vicii.buf_offset];
        f = vicii.dtvpalette[f & cmask];

        if (vicii.raster.xsmooth_shift_left > 0) {
            b = (b >> vicii.raster.xsmooth_shift_left) << vicii.raster.xsmooth_shift_left;
        }

        *(msk_ptr + i) = b;
        DRAW_STD_TEXT_BYTE(p, b, f);
    }
}

/* Hires Bitmap mode.  */

static int get_hires_bitmap(raster_cache_t *cache, unsigned int *xs,
                            unsigned int *xe, int rr)
{
    int r;

    r = raster_cache_data_fill(cache->background_data,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill_1fff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8 + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_hires_bitmap(BYTE *p, unsigned int xs,
                                      unsigned int xe, BYTE *gfx_msk_ptr)
{
    BYTE *bmptr_low, *bmptr_high, *msk_ptr;
    BYTE bmval;
    unsigned int i, j;

    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    for (j = ((vicii.memptr << 3) + vicii.raster.ycounter + xs * 8) & 0x1fff, i = xs;
         i <= xe; i++, j = (j + 8) & 0x1fff) {
        BYTE fcolor;
        BYTE bcolor;

        if (j & 0x1000) {
            bmval = bmptr_high[j & 0xfff];
        } else {
            bmval = bmptr_low[j];
        }

        *(msk_ptr + i) = bmval;
        fcolor = vicii.dtvpalette[(vicii.vbuf[i] >> 4) & 0xf];
        bcolor = vicii.dtvpalette[vicii.vbuf[i] & 0xf];
        *(p + i * 8 + 0) = (bmval & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (bmval & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (bmval & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (bmval & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (bmval & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (bmval & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (bmval & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (bmval & 0x01) ? fcolor : bcolor;
    }
}

inline static void _draw_hires_bitmap_cached(BYTE *p, unsigned int xs,
                                             unsigned int xe,
                                             raster_cache_t *cache)
{
    BYTE *foreground_data, *background_data, *msk_ptr;
    unsigned int i;

    BYTE bmval;
    BYTE fcolor;
    BYTE bcolor;

    foreground_data = cache->foreground_data;
    background_data = cache->background_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    for (i = xs; i <= xe; i++) {
        *(msk_ptr + i) = foreground_data[i];
        bmval = foreground_data[i];
        fcolor = vicii.dtvpalette[(background_data[i] >> 4) & 0xf];
        bcolor = vicii.dtvpalette[background_data[i] & 0xf];
        *(p + i * 8 + 0) = (bmval & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (bmval & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (bmval & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (bmval & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (bmval & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (bmval & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (bmval & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (bmval & 0x01) ? fcolor : bcolor;
    }
}

static void draw_hires_bitmap(void)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap, 0, VICII_SCREEN_TEXTCOLS - 1,
                    vicii.raster.gfx_msk);
}

static void draw_hires_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                     unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap_cached, xs, xe, cache);
}

inline static void _draw_hires_bitmap_foreground(BYTE *p, unsigned int xs,
                                                 unsigned int xe,
                                                 BYTE *gfx_msk_ptr)
{
    BYTE *bmptr_low, *bmptr_high, *msk_ptr;
    BYTE bmval;
    unsigned int i, j;

    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    for (j = ((vicii.memptr << 3) + vicii.raster.ycounter + xs * 8) & 0x1fff, i = xs;
         i <= xe; i++, j = (j + 8) & 0x1fff) {
        BYTE fcolor;
        BYTE bcolor;

        if (vicii.raster.last_video_mode == VICII_ILLEGAL_BITMAP_MODE_1) {
            j &= 0x19ff;
        }

        if (j & 0x1000) {
            bmval = bmptr_high[j & 0xfff];
        } else {
            bmval = bmptr_low[j];
        }

        if (vicii.raster.last_video_mode == VICII_NORMAL_TEXT_MODE) {
            BYTE *char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
            bmval = char_ptr[vicii.vbuf[i - vicii.buf_offset] * 8];
        }

        *(msk_ptr + i) = bmval;

        fcolor = vicii.dtvpalette[(vicii.vbuf[i] >> 4) & 0xf];
        bcolor = vicii.dtvpalette[vicii.vbuf[i] & 0xf];
        *(p + i * 8 + 0) = (bmval & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (bmval & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (bmval & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (bmval & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (bmval & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (bmval & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (bmval & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (bmval & 0x01) ? fcolor : bcolor;
    }
}

static void draw_hires_bitmap_foreground(unsigned int start_char,
                                         unsigned int end_char)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap_foreground, start_char, end_char,
                    vicii.raster.gfx_msk);
}

/* Multicolor text mode.  */

static int get_mc_text(raster_cache_t *cache, unsigned int *xs,
                       unsigned int *xe, int rr)
{
    int r;

    if (cache->background_data[0] != vicii.raster.background_color
        || cache->color_data_1[0] != vicii.ext_background_color[0]
        || cache->color_data_1[1] != vicii.ext_background_color[1]
        || cache->chargen_ptr != vicii.chargen_ptr) {
        cache->background_data[0] = vicii.raster.background_color;
        cache->color_data_1[0] = vicii.ext_background_color[0];
        cache->color_data_1[1] = vicii.ext_background_color[1];
        cache->chargen_ptr = vicii.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text(cache->foreground_data,
                                    vicii.vbuf,
                                    vicii.chargen_ptr + vicii.raster.ycounter,
                                    VICII_SCREEN_TEXTCOLS,
                                    xs, xe,
                                    rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_mc_text(BYTE *p, unsigned int xs, unsigned int xe,
                                 BYTE *gfx_msk_ptr)
{
    BYTE c[12];
    BYTE *char_ptr, *msk_ptr;
    WORD *ptmp;
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    c[1] = c[0] = vicii.raster.background_color;
    c[3] = c[2] = vicii.ext_background_color[0];
    c[5] = c[4] = vicii.ext_background_color[1];
    c[11] = c[8] = vicii.raster.background_color;

    ptmp = (WORD *)(p + xs * 8);

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = (*(char_ptr + vicii.vbuf[i] * 8))
            | ((vicii.cbuf[i] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
        c[10] = c[9] = c[7] = c[6] = vicii.dtvpalette[vicii.cbuf[i] & cmask & 0xf7];
        ptmp[0] = ((WORD *)c)[mc_table[d]];
        ptmp[1] = ((WORD *)c)[mc_table[0x200 + d]];
        ptmp[2] = ((WORD *)c)[mc_table[0x400 + d]];
        ptmp[3] = ((WORD *)c)[mc_table[0x600 + d]];
        ptmp += 4;
    }
}

inline static void _draw_mc_text_cached(BYTE *p, unsigned int xs,
                                        unsigned int xe, raster_cache_t *cache)
{
    BYTE c[12];
    BYTE *foreground_data, *color_data_3, *msk_ptr;
    WORD *ptmp;
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    foreground_data = cache->foreground_data;
    color_data_3 = cache->color_data_3;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    c[1] = c[0] = cache->background_data[0];
    c[3] = c[2] = cache->color_data_1[0];
    c[5] = c[4] = cache->color_data_1[1];
    c[11] = c[8] = cache->background_data[0];

    ptmp = (WORD *)(p + xs * 8);

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = foreground_data[i] | ((color_data_3[i] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
        c[10] = c[9] = c[7] = c[6] = vicii.dtvpalette[color_data_3[i] & cmask & 0xf7];
        ptmp[0] = ((WORD *)c)[mc_table[d]];
        ptmp[1] = ((WORD *)c)[mc_table[0x200 + d]];
        ptmp[2] = ((WORD *)c)[mc_table[0x400 + d]];
        ptmp[3] = ((WORD *)c)[mc_table[0x600 + d]];
        ptmp += 4;
    }
}

static void draw_mc_text(void)
{
    ALIGN_DRAW_FUNC(_draw_mc_text, 0, VICII_SCREEN_TEXTCOLS - 1,
                    vicii.raster.gfx_msk);
}

static void draw_mc_text_cached(raster_cache_t *cache, unsigned int xs,
                                unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_mc_text_cached, xs, xe, cache);
}

/* FIXME: aligned/unaligned versions.  */
#define DRAW_MC_BYTE(p, b, f1, f2, f3)          \
    do {                                        \
        if ((b) & 0x80) {                       \
            if ((b) & 0x40) {                   \
                *(p) = *((p) + 1) = (f3);       \
            } else {                            \
                *(p) = *((p) + 1) = (f2);       \
            }                                   \
        } else if ((b) & 0x40) {                \
            *(p) = *((p) + 1) = (f1);           \
        }                                       \
                                                \
        if ((b) & 0x20) {                       \
            if ((b) & 0x10) {                   \
                *((p) + 2) = *((p) + 3) = (f3); \
            } else {                            \
                *((p) + 2) = *((p) + 3) = (f2); \
            }                                   \
        } else if ((b) & 0x10) {                \
            *((p) + 2) = *((p) + 3) = (f1);     \
        }                                       \
                                                \
        if ((b) & 0x08) {                       \
            if ((b) & 0x04) {                   \
                *((p) + 4) = *((p) + 5) = (f3); \
            } else {                            \
                *((p) + 4) = *((p) + 5) = (f2); \
            }                                   \
        } else if ((b) & 0x04) {                \
            *((p) + 4) = *((p) + 5) = (f1);     \
        }                                       \
                                                \
        if ((b) & 0x02) {                       \
            if ((b) & 0x01) {                   \
                *((p) + 6) = *((p) + 7) = (f3); \
            } else {                            \
                *((p) + 6) = *((p) + 7) = (f2); \
            }                                   \
        } else if ((b) & 0x01) {                \
            *((p) + 6) = *((p) + 7) = (f1);     \
        }                                       \
    } while (0)

static void draw_mc_text_foreground(unsigned int start_char, unsigned int end_char)
{
    BYTE *char_ptr, *msk_ptr;
    BYTE c1, c2;
    BYTE *p;
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = vicii.raster.gfx_msk + GFX_MSK_LEFTBORDER_SIZE;
    c1 = vicii.ext_background_color[0];
    c2 = vicii.ext_background_color[1];
    p = GFX_PTR() + 8 * start_char;

    for (i = start_char; i <= end_char; i++, p += 8) {
        BYTE b, c;

        c = vicii.cbuf[i - vicii.buf_offset];
        if (vicii.raster.last_video_mode == VICII_MULTICOLOR_BITMAP_MODE) {
            unsigned int j = ((vicii.memptr << 3)
                              + vicii.raster.ycounter + i * 8) & 0x1fff;
            if (j & 0x1000) {
                b = vicii.bitmap_high_ptr[j & 0xfff];
            } else {
                b = vicii.bitmap_low_ptr[j];
            }
        } else {
            b = *(char_ptr + vicii.vbuf[i - vicii.buf_offset] * 8);
        }

        if (c & 0x8) {
            BYTE c3;
            BYTE orig_background = *p;

            c3 = vicii.dtvpalette[c & cmask & 0xf7];
            DRAW_MC_BYTE(p, b, c1, c2, c3);
            *(msk_ptr + i) = mcmsktable[0x100 + b];

            if (vicii.raster.xsmooth_shift_left > 0) {
                int j;

                for (j = 0; j < vicii.raster.xsmooth_shift_left; j++) {
                    *(p + 7 - j) = orig_background;
                }

                *(msk_ptr + i) = (BYTE)((mcmsktable[0x100 + b]
                                         >> vicii.raster.xsmooth_shift_left)
                                        << vicii.raster.xsmooth_shift_left);
            }
        } else {
            BYTE c3;

            if (vicii.raster.xsmooth_shift_left > 0) {
                b = (b >> vicii.raster.xsmooth_shift_left)
                    << vicii.raster.xsmooth_shift_left;
            }

            c3 = c;
            c3 = vicii.dtvpalette[c3 & cmask];
            DRAW_STD_TEXT_BYTE(p, b, c3);
            *(msk_ptr + i) = b;
        }
    }
}

/* Multicolor Bitmap Mode.  */

static int get_mc_bitmap(raster_cache_t *cache, unsigned int *xs,
                         unsigned int *xe, int rr)
{
    int r;

    if (cache->background_data[0] != vicii.raster.background_color) {
        cache->background_data[0] = vicii.raster.background_color;
        rr = 1;
    }

    r = raster_cache_data_fill(cache->color_data_1,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_1fff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     8 * vicii.memptr + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_mc_bitmap(BYTE *p, unsigned int xs, unsigned int xe,
                                   BYTE *gfx_msk_ptr)
{
    BYTE *colptr, *bmptr_low, *bmptr_high, *msk_ptr, *ptmp;
    BYTE c[4];
    unsigned int i, j;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    colptr = vicii.cbuf;
    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    c[0] = vicii.raster.background_color;

    ptmp = p + xs * 8;

    for (j = ((vicii.memptr << 3) + vicii.raster.ycounter + xs * 8) & 0x1fff,
         i = xs; i <= xe; i++, j = (j + 8) & 0x1fff) {
        unsigned int d;

        if (j & 0x1000) {
            d = bmptr_high[j & 0xfff];
        } else {
            d = bmptr_low[j];
        }

        *(msk_ptr + i) = mcmsktable[d | 0x100];
        c[1] = vicii.vbuf[i] >> 4;
        c[2] = vicii.vbuf[i] & 0xf;
        c[3] = colptr[i];

        c[1] = vicii.dtvpalette[c[1]];
        c[2] = vicii.dtvpalette[c[2]];
        c[3] = vicii.dtvpalette[c[3] & cmask];

        ptmp[1] = ptmp[0] = c[mc_table[0x100 + d]];
        ptmp[3] = ptmp[2] = c[mc_table[0x300 + d]];
        ptmp[5] = ptmp[4] = c[mc_table[0x500 + d]];
        ptmp[7] = ptmp[6] = c[mc_table[0x700 + d]];
        ptmp += 8;
    }
}

inline static void _draw_mc_bitmap_cached(BYTE *p, unsigned int xs,
                                          unsigned int xe,
                                          raster_cache_t *cache)
{
    BYTE *foreground_data, *color_data_1, *color_data_3;
    BYTE *msk_ptr, *ptmp;
    BYTE c[4];
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    foreground_data = cache->foreground_data;
    color_data_1 = cache->color_data_1;
    color_data_3 = cache->color_data_3;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    c[0] = cache->background_data[0];

    ptmp = p + xs * 8;

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = foreground_data[i];

        *(msk_ptr + i) = mcmsktable[d | 0x100];

        c[1] = color_data_1[i] >> 4;
        c[2] = color_data_1[i] & 0xf;
        c[3] = color_data_3[i];

        c[1] = vicii.dtvpalette[c[1]];
        c[2] = vicii.dtvpalette[c[2]];
        c[3] = vicii.dtvpalette[c[3] & cmask];

        ptmp[1] = ptmp[0] = c[mc_table[0x100 + d]];
        ptmp[3] = ptmp[2] = c[mc_table[0x300 + d]];
        ptmp[5] = ptmp[4] = c[mc_table[0x500 + d]];
        ptmp[7] = ptmp[6] = c[mc_table[0x700 + d]];
        ptmp += 8;
    }
}

static void draw_mc_bitmap(void)
{
    _draw_mc_bitmap(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                    vicii.raster.gfx_msk);
}

static void draw_mc_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                  unsigned int xe)
{
    _draw_mc_bitmap_cached(GFX_PTR(), xs, xe, cache);
}

static void draw_mc_bitmap_foreground(unsigned int start_char,
                                      unsigned int end_char)
{
    BYTE *p, *bmptr_low, *bmptr_high, *msk_ptr;
    unsigned int i, j;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    p = GFX_PTR() + 8 * start_char;
    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = vicii.raster.gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    for (j = ((vicii.memptr << 3)
              + vicii.raster.ycounter + 8 * start_char) & 0x1fff,
         i = start_char; i <= end_char; j = (j + 8) & 0x1fff, i++, p += 8) {
        BYTE c1, c2, c3;
        BYTE b;
        BYTE orig_background = *p;

        c1 = vicii.vbuf[i - vicii.buf_offset] >> 4;
        c2 = vicii.vbuf[i - vicii.buf_offset] & 0xf;
        c3 = vicii.cbuf[i - vicii.buf_offset];

        c1 = vicii.dtvpalette[c1];
        c2 = vicii.dtvpalette[c2];
        c3 = vicii.dtvpalette[c3 & cmask];

        if (vicii.raster.last_video_mode == VICII_ILLEGAL_BITMAP_MODE_2) {
            j &= 0x19ff;
        }

        if (j & 0x1000) {
            b = bmptr_high[j & 0xfff];
        } else {
            b = bmptr_low[j];
        }

        if (vicii.raster.last_video_mode == VICII_MULTICOLOR_TEXT_MODE) {
            BYTE *char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
            b = char_ptr[vicii.vbuf[i - vicii.buf_offset] * 8];
        }

        *(msk_ptr + i) = mcmsktable[0x100 + b];
        DRAW_MC_BYTE(p, b, c1, c2, c3);

        if (vicii.raster.xsmooth_shift_left > 0) {
            int j;

            for (j = 0; j < vicii.raster.xsmooth_shift_left; j++) {
                *(p + 7 - j) = orig_background;
            }

            *(msk_ptr + i) = (BYTE)((mcmsktable[0x100 + b]
                                     >> vicii.raster.xsmooth_shift_left)
                                    << vicii.raster.xsmooth_shift_left);
        }
    }
}

/* Extended Text Mode.  */

static int get_ext_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    int r;

    if (cache->color_data_2[0] != vicii.raster.background_color
        || cache->color_data_2[1] != vicii.ext_background_color[0]
        || cache->color_data_2[2] != vicii.ext_background_color[1]
        || cache->color_data_2[3] != vicii.ext_background_color[2]
        || cache->chargen_ptr != vicii.chargen_ptr) {
        cache->color_data_2[0] = vicii.raster.background_color;
        cache->color_data_2[1] = vicii.ext_background_color[0];
        cache->color_data_2[2] = vicii.ext_background_color[1];
        cache->color_data_2[3] = vicii.ext_background_color[2];
        cache->chargen_ptr = vicii.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text_ext(cache->foreground_data,
                                        cache->color_data_3,
                                        vicii.vbuf,
                                        vicii.chargen_ptr,
                                        8,
                                        VICII_SCREEN_TEXTCOLS,
                                        vicii.raster.ycounter,
                                        xs, xe,
                                        rr);

    r |= raster_cache_data_fill(cache->color_data_1,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_ext_text(BYTE *p, unsigned int xs, unsigned int xe,
                                  BYTE *gfx_msk_ptr)
{
    BYTE *char_ptr, *msk_ptr;
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    for (i = xs; i <= xe; i++) {
        int bg_idx;
        int d;

        BYTE fcolor;
        BYTE bcolor;

        bg_idx = vicii.vbuf[i] >> 6;
        d = *(char_ptr + (vicii.vbuf[i] & 0x3f) * 8);

        fcolor = vicii.dtvpalette[vicii.cbuf[i] & cmask];
        bcolor = (bg_idx == 0) ? (BYTE)vicii.raster.background_color : vicii.ext_background_color[bg_idx - 1];
        *(msk_ptr + i) = d;
        *(p + i * 8 + 0) = (d & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (d & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (d & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (d & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (d & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (d & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (d & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (d & 0x01) ? fcolor : bcolor;
    }
}

inline static void _draw_ext_text_cached(BYTE *p, unsigned int xs,
                                         unsigned int xe,
                                         raster_cache_t *cache)
{
    BYTE *foreground_data, *color_data_1, *color_data_2, *color_data_3;
    BYTE *msk_ptr;
    unsigned int i;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    foreground_data = cache->foreground_data;
    color_data_1 = cache->color_data_1;
    color_data_2 = cache->color_data_2;
    color_data_3 = cache->color_data_3;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    for (i = xs; i <= xe; i++) {
        DWORD *ptr;
        int d;

        BYTE fcolor;
        BYTE bcolor;

        ptr = hr_table + (color_data_1[i] << 8);
        d = foreground_data[i];

        ptr += color_data_2[color_data_3[i]] << 4;
        *(msk_ptr + i) = d;

        fcolor = vicii.dtvpalette[color_data_1[i] & cmask];
        bcolor = color_data_2[color_data_3[i]];
        *(p + i * 8 + 0) = (d & 0x80) ? fcolor : bcolor;
        *(p + i * 8 + 1) = (d & 0x40) ? fcolor : bcolor;
        *(p + i * 8 + 2) = (d & 0x20) ? fcolor : bcolor;
        *(p + i * 8 + 3) = (d & 0x10) ? fcolor : bcolor;
        *(p + i * 8 + 4) = (d & 0x08) ? fcolor : bcolor;
        *(p + i * 8 + 5) = (d & 0x04) ? fcolor : bcolor;
        *(p + i * 8 + 6) = (d & 0x02) ? fcolor : bcolor;
        *(p + i * 8 + 7) = (d & 0x01) ? fcolor : bcolor;
    }
}

static void draw_ext_text(void)
{
    ALIGN_DRAW_FUNC(_draw_ext_text, 0, VICII_SCREEN_TEXTCOLS - 1,
                    vicii.raster.gfx_msk);
}

static void draw_ext_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_ext_text_cached, xs, xe, cache);
}

static void draw_ext_text_foreground(unsigned int start_char,
                                     unsigned int end_char)
{
    unsigned int i;
    BYTE *char_ptr, *msk_ptr, *p;
    BYTE cmask;
    cmask = (vicii.high_color) ? 0xff : 0x0f;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = vicii.raster.gfx_msk + GFX_MSK_LEFTBORDER_SIZE;
    p = GFX_PTR() + 8 * start_char;

    for (i = start_char; i <= end_char; i++, p += 8) {
        BYTE b, f;
        int bg_idx;

        b = char_ptr[(vicii.vbuf[i - vicii.buf_offset] & 0x3f) * 8];

        if (vicii.raster.last_video_mode == VICII_ILLEGAL_BITMAP_MODE_1) {
            unsigned int j = ((vicii.memptr << 3) + vicii.raster.ycounter + i * 8) & 0x19ff;
            if (j & 0x1000) {
                b = vicii.bitmap_high_ptr[j & 0xfff];
            } else {
                b = vicii.bitmap_low_ptr[j];
            }
        }

        f = vicii.cbuf[i - vicii.buf_offset];
        f = vicii.dtvpalette[f & cmask];
        bg_idx = vicii.vbuf[i - vicii.buf_offset] >> 6;

        if (vicii.raster.xsmooth_shift_left > 0) {
            b = (b >> vicii.raster.xsmooth_shift_left) << vicii.raster.xsmooth_shift_left;
        }

        if (bg_idx > 0) {
            p[7] = p[6] = p[5] = p[4] = p[3] = p[2] = p[1] = p[0] = vicii.ext_background_color[bg_idx - 1];
        }

        *(msk_ptr + i) = b;
        DRAW_STD_TEXT_BYTE(p, b, f);
    }
}

/* Illegal text mode.  */

static int get_illegal_text(raster_cache_t *cache, unsigned int *xs,
                            unsigned int *xe, int rr)
{
    int r;

    if (cache->chargen_ptr != vicii.chargen_ptr) {
        cache->chargen_ptr = vicii.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text_ext(cache->foreground_data,
                                        cache->color_data_3,
                                        vicii.vbuf,
                                        vicii.chargen_ptr,
                                        8,
                                        VICII_SCREEN_TEXTCOLS,
                                        vicii.raster.ycounter,
                                        xs, xe,
                                        rr);

    r |= raster_cache_data_fill(cache->color_data_1,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_illegal_text(BYTE *p, unsigned int xs,
                                      unsigned int xe, BYTE *gfx_msk_ptr)
{
    BYTE *char_ptr, *msk_ptr;
    unsigned int i;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = (*(char_ptr + (vicii.vbuf[i] & 0x3f) * 8))
            | ((vicii.cbuf[i] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
    }
}

inline static void _draw_illegal_text_cached(BYTE *p, unsigned int xs,
                                             unsigned int xe,
                                             raster_cache_t *cache)
{
    BYTE *foreground_data, *color_data_1, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    color_data_1 = cache->color_data_1;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = foreground_data[i] | ((color_data_1[i] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
    }
}

static void draw_illegal_text(void)
{
    _draw_illegal_text(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                       vicii.raster.gfx_msk);
}

static void draw_illegal_text_cached(raster_cache_t *cache, unsigned int xs,
                                     unsigned int xe)
{
    _draw_illegal_text_cached(GFX_PTR(), xs, xe, cache);
}

static void draw_illegal_text_foreground(unsigned int start_char,
                                         unsigned int end_char)
{
    BYTE *char_ptr, *msk_ptr, *p;
    unsigned int i;

    char_ptr = vicii.chargen_ptr + vicii.raster.ycounter;
    msk_ptr = vicii.raster.gfx_msk + GFX_MSK_LEFTBORDER_SIZE;
    p = GFX_PTR() + 8 * start_char;

    memset(p, 0, (end_char - start_char + 1) * 8);

    for (i = start_char; i <= end_char; i++) {
        unsigned int d;

        d = (*(char_ptr + (vicii.vbuf[i - vicii.buf_offset] & 0x3f) * 8))
            | ((vicii.cbuf[i - vicii.buf_offset] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
    }
}

/* Illegal bitmap mode 1.  */

static int get_illegal_bitmap_mode1(raster_cache_t *cache, unsigned int *xs,
                                    unsigned int *xe, int rr)
{
    int r;

    r = raster_cache_data_fill(cache->background_data,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_illegal_bitmap_mode1(BYTE *p, unsigned int xs,
                                              unsigned int xe,
                                              BYTE *gfx_msk_ptr)
{
    BYTE *bmptr_low, *bmptr_high, *msk_ptr;
    BYTE bmval;
    unsigned int i, j;

    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (j = ((vicii.memptr << 3) + vicii.raster.ycounter + xs * 8) & 0x1fff, i = xs;
         i <= xe; i++, j = (j + 8) & 0x1fff) {
        if (j & 0x1000) {
            bmval = bmptr_low[j & 0x9ff];
        } else {
            bmval = bmptr_high[j & 0x9ff];
        }

        *(msk_ptr + i) = bmval;
    }
}

inline static void _draw_illegal_bitmap_mode1_cached(BYTE *p, unsigned int xs,
                                                     unsigned int xe,
                                                     raster_cache_t *cache)
{
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++) {
        *(msk_ptr + i) = foreground_data[i];
    }
}

static void draw_illegal_bitmap_mode1(void)
{
    _draw_illegal_bitmap_mode1(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                               vicii.raster.gfx_msk);
}

static void draw_illegal_bitmap_mode1_cached(raster_cache_t *cache,
                                             unsigned int xs, unsigned int xe)
{
    _draw_illegal_bitmap_mode1_cached(GFX_PTR(), xs, xe, cache);
}

static void draw_illegal_bitmap_mode1_foreground(unsigned int start_char,
                                                 unsigned int end_char)
{
    _draw_illegal_bitmap_mode1(GFX_PTR(), start_char, end_char,
                               vicii.raster.gfx_msk);
}

/* Illegal bitmap mode 2.  */

static int get_illegal_bitmap_mode2(raster_cache_t *cache, unsigned int *xs,
                                    unsigned int *xe, int rr)
{
    int r;

    r = raster_cache_data_fill(cache->color_data_1,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_illegal_bitmap_mode2(BYTE *p, unsigned int xs,
                                              unsigned int xe,
                                              BYTE *gfx_msk_ptr)
{
    BYTE *bmptr_low, *bmptr_high, *msk_ptr;
    BYTE bmval;
    unsigned int i, j;

    bmptr_low = vicii.bitmap_low_ptr;
    bmptr_high = vicii.bitmap_high_ptr;
    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (j = ((vicii.memptr << 3) + vicii.raster.ycounter + xs * 8) & 0x1fff, i = xs;
         i <= xe; i++, j = (j + 8) & 0x1fff) {
        if (j & 0x1000) {
            bmval = bmptr_high[j & 0x9ff];
        } else {
            bmval = bmptr_low[j & 0x9ff];
        }

        *(msk_ptr + i) = mcmsktable[bmval | 0x100];
    }
}

inline static void _draw_illegal_bitmap_mode2_cached(BYTE *p, unsigned int xs,
                                                     unsigned int xe,
                                                     raster_cache_t *cache)
{
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++) {
        *(msk_ptr + i) = mcmsktable[foreground_data[i] | 0x100];
    }
}

static void draw_illegal_bitmap_mode2(void)
{
    _draw_illegal_bitmap_mode2(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                               vicii.raster.gfx_msk);
}

static void draw_illegal_bitmap_mode2_cached(raster_cache_t *cache,
                                             unsigned int xs, unsigned int xe)
{
    _draw_illegal_bitmap_mode2_cached(GFX_PTR(), xs, xe, cache);
}

static void draw_illegal_bitmap_mode2_foreground(unsigned int start_char,
                                                 unsigned int end_char)
{
    _draw_illegal_bitmap_mode2(GFX_PTR(), start_char, end_char,
                               vicii.raster.gfx_msk);
}

/* Idle state.  */

static int get_idle(raster_cache_t *cache, unsigned int *xs, unsigned int *xe,
                    int rr)
{
    if (rr
        || cache->foreground_data[0] != vicii.idle_data
        || cache->color_data_1[0] != vicii.raster.background_color
        || cache->color_data_1[1] != vicii.raster.idle_background_color
        || cache->color_data_1[2] != vicii.raster.video_mode) {
        cache->foreground_data[0] = (BYTE)vicii.idle_data;
        cache->color_data_1[0] = vicii.raster.background_color;
        cache->color_data_1[1] = vicii.raster.idle_background_color;
        cache->color_data_1[2] = vicii.raster.video_mode;
        *xs = 0;
        if (!vicii.overscan) {
            *xe = VICII_SCREEN_TEXTCOLS - 1;
        } else {
            *xe = VICII_SCREEN_TEXTCOLS + 8 - 1;
        }
        return 1;
    } else {
        return 0;
    }
}

inline static void _draw_idle(BYTE *p, unsigned int xs, unsigned int xe,
                              BYTE *gfx_msk_ptr)
{
    BYTE *msk_ptr;
    unsigned int i;

    msk_ptr = gfx_msk_ptr + GFX_MSK_LEFTBORDER_SIZE;

    if (vicii.raster.video_mode == VICII_8BPP_CHUNKY_MODE) {
        BYTE *dest_ptr;

        for (i = 0; i < 4; i++) {
            vicii.idle_data_m[i] = *(((vicii.regs[0x45] & 0x1f) << 16) + vicii.screen_ptr + 0x03fc + i);
        }

        dest_ptr = p + 8 * xs;
        for (i = xs; i <= xe; i++) {
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[3]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[0]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[1]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[2]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[3]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[0]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[1]];
            *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[2]];
        }
        memset(msk_ptr + xs, 0xff, xe + 1 - xs); /* fix me! */
    } else {
        memset(p + xs * 8, vicii.raster.idle_background_color, (xe + 1 - xs) * 8);
        memset(msk_ptr + xs, 0, xe + 1 - xs);
    }
}

static void draw_idle(void)
{
    ALIGN_DRAW_FUNC(_draw_idle, 0, vicii.raster.geometry->text_size.width - 1,
                    vicii.raster.gfx_msk);
}

static void draw_idle_cached(raster_cache_t *cache, unsigned int xs,
                             unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_idle, xs, xe, cache->gfx_msk);
}

static void draw_idle_foreground(unsigned int start_char,
                                 unsigned int end_char)
{
    _draw_idle(GFX_PTR(), start_char, end_char, vicii.raster.gfx_msk);
}

/* 8BPP Chunky bitmap mode  */

static int get_8bpp_chunky_bitmap(raster_cache_t *cache, unsigned int *xs,
                                  unsigned int *xe, int rr)
{
/* TODO */
/*
    int r;

    if (cache->chargen_ptr != vicii.chargen_ptr) {
        cache->chargen_ptr = vicii.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text_ext(cache->foreground_data,
                                        cache->color_data_3,
                                        vicii.vbuf,
                                        vicii.chargen_ptr,
                                        8,
                                        VICII_SCREEN_TEXTCOLS,
                                        vicii.raster.ycounter,
                                        xs, xe,
                                        rr);

    r |= raster_cache_data_fill(cache->color_data_1,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
*/
    return 0;
}

inline static void _draw_8bpp_chunky_bitmap(BYTE *p, unsigned int xs,
                                            unsigned int xe, BYTE *gfx_msk_ptr)
{
    BYTE *countb_ptr, *dest_ptr;
    unsigned int i;

    countb_ptr = mem_ram + vicii.countb + xs * vicii.countb_step;

    /* fetch idle data (this should really be made somewhere better) */
    for (i = 0; i < 4; i++) {
        vicii.idle_data_m[i] = *(((vicii.regs[0x45] & 0x1f) << 16) + vicii.screen_ptr + 0x03fc + i);
    }

    /* skip off-screen overscan region by adding an offset */
    if (vicii.overscan) {
        countb_ptr += 0x18;
    }

    dest_ptr = p + 8 * xs;

    for (i = xs; i <= xe; i++) {
        if (i < 45) {
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 0)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 1)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 2)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 3)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 4)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 5)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 6)];
            *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 7)];
            countb_ptr += vicii.countb_step;
        } else {
            if (vicii.raster.ycounter != 7) {
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 0)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 1)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 2)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 3)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 4)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 5)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 6)];
                *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 7)];
            } else {
                if (i == 45) {
                    *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 0)];
                    *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 1)];
                    *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 2)];
                    *(dest_ptr++) = vicii.dtvpalette[*(countb_ptr + 3)];
                } else {
                    *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[3]];
                    *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[0]];
                    *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[1]];
                    *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[2]];
                }
                *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[3]];
                *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[0]];
                *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[1]];
                *(dest_ptr++) = vicii.dtvpalette[vicii.idle_data_m[2]];
            }
        }
    }
}

inline static void _draw_8bpp_chunky_bitmap_cached(BYTE *p, unsigned int xs,
                                                   unsigned int xe,
                                                   raster_cache_t *cache)
{
/* TODO */
/*
    BYTE *foreground_data, *color_data_1, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    color_data_1 = cache->color_data_1;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++) {
        unsigned int d;

        d = foreground_data[i] | ((color_data_1[i] & 0x8) << 5);

        *(msk_ptr + i) = mcmsktable[d];
    }
*/
}

static void draw_8bpp_chunky_bitmap(void)
{
    _draw_8bpp_chunky_bitmap(GFX_PTR(), 0, vicii.raster.geometry->text_size.width - 1,
                             vicii.raster.gfx_msk);
}

static void draw_8bpp_chunky_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                           unsigned int xe)
{
/*
    _draw_8bpp_chunky_bitmap_cached(GFX_PTR(), xs, xe, cache);
*/
    _draw_8bpp_chunky_bitmap(GFX_PTR(), 0, vicii.raster.geometry->text_size.width - 1,
                             vicii.raster.gfx_msk);
}

static void draw_8bpp_chunky_bitmap_foreground(unsigned int start_char,
                                               unsigned int end_char)
{
    _draw_8bpp_chunky_bitmap(GFX_PTR(), start_char, end_char, vicii.raster.gfx_msk);
}

/* 8BPP Two Plane bitmap mode  */

static int get_8bpp_two_plane_bitmap_mode(raster_cache_t *cache, unsigned int *xs,
                                          unsigned int *xe, int rr)
{
/* TODO */
/*
    int r;

    r = raster_cache_data_fill(cache->background_data,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
*/
    return 0;
}

inline static void _draw_8bpp_two_plane_bitmap_mode(BYTE *p, unsigned int xs,
                                                    unsigned int xe,
                                                    BYTE *gfx_msk_ptr)
{
    BYTE *countb_ptr, *counta_ptr, *dest_ptr, c, pa, pb;
    unsigned int i;
    BYTE ctab[4];

    countb_ptr = mem_ram + vicii.countb + xs * vicii.countb_step;

    counta_ptr = mem_ram + vicii.counta + xs * vicii.counta_step;

    dest_ptr = p + 8 * xs;

    ctab[0] = vicii.raster.background_color;
    ctab[3] = vicii.ext_background_color[0];
    for (i = xs; i <= xe; i++) {
        ctab[1] = vicii.dtvpalette[vicii.cbuf[i] & 0x0f];
        ctab[2] = vicii.dtvpalette[vicii.cbuf[i] >> 4];
        pa = *(counta_ptr);
        pb = *(countb_ptr);

        c = ((pb & 0x80) >> 6) | ((pa & 0x80) >> 7);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x40) >> 5) | ((pa & 0x40) >> 6);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x20) >> 4) | ((pa & 0x20) >> 5);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x10) >> 3) | ((pa & 0x10) >> 4);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x08) >> 2) | ((pa & 0x08) >> 3);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x04) >> 1) | ((pa & 0x04) >> 2);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x02) >> 0) | ((pa & 0x02) >> 1);
        *(dest_ptr++) = ctab[c];
        c = ((pb & 0x01) << 1) | ((pa & 0x01) >> 0);
        *(dest_ptr++) = ctab[c];

        countb_ptr += vicii.countb_step;
        counta_ptr += vicii.counta_step;
    }
}

inline static void _draw_8bpp_two_plane_bitmap_mode_cached(BYTE *p, unsigned int xs,
                                                           unsigned int xe,
                                                           raster_cache_t *cache)
{
/* TODO */
/*
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++)
        *(msk_ptr + i) = foreground_data[i];
*/
}

static void draw_8bpp_two_plane_bitmap_mode(void)
{
    _draw_8bpp_two_plane_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                     vicii.raster.gfx_msk);
}

static void draw_8bpp_two_plane_bitmap_mode_cached(raster_cache_t *cache, unsigned int xs, unsigned int xe)
{
/*
    _draw_8bpp_two_plane_bitmap_mode_cached(GFX_PTR(), xs, xe, cache);
*/
    _draw_8bpp_two_plane_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                     vicii.raster.gfx_msk);
}

static void draw_8bpp_two_plane_bitmap_mode_foreground(unsigned int start_char, unsigned int end_char)
{
    _draw_8bpp_two_plane_bitmap_mode(GFX_PTR(), start_char, end_char,
                                     vicii.raster.gfx_msk);
}

/* 8BPP FRED bitmap mode  */

static int get_8bpp_fred_bitmap_mode(raster_cache_t *cache, unsigned int *xs,
                                     unsigned int *xe, int rr)
{
/* TODO */
/*
    int r;

    r = raster_cache_data_fill(cache->color_data_1,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
*/
    return 0;
}

inline static void _draw_8bpp_fred_bitmap_mode(BYTE *p, unsigned int xs,
                                               unsigned int xe,
                                               BYTE *gfx_msk_ptr)
{
    BYTE *countb_ptr, *counta_ptr, *dest_ptr, c, cr, pa, pb;
    unsigned int i;

    countb_ptr = mem_ram + vicii.countb + xs * vicii.countb_step;

    counta_ptr = mem_ram + vicii.counta + xs * vicii.counta_step;

    dest_ptr = p + 8 * xs;

    for (i = xs; i <= xe; i++) {
        cr = vicii.cbuf[i] << 4;
        pa = *(counta_ptr);
        pb = *(countb_ptr);
        c = vicii.dtvpalette[cr | ((pb & 0xc0) >> 4) | ((pa & 0xc0) >> 6)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | ((pb & 0xc0) >> 4) | ((pa & 0xc0) >> 6)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | ((pb & 0xc0) >> 4) | ((pa & 0xc0) >> 6)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | ((pb & 0xc0) >> 4) | ((pa & 0xc0) >> 6)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        countb_ptr += vicii.countb_step;
        counta_ptr += vicii.counta_step;
    }
}

inline static void _draw_8bpp_fred_bitmap_mode_cached(BYTE *p, unsigned int xs,
                                                      unsigned int xe,
                                                      raster_cache_t *cache)
{
/* TODO */
/*
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++)
        *(msk_ptr + i) = mcmsktable[foreground_data[i] | 0x100];
*/
}

static void draw_8bpp_fred_bitmap_mode(void)
{
    _draw_8bpp_fred_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                vicii.raster.gfx_msk);
}

static void draw_8bpp_fred_bitmap_mode_cached(raster_cache_t *cache,
                                              unsigned int xs, unsigned int xe)
{
/*
    _draw_8bpp_fred_bitmap_mode_cached(GFX_PTR(), xs, xe, cache);
*/
    _draw_8bpp_fred_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                vicii.raster.gfx_msk);
}

static void draw_8bpp_fred_bitmap_mode_foreground(unsigned int start_char,
                                                  unsigned int end_char)
{
    _draw_8bpp_fred_bitmap_mode(GFX_PTR(), start_char, end_char,
                                vicii.raster.gfx_msk);
}

/* 8BPP FRED2 bitmap mode  */

static int get_8bpp_fred2_bitmap_mode(raster_cache_t *cache, unsigned int *xs,
                                      unsigned int *xe, int rr)
{
/* TODO */
/*
    int r;

    r = raster_cache_data_fill(cache->color_data_1,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
*/
    return 0;
}

inline static void _draw_8bpp_fred2_bitmap_mode(BYTE *p, unsigned int xs,
                                                unsigned int xe,
                                                BYTE *gfx_msk_ptr)
{
/* TODO this function is a HACK */
    BYTE *countb_ptr, *counta_ptr, *dest_ptr, c, cr, pa, pb;
    unsigned int i;

    countb_ptr = mem_ram
                 + vicii.countb
                 + xs;

    counta_ptr = mem_ram
                 + vicii.counta
                 + xs;

    dest_ptr = p + 8 * xs;

    for (i = xs; i <= xe; i++) {
        cr = (vicii.cbuf[i] & 0xc << 2) | (vicii.cbuf[i] & 0x3);
        pa = *(counta_ptr);
        pb = *(countb_ptr);
        c = vicii.dtvpalette[cr | (pb & 0xc0) | ((pa & 0xc0) >> 4)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | (pb & 0xc0) | ((pa & 0xc0) >> 4)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | (pb & 0xc0) | ((pa & 0xc0) >> 4)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        pb <<= 2;
        pa <<= 2;
        c = vicii.dtvpalette[cr | (pb & 0xc0) | ((pa & 0xc0) >> 4)];
        *(dest_ptr++) = c;
        *(dest_ptr++) = c;
        countb_ptr += vicii.countb_step;
        counta_ptr += vicii.counta_step;
    }
}

inline static void _draw_8bpp_fred2_bitmap_mode_cached(BYTE *p, unsigned int xs,
                                                       unsigned int xe,
                                                       raster_cache_t *cache)
{
/* TODO */
/*
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++)
        *(msk_ptr + i) = mcmsktable[foreground_data[i] | 0x100];
*/
}

static void draw_8bpp_fred2_bitmap_mode(void)
{
    _draw_8bpp_fred2_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                 vicii.raster.gfx_msk);
}

static void draw_8bpp_fred2_bitmap_mode_cached(raster_cache_t *cache,
                                               unsigned int xs, unsigned int xe)
{
/*
    _draw_8bpp_fred2_bitmap_mode_cached(GFX_PTR(), xs, xe, cache);
*/
    _draw_8bpp_fred2_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                 vicii.raster.gfx_msk);
}

static void draw_8bpp_fred2_bitmap_mode_foreground(unsigned int start_char,
                                                   unsigned int end_char)
{
    _draw_8bpp_fred2_bitmap_mode(GFX_PTR(), start_char, end_char,
                                 vicii.raster.gfx_msk);
}

/* 8BPP pixel cell mode  */

static int get_8bpp_pixel_cell_bitmap_mode(raster_cache_t *cache, unsigned int *xs,
                                           unsigned int *xe, int rr)
{
/* TODO */
/*
    int r;

    r = raster_cache_data_fill(cache->color_data_1,
                               vicii.vbuf,
                               VICII_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                vicii.cbuf,
                                VICII_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_39ff(cache->foreground_data,
                                     vicii.bitmap_low_ptr,
                                     vicii.bitmap_high_ptr,
                                     vicii.memptr * 8
                                     + vicii.raster.ycounter,
                                     VICII_SCREEN_TEXTCOLS,
                                     8,
                                     xs, xe,
                                     rr);
    return r;
*/
    return 0;
}

inline static void _draw_8bpp_pixel_cell_bitmap_mode(BYTE *p, unsigned int xs,
                                                     unsigned int xe,
                                                     BYTE *gfx_msk_ptr)
{
    BYTE *c_ptr, *countb_ptr, *dest_ptr;
    unsigned int i;
/*
    BYTE *counta_ptr;
    counta_ptr = mem_ram
                 + (vicii.counta & 0x1fc000)
                 + vicii.mem_counter; // TODO broken in last ~3 lines
*/
    countb_ptr = mem_ram + (vicii.countb & 0x1fc000) + vicii.raster.ycounter * 8;

    dest_ptr = p + 8 * xs;

    for (i = xs; i <= xe; i++) {
        c_ptr = countb_ptr + vicii.vbuf[i] * 64; /* (*counta_ptr)*64; */
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        *(dest_ptr++) = vicii.dtvpalette[*(c_ptr++)];
        /* counta_ptr += vicii.counta_step; */
    }
}

inline static void _draw_8bpp_pixel_cell_bitmap_mode_cached(BYTE *p, unsigned int xs,
                                                            unsigned int xe,
                                                            raster_cache_t *cache)
{
/* TODO */
/*
    BYTE *foreground_data, *msk_ptr;
    unsigned int i;

    foreground_data = cache->foreground_data;
    msk_ptr = cache->gfx_msk + GFX_MSK_LEFTBORDER_SIZE;

    memset(p + 8 * xs, 0, (xe - xs + 1) * 8);

    for (i = xs; i <= xe; i++)
        *(msk_ptr + i) = mcmsktable[foreground_data[i] | 0x100];
*/
}

static void draw_8bpp_pixel_cell_bitmap_mode(void)
{
    _draw_8bpp_pixel_cell_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                      vicii.raster.gfx_msk);
}

static void draw_8bpp_pixel_cell_bitmap_mode_cached(raster_cache_t *cache,
                                                    unsigned int xs, unsigned int xe)
{
/*
    _draw_8bpp_pixel_cell_bitmap_mode_cached(GFX_PTR(), xs, xe, cache);
*/
    _draw_8bpp_pixel_cell_bitmap_mode(GFX_PTR(), 0, VICII_SCREEN_TEXTCOLS - 1,
                                      vicii.raster.gfx_msk);
}

static void draw_8bpp_pixel_cell_bitmap_mode_foreground(unsigned int start_char,
                                                        unsigned int end_char)
{
    _draw_8bpp_pixel_cell_bitmap_mode(GFX_PTR(), start_char, end_char,
                                      vicii.raster.gfx_msk);
}

static void setup_modes(void)
{
    raster_modes_set(vicii.raster.modes, VICII_NORMAL_TEXT_MODE,
                     get_std_text,
                     draw_std_text_cached,
                     draw_std_text,
                     draw_std_background,
                     draw_std_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_MULTICOLOR_TEXT_MODE,
                     get_mc_text,
                     draw_mc_text_cached,
                     draw_mc_text,
                     draw_std_background,
                     draw_mc_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_HIRES_BITMAP_MODE,
                     get_hires_bitmap,
                     draw_hires_bitmap_cached,
                     draw_hires_bitmap,
                     draw_std_background,
                     draw_hires_bitmap_foreground);

    raster_modes_set(vicii.raster.modes, VICII_MULTICOLOR_BITMAP_MODE,
                     get_mc_bitmap,
                     draw_mc_bitmap_cached,
                     draw_mc_bitmap,
                     draw_std_background,
                     draw_mc_bitmap_foreground);

    raster_modes_set(vicii.raster.modes, VICII_EXTENDED_TEXT_MODE,
                     get_ext_text,
                     draw_ext_text_cached,
                     draw_ext_text,
                     draw_std_background,
                     draw_ext_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_IDLE_MODE,
                     get_idle,
                     draw_idle_cached,
                     draw_idle,
                     draw_idle_std_background,
                     draw_idle_foreground);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_TEXT_MODE,
                     get_illegal_text,
                     draw_illegal_text_cached,
                     draw_illegal_text,
                     draw_std_background,
                     draw_illegal_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_BITMAP_MODE_1,
                     get_illegal_bitmap_mode1,
                     draw_illegal_bitmap_mode1_cached,
                     draw_illegal_bitmap_mode1,
                     draw_std_background,
                     draw_illegal_bitmap_mode1_foreground);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_BITMAP_MODE_2,
                     get_illegal_bitmap_mode2,
                     draw_illegal_bitmap_mode2_cached,
                     draw_illegal_bitmap_mode2,
                     draw_std_background,
                     draw_illegal_bitmap_mode2_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_NORMAL_TEXT_MODE,
                     get_std_text,
                     draw_std_text_cached,
                     draw_std_text,
                     draw_std_background,
                     draw_std_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_MULTICOLOR_TEXT_MODE,
                     get_mc_text,
                     draw_mc_text_cached,
                     draw_mc_text,
                     draw_std_background,
                     draw_mc_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_HIRES_BITMAP_MODE,
                     get_hires_bitmap,
                     draw_hires_bitmap_cached,
                     draw_hires_bitmap,
                     draw_std_background,
                     draw_hires_bitmap_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_MULTICOLOR_BITMAP_MODE,
                     get_mc_bitmap,
                     draw_mc_bitmap_cached,
                     draw_mc_bitmap,
                     draw_std_background,
                     draw_mc_bitmap_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_EXTENDED_TEXT_MODE,
                     get_ext_text,
                     draw_ext_text_cached,
                     draw_ext_text,
                     draw_std_background,
                     draw_ext_text_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_CHUNKY_MODE,
                     get_8bpp_chunky_bitmap,
                     draw_8bpp_chunky_bitmap_cached,
                     draw_8bpp_chunky_bitmap,
                     draw_std_background,
                     draw_8bpp_chunky_bitmap_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_TWO_PLANE_BITMAP_MODE,
                     get_8bpp_two_plane_bitmap_mode,
                     draw_8bpp_two_plane_bitmap_mode_cached,
                     draw_8bpp_two_plane_bitmap_mode,
                     draw_std_background,
                     draw_8bpp_two_plane_bitmap_mode_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_FRED_MODE,
                     get_8bpp_fred_bitmap_mode,
                     draw_8bpp_fred_bitmap_mode_cached,
                     draw_8bpp_fred_bitmap_mode,
                     draw_std_background,
                     draw_8bpp_fred_bitmap_mode_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_FRED2_MODE,
                     get_8bpp_fred2_bitmap_mode,
                     draw_8bpp_fred2_bitmap_mode_cached,
                     draw_8bpp_fred2_bitmap_mode,
                     draw_std_background,
                     draw_8bpp_fred2_bitmap_mode_foreground);

    raster_modes_set(vicii.raster.modes, VICII_8BPP_PIXEL_CELL_MODE,
                     get_8bpp_pixel_cell_bitmap_mode,
                     draw_8bpp_pixel_cell_bitmap_mode_cached,
                     draw_8bpp_pixel_cell_bitmap_mode,
                     draw_std_background,
                     draw_8bpp_pixel_cell_bitmap_mode_foreground);

    raster_modes_set(vicii.raster.modes, VICII_ILLEGAL_LINEAR_MODE,
                     get_illegal_text,
                     draw_illegal_text_cached,
                     draw_illegal_text,
                     draw_std_background,
                     draw_illegal_text_foreground);
}

/* Initialize the drawing tables.  */
static void init_drawing_tables(void)
{
    DWORD i;
    unsigned int f, b;
    const BYTE tmptable[4] = { 0, 4, 5, 3 };

    for (i = 0; i <= 0xf; i++) {
        for (f = 0; f <= 0xf; f++) {
            for (b = 0; b <= 0xf; b++) {
                BYTE fp, bp;
                BYTE *p;
                int offset;

                fp = f;
                bp = b;
                offset = (f << 8) | (b << 4);
                p = (BYTE *)(hr_table + offset + i);

                *p = i & 0x8 ? fp : bp;
                *(p + 1) = i & 0x4 ? fp : bp;
                *(p + 2) = i & 0x2 ? fp : bp;
                *(p + 3) = i & 0x1 ? fp : bp;
            }
        }
    }

    for (i = 0; i <= 0xff; i++) {
        mc_table[i + 0x100] = (BYTE)(i >> 6);
        mc_table[i + 0x300] = (BYTE)((i >> 4) & 0x3);
        mc_table[i + 0x500] = (BYTE)((i >> 2) & 0x3);
        mc_table[i + 0x700] = (BYTE)(i & 0x3);
        mc_table[i] = tmptable[i >> 6];
        mc_table[i + 0x200] = tmptable[(i >> 4) & 0x3];
        mc_table[i + 0x400] = tmptable[(i >> 2) & 0x3];
        mc_table[i + 0x600] = tmptable[i & 0x3];
        mcmsktable[i + 0x100] = (BYTE)0;
        mcmsktable[i + 0x100] |= (BYTE)(((i >> 6) & 0x2) ? 0xc0 : 0);
        mcmsktable[i + 0x100] |= (BYTE)(((i >> 4) & 0x2) ? 0x30 : 0);
        mcmsktable[i + 0x100] |= (BYTE)(((i >> 2) & 0x2) ? 0x0c : 0);
        mcmsktable[i + 0x100] |= (BYTE)((i & 0x2) ? 0x03 : 0);
        mcmsktable[i] = (BYTE)i;
    }
}

void vicii_draw_init(void)
{
    init_drawing_tables();

    setup_modes();
}
