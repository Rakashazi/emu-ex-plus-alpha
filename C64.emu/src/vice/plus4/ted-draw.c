/*
 * ted-draw.c - Rendering for the TED emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Tibor Biczo <crown@axelero.hu>
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

#include "raster-cache-fill.h"
#include "raster-cache-fill-1fff.h"
#include "raster-cache-nibbles.h"
#include "raster-cache-text-ext.h"
#include "raster-cache-text-std.h"
#include "raster-cache.h"
#include "raster-modes.h"
#include "ted-draw.h"
#include "ted.h"
#include "tedtypes.h"
#include "types.h"


/* The following tables are used to speed up the drawing.  We do not use
   multi-dimensional arrays as we can optimize better this way...  */

/* foreground(7) | background(7) | nibble(4) -> 4 pixels.  */
static DWORD hr_table[128 * 128 * 16];

/* mc flag(1) | idx(2) | byte(8) -> index into double-pixel table.  */
static BYTE mc_table[2 * 4 * 256];

/* These functions draw the background from `start_pixel' to `end_pixel'.  */

static void draw_std_background(unsigned int start_pixel,
                                unsigned int end_pixel)
{
    memset(ted.raster.draw_buffer_ptr + start_pixel,
           ted.raster.idle_background_color,
           end_pixel - start_pixel + 1);
}

/* If unaligned 32-bit access is not allowed, the graphics is stored in a
   temporary aligned buffer, and later copied to the real frame buffer.  This
   is ugly, but should be hopefully faster than accessing 8 bits at a time
   anyway.  */

#ifndef ALLOW_UNALIGNED_ACCESS
static DWORD _aligned_line_buffer[TED_SCREEN_XPIX / 2 + 1];
static BYTE *const aligned_line_buffer = (BYTE *)_aligned_line_buffer;
#endif

/* Pointer to the start of the graphics area on the frame buffer.  */
#define GFX_PTR()               \
    (ted.raster.draw_buffer_ptr \
     + (ted.screen_leftborderwidth + ted.raster.xsmooth))

#ifdef ALLOW_UNALIGNED_ACCESS
#define ALIGN_DRAW_FUNC(name, xs, xe) \
    name(GFX_PTR(), (xs), (xe))
#else
#define ALIGN_DRAW_FUNC(name, xs, xe)          \
    do {                                       \
        name(aligned_line_buffer, (xs), (xe)); \
        memcpy(GFX_PTR() + (xs) * 8,           \
               aligned_line_buffer + (xs) * 8, \
               ((xe) - (xs) + 1) * 8);         \
    } while (0)
#endif

#ifdef ALLOW_UNALIGNED_ACCESS
#define ALIGN_DRAW_FUNC_CACHE(name, xs, xe, cache_ptr) \
    name(GFX_PTR(), (xs), (xe), (cache_ptr))
#else
#define ALIGN_DRAW_FUNC_CACHE(name, xs, xe, cache_ptr)      \
    do {                                                    \
        name(aligned_line_buffer, (xs), (xe), (cache_ptr)); \
        memcpy(GFX_PTR() + (xs) * 8,                        \
               aligned_line_buffer + (xs) * 8,              \
               ((xe) - (xs) + 1) * 8);                      \
    } while (0)
#endif

/*-----------------------------------------------------------------------*/

inline static BYTE get_char_data(BYTE c, BYTE col, int l, BYTE *char_mem,
                                 int bytes_per_char, int curpos, int index)
{
    BYTE data;

    if ((col & 0x80) && (!ted.cursor_visible)) {
        data = 0;
    } else {
        if (!ted.reverse_mode && (c & 0x80)) {
            data = char_mem[((c & 0x7f) * bytes_per_char) + (l)] ^ 0xff;
        } else {
            data = char_mem[((c) * bytes_per_char) + (l)];
        }
    }


    if (curpos == index) {
        data ^= 0xff;
    }

    return data;
}

inline static int cache_data_fill_text(BYTE *dest,
                                       const BYTE *src,
                                       const BYTE *src2,
                                       BYTE *char_mem,
                                       int bytes_per_char,
                                       unsigned int length,
                                       int l,
                                       unsigned int *xs,
                                       unsigned int *xe,
                                       int no_check,
                                       int curpos)
{
    unsigned int i;

    if (no_check) {
        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++, src++, src2++) {
            dest[i] = get_char_data(src[0], src2[0], l, char_mem,
                                    bytes_per_char, curpos, i);
        }
        return 1;
    } else {
        BYTE b;

        for (i = 0;
             i < length && dest[i] == get_char_data(src[0], src2[0], l, char_mem,
                                                    bytes_per_char, curpos, i);
             i++, src++, src2++) {
            /* do nothing */
        }

        if (i < length) {
            *xs = *xe = i;

            for (; i < length; i++, src++, src2++) {
                if (dest[i] != (b = get_char_data(src[0], src2[0], l, char_mem,
                                                  bytes_per_char, curpos, i))) {
                    dest[i] = b;
                    *xe = i;
                }
            }

            return 1;
        } else {
            return 0;
        }
    }
}

/*-----------------------------------------------------------------------*/

/* FIXME: in the cache, we store the foreground bitmap values for the
   characters, but we do not use them when drawing and this is slow!  */

/* Standard text mode.  */

static int get_std_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    int r, cursor_pos = -1;

    if (ted.raster.background_color != cache->background_data[0]
        || cache->chargen_ptr != ted.chargen_ptr) {
        cache->background_data[0] = ted.raster.background_color;
        cache->chargen_ptr = ted.chargen_ptr;
        rr = 1;
    }

    if (ted.cursor_visible) {
        int crsrpos = ted.crsrpos - ted.memptr;
        if (crsrpos >= 0 && crsrpos < TED_SCREEN_TEXTCOLS) {
            cursor_pos = crsrpos;
        }
    }

    r = cache_data_fill_text(cache->foreground_data,
                             ted.vbuf,
                             ted.cbuf,
                             ted.chargen_ptr,
                             8,   /* FIXME */
                             TED_SCREEN_TEXTCOLS,
                             ted.raster.ycounter,
                             xs, xe,
                             rr,
                             cursor_pos);
    r |= raster_cache_data_fill(cache->color_data_1,
                                ted.cbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill(cache->color_data_2,
                                ted.vbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);



    return r;
}

/* without video cache */
inline static void _draw_std_text(BYTE *p, unsigned int xs, unsigned int xe)
{
    DWORD *table_ptr;
    BYTE *char_ptr;
    unsigned int i;
    int cursor_pos = -1;

    table_ptr = hr_table + (ted.raster.background_color << 4);
    char_ptr = ted.chargen_ptr + ted.raster.ycounter;

    if (ted.cursor_visible) {
        int crsrpos = ted.crsrpos - ted.memptr;
        if (crsrpos >= 0 && crsrpos < TED_SCREEN_TEXTCOLS) {
            cursor_pos = crsrpos;
        }
    }

    if (ted.reverse_mode) {
        for (i = xs; i <= xe; i++) {
            int d;
            DWORD *ptr = table_ptr + ((ted.cbuf[i] & 0x7f) << 11);

            if ((ted.cbuf[i] & 0x80) && (!ted.cursor_visible)) {
                d = 0;
            } else {
                d = *(char_ptr + ted.vbuf[i] * 8);
            }
            if ((int)i == cursor_pos) {
                d ^= 0xff;
            }
            *((DWORD *)p + i * 2) = *(ptr + (d >> 4));
            *((DWORD *)p + i * 2 + 1) = *(ptr + (d & 0xf));
        }
    } else {
        for (i = xs; i <= xe; i++) {
            int d;
            DWORD *ptr = table_ptr + ((ted.cbuf[i] & 0x7f) << 11);

            if ((ted.cbuf[i] & 0x80) && (!ted.cursor_visible)) {
                d = (ted.vbuf[i] & 0x80 ? 0xff : 0x00);
            } else {
                d = *(char_ptr + (ted.vbuf[i] & 0x7f) * 8)
                    ^ (ted.vbuf[i] & 0x80 ? 0xff : 0x00);
            }
            if ((int)i == cursor_pos) {
                d ^= 0xff;
            }
            *((DWORD *)p + i * 2) = *(ptr + (d >> 4));
            *((DWORD *)p + i * 2 + 1) = *(ptr + (d & 0xf));
        }
    }
}

static void draw_std_text(void)
{
    ALIGN_DRAW_FUNC(_draw_std_text, 0, TED_SCREEN_TEXTCOLS - 1);
}

/* with video cache */
inline static void _draw_std_text_cached(BYTE *p, unsigned int xs,
                                         unsigned int xe,
                                         raster_cache_t *cache)
{
    DWORD *table_ptr;
    BYTE *foreground_data, *color_data, *vbuf;
    unsigned int i;

    table_ptr = hr_table + (cache->background_data[0] << 4);
    foreground_data = cache->foreground_data; /* contains both vbuf and cbuf */
    color_data = cache->color_data_1;
    vbuf = cache->color_data_2;

    for (i = xs; i <= xe; i++) {
        int d;
        DWORD *ptr = table_ptr + ((color_data[i] & 0x7f) << 11);

        if ((color_data[i] & 0x80) && (!ted.cursor_visible)) {
            d = (vbuf[i] & 0x80 ? 0xff : 0x00);
        } else {
            d = foreground_data[i];
        }

        *((DWORD *)p + i * 2) = *(ptr + (d >> 4));
        *((DWORD *)p + i * 2 + 1) = *(ptr + (d & 0xf));
    }
}

static void draw_std_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    ALIGN_DRAW_FUNC_CACHE(_draw_std_text_cached, xs, xe, cache);
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
    BYTE *char_ptr;
    BYTE *p;
    int cursor_pos = -1;

    char_ptr = ted.chargen_ptr + ted.raster.ycounter;
    p = GFX_PTR() + 8 * start_char;

    if (ted.cursor_visible) {
        int crsrpos = ted.crsrpos - ted.memptr;
        if (crsrpos >= 0 && crsrpos < TED_SCREEN_TEXTCOLS) {
            cursor_pos = crsrpos;
        }
    }

    if (ted.reverse_mode) {
        for (i = start_char; i <= end_char; i++, p += 8) {
            BYTE b, f;

            if ((ted.cbuf[i] & 0x80) && (!ted.cursor_visible)) {
                b = 0;
            } else {
                b = char_ptr[ted.vbuf[i] * 8];
            }
            if ((int)i == cursor_pos) {
                b ^= 0xff;
            }
            f = ted.cbuf[i] & 0x7f;

            DRAW_STD_TEXT_BYTE(p, b, f);
        }
    } else {
        for (i = start_char; i <= end_char; i++, p += 8) {
            BYTE b, f;

            if ((ted.cbuf[i] & 0x80) && (!ted.cursor_visible)) {
                b = (ted.vbuf[i] & 0x80 ? 0xff : 0x00);
            } else {
                b = char_ptr[(ted.vbuf[i] & 0x7f) * 8]
                    ^ (ted.vbuf[i] & 0x80 ? 0xff : 0x00);
            }
            if ((int)i == cursor_pos) {
                b ^= 0xff;
            }
            f = ted.cbuf[i] & 0x7f;

            DRAW_STD_TEXT_BYTE(p, b, f);
        }
    }
}

/*
    Hires Bitmap mode.
*/

static int get_hires_bitmap(raster_cache_t *cache, unsigned int *xs,
                            unsigned int *xe, int rr)
{
    int r;

    r = raster_cache_data_fill_nibbles(cache->color_data_1,
                                       cache->background_data,
                                       ted.vbuf,
                                       TED_SCREEN_TEXTCOLS,
                                       1,
                                       xs, xe,
                                       rr);
    r |= raster_cache_data_fill(cache->color_data_2,
                                ted.cbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_1fff(cache->foreground_data,
                                     ted.bitmap_ptr,
                                     ted.bitmap_ptr + 0x1000,
                                     ted.memptr * 8 + ted.raster.ycounter,
                                     TED_SCREEN_TEXTCOLS,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_hires_bitmap(BYTE *p, unsigned int xs,
                                      unsigned int xe)
{
    BYTE *bmptr;
    unsigned int i, j;
    DWORD *ptr;

    bmptr = ted.bitmap_ptr;

    for (j = ((ted.memptr << 3) + ted.raster.ycounter + xs * 8) & 0x1fff, i = xs;
         i <= xe; i++, j = (j + 8) & 0x1fff) {
        int d;

        ptr = hr_table
              + ((ted.cbuf[i] & 0x07) << 15) + ((ted.vbuf[i] & 0xf0) << 7)
              + ((ted.cbuf[i] & 0x70) << 4) + ((ted.vbuf[i] & 0x0f) << 4);

        d = bmptr[j];
        *((DWORD *)p + i * 2) = *(ptr + (d >> 4));
        *((DWORD *)p + i * 2 + 1) = *(ptr + (d & 0xf));
    }
}

static void draw_hires_bitmap(void)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap, 0, TED_SCREEN_TEXTCOLS - 1);

    /* Overscan color in HIRES is determined by last char of previous line */
    ted.raster.idle_background_color = ted.vbuf[TED_SCREEN_TEXTCOLS - 1] & 0x7f;
}

static void draw_hires_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                     unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap, xs, xe);

    /* Overscan color in HIRES is determined by last char of previous line */
    if (xe == TED_SCREEN_TEXTCOLS - 1) {
        ted.raster.idle_background_color = ted.vbuf[TED_SCREEN_TEXTCOLS - 1] & 0x7f;
    }
}

static void draw_hires_bitmap_foreground(unsigned int start_char,
                                         unsigned int end_char)
{
    ALIGN_DRAW_FUNC(_draw_hires_bitmap, start_char, end_char);
}

/*
    Multicolor text mode.
*/

static int get_mc_text(raster_cache_t *cache, unsigned int *xs,
                       unsigned int *xe, int rr)
{
    int r;

    if (ted.raster.background_color != cache->background_data[0]
        || cache->color_data_1[0] != ted.ext_background_color[0]
        || cache->color_data_1[1] != ted.ext_background_color[1]
        || cache->chargen_ptr != ted.chargen_ptr) {
        cache->background_data[0] = ted.raster.background_color;
        cache->color_data_1[0] = ted.ext_background_color[0];
        cache->color_data_1[1] = ted.ext_background_color[1];
        cache->chargen_ptr = ted.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text(cache->foreground_data,
                                    ted.vbuf,
                                    ted.chargen_ptr + ted.raster.ycounter,
                                    TED_SCREEN_TEXTCOLS,
                                    xs, xe,
                                    rr);
    r |= raster_cache_data_fill(cache->color_data_3,
                                ted.cbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_mc_text(BYTE *p, unsigned int xs, unsigned int xe)
{
    BYTE c[12];
    BYTE *char_ptr;
    WORD *ptmp;
    unsigned int i, v, d;

    char_ptr = ted.chargen_ptr + ted.raster.ycounter;

    c[1] = c[0] = ted.raster.background_color;
    c[3] = c[2] = ted.ext_background_color[0];
    c[5] = c[4] = ted.ext_background_color[1];
    c[11] = c[8] = ted.raster.background_color;

    ptmp = (WORD *)(p + xs * 8);
    for (i = xs; i <= xe; i++) {
/*         unsigned int d = (*(char_ptr + ted.vbuf[i] * 8))
                          | ((ted.cbuf[i] & 0x8) << 5); */
        v = ted.vbuf[i] & (ted.reverse_mode ? 0xff : 0x7f);
        d = char_ptr[v * 8] | ((ted.cbuf[i] & 0x8) << 5);

        c[10] = c[9] = c[7] = c[6] = ted.cbuf[i] & 0x77;

        ptmp[0] = ((WORD *)c)[mc_table[d]];
        ptmp[1] = ((WORD *)c)[mc_table[0x200 + d]];
        ptmp[2] = ((WORD *)c)[mc_table[0x400 + d]];
        ptmp[3] = ((WORD *)c)[mc_table[0x600 + d]];
        ptmp += 4;
    }
}

static void draw_mc_text(void)
{
    ALIGN_DRAW_FUNC(_draw_mc_text, 0, TED_SCREEN_TEXTCOLS - 1);
}

static void draw_mc_text_cached(raster_cache_t *cache, unsigned int xs,
                                unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_mc_text, xs, xe);
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
    BYTE *char_ptr;
    BYTE c1, c2;
    BYTE *p;
    unsigned int i;

    char_ptr = ted.chargen_ptr + ted.raster.ycounter;
    c1 = ted.ext_background_color[0];
    c2 = ted.ext_background_color[1];
    p = GFX_PTR() + 8 * start_char;

    for (i = start_char; i <= end_char; i++, p += 8) {
        BYTE b, c;

        b = *(char_ptr + ted.vbuf[i] * 8);
        c = ted.cbuf[i];

        if (c & 0x8) {
            BYTE c3;

            c3 = c & 0x77;
            DRAW_MC_BYTE (p, b, c1, c2, c3);
        } else {
            BYTE c3;

            c3 = c & 0x77;
            DRAW_STD_TEXT_BYTE(p, b, c3);
        }
    }
}

/* Multicolor Bitmap Mode.  */

static int get_mc_bitmap(raster_cache_t *cache, unsigned int *xs,
                         unsigned int *xe, int rr)
{
    int r;

    if (ted.raster.background_color != cache->background_data[0]
        || ted.ext_background_color[0] != cache->color_data_3[0]) {
        cache->background_data[0] = ted.raster.background_color;
        cache->color_data_3[0] = ted.ext_background_color[0];
        rr = 1;
    }

    r = raster_cache_data_fill(cache->color_data_1,
                               ted.vbuf,
                               TED_SCREEN_TEXTCOLS,
                               xs, xe,
                               rr);
    r |= raster_cache_data_fill(cache->color_data_2,
                                ted.cbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    r |= raster_cache_data_fill_1fff(cache->foreground_data,
                                     ted.bitmap_ptr,
                                     ted.bitmap_ptr + 0x1000,
                                     ted.memptr * 8 + ted.raster.ycounter,
                                     TED_SCREEN_TEXTCOLS,
                                     xs, xe,
                                     rr);
    return r;
}

inline static void _draw_mc_bitmap(BYTE *p, unsigned int xs, unsigned int xe)
{
    BYTE *bmptr, *ptmp;
    BYTE c[4];
    unsigned int i, j;

    bmptr = ted.bitmap_ptr;

    c[0] = ted.raster.background_color;
    c[3] = ted.ext_background_color[0];

    ptmp = p + xs * 8;
    for (j = ((ted.memptr << 3) + ted.raster.ycounter + xs * 8) & 0x1fff,
         i = xs; i <= xe; i++, j = (j + 8) & 0x1fff) {
        unsigned int d;

        d = bmptr[j];

        c[1] = (ted.vbuf[i] >> 4) + ((ted.cbuf[i] & 0x07) << 4);
        c[2] = (ted.vbuf[i] & 0x0f) + (ted.cbuf[i] & 0x70);

        ptmp[1] = ptmp[0] = c[mc_table[0x100 + d]];
        ptmp[3] = ptmp[2] = c[mc_table[0x300 + d]];
        ptmp[5] = ptmp[4] = c[mc_table[0x500 + d]];
        ptmp[7] = ptmp[6] = c[mc_table[0x700 + d]];
        ptmp += 8;
    }
}

static void draw_mc_bitmap(void)
{
    ALIGN_DRAW_FUNC(_draw_mc_bitmap, 0, TED_SCREEN_TEXTCOLS - 1);
}

static void draw_mc_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                  unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_mc_bitmap, xs, xe);
}

static void draw_mc_bitmap_foreground(unsigned int start_char,
                                      unsigned int end_char)
{
    BYTE *p;
    BYTE *bmptr;
    unsigned int i, j;

    p = GFX_PTR() + 8 * start_char;
    bmptr = ted.bitmap_ptr;

    for (j = ((ted.memptr << 3) + ted.raster.ycounter + 8 * start_char) & 0x1fff,
         i = start_char; i <= end_char; j = (j + 8) & 0x1fff, i++, p += 8) {
        BYTE c1, c2, c3;
        BYTE b;

        c1 = (ted.vbuf[i] >> 4) + ((ted.cbuf[i] & 0x07) << 4);
        c2 = (ted.vbuf[i] & 0x0f) + (ted.cbuf[i] & 0x70);
        c3 = ted.ext_background_color[0];
        b = bmptr[j];

        DRAW_MC_BYTE(p, b, c1, c2, c3);
    }
}

/* Extended Text Mode.  */

static int get_ext_text(raster_cache_t *cache, unsigned int *xs,
                        unsigned int *xe, int rr)
{
    int r;

    if (cache->color_data_2[0] != ted.raster.background_color
        || cache->color_data_2[1] != ted.ext_background_color[0]
        || cache->color_data_2[2] != ted.ext_background_color[1]
        || cache->color_data_2[3] != ted.ext_background_color[2]
        || cache->chargen_ptr != ted.chargen_ptr) {
        cache->color_data_2[0] = ted.raster.background_color;
        cache->color_data_2[1] = ted.ext_background_color[0];
        cache->color_data_2[2] = ted.ext_background_color[1];
        cache->color_data_2[3] = ted.ext_background_color[2];
        cache->chargen_ptr = ted.chargen_ptr;
        rr = 1;
    }

    r = raster_cache_data_fill_text_ext(cache->foreground_data,
                                        cache->color_data_3,
                                        ted.vbuf,
                                        ted.chargen_ptr,
                                        8,
                                        TED_SCREEN_TEXTCOLS,
                                        ted.raster.ycounter,
                                        xs, xe,
                                        rr);

    r |= raster_cache_data_fill(cache->color_data_1,
                                ted.cbuf,
                                TED_SCREEN_TEXTCOLS,
                                xs, xe,
                                rr);
    return r;
}

inline static void _draw_ext_text(BYTE *p, unsigned int xs, unsigned int xe)
{
    BYTE *char_ptr;
    unsigned int i;

    char_ptr = ted.chargen_ptr + ted.raster.ycounter;

    for (i = xs; i <= xe; i++) {
        DWORD *ptr;
        int bg_idx;
        int d;

        ptr = hr_table + ((ted.cbuf[i] & 0x7f) << 11);
        bg_idx = ted.vbuf[i] >> 6;
        d = *(char_ptr + (ted.vbuf[i] & 0x3f) * 8);

        if (bg_idx == 0) {
            ptr += ted.raster.background_color << 4;
        } else {
            ptr += ted.ext_background_color[bg_idx - 1] << 4;
        }

        *((DWORD *)p + 2 * i) = *(ptr + (d >> 4));
        *((DWORD *)p + 2 * i + 1) = *(ptr + (d & 0xf));
    }
}

static void draw_ext_text(void)
{
    ALIGN_DRAW_FUNC(_draw_ext_text, 0, TED_SCREEN_TEXTCOLS - 1);
}

static void draw_ext_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
{
    ALIGN_DRAW_FUNC(_draw_ext_text, xs, xe);
}

/* FIXME: This is *slow* and might not be 100% correct.  */
static void draw_ext_text_foreground(unsigned int start_char,
                                     unsigned int end_char)
{
    unsigned int i;
    BYTE *char_ptr;
    BYTE *p;

    char_ptr = ted.chargen_ptr + ted.raster.ycounter;
    p = GFX_PTR() + 8 * start_char;

    for (i = start_char; i <= end_char; i++, p += 8) {
        BYTE b;
        BYTE f;
        int bg_idx;

        b = char_ptr[(ted.vbuf[i] & 0x3f) * 8];
        f = ted.cbuf[i] & 0x7f;
        bg_idx = ted.vbuf[i] >> 6;

        if (bg_idx > 0) {
            p[7] = p[6] = p[5] = p[4] = p[3] = p[2] = p[1] = p[0] = ted.ext_background_color[bg_idx - 1];
        }

        DRAW_STD_TEXT_BYTE(p, b, f);
    }
}

/* Illegal mode.  Everything is black.  */

static int get_black(raster_cache_t *cache, unsigned int *xs,
                     unsigned int *xe, int r)
{
    /* Let's simplify here: if also the previous time we had the Black Mode,
       nothing has changed.  If we had not, the whole line has changed.  */

    if (r) {
        *xs = 0;
        *xe = TED_SCREEN_TEXTCOLS - 1;
    }

    return r;
}

static void draw_black(void)
{
    BYTE *p;

    p = GFX_PTR();

    memset(p, 0, TED_SCREEN_TEXTCOLS * 8);
}

static void draw_black_cached(raster_cache_t *cache, unsigned int xs,
                              unsigned int xe)
{
    BYTE *p;

    p = GFX_PTR();

    memset(p, 0, TED_SCREEN_TEXTCOLS * 8);
}

static void draw_black_foreground(unsigned int start_char,
                                  unsigned int end_char)
{
    BYTE *p;

    p = GFX_PTR() + 8 * start_char;

    memset(p, 0, (end_char - start_char + 1) * 8);
}


/* Idle state.  */

static int get_idle(raster_cache_t *cache, unsigned int *xs, unsigned int *xe,
                    int rr)
{
    if (rr
        || ted.raster.background_color != cache->color_data_1[0]
        || ted.idle_data != cache->foreground_data[0]) {
        cache->color_data_1[0] = ted.raster.background_color;
        cache->foreground_data[0] = (BYTE)ted.idle_data;
        *xs = 0;
        *xe = TED_SCREEN_TEXTCOLS - 1;
        return 1;
    } else {
        return 0;
    }
}

inline static void _draw_idle(unsigned int xs, unsigned int xe)
{
    BYTE *p;
    BYTE d = 0;
    unsigned int i;

    if (!ted.raster.blank_enabled) {
        d = (BYTE)ted.idle_data;
    }

#ifdef ALLOW_UNALIGNED_ACCESS
    p = GFX_PTR();
#else
    p = aligned_line_buffer;
#endif

    if (TED_IS_ILLEGAL_MODE(ted.raster.video_mode)) {
        memset(p, 0, TED_SCREEN_XPIX);
    } else {
        /* The foreground color is always black (0).  */
        unsigned int offs;
        DWORD c1, c2;

        offs = ted.raster.idle_background_color << 4;
        c1 = *(hr_table + offs + (d >> 4));
        c2 = *(hr_table + offs + (d & 0xf));

        for (i = xs * 8; i <= xe * 8; i += 8) {
            *((DWORD *)(p + i)) = c1;
            *((DWORD *)(p + i + 4)) = c2;
        }
    }

#ifndef ALLOW_UNALIGNED_ACCESS
    memcpy(GFX_PTR(), aligned_line_buffer + xs * 8, (xe - xs + 1) * 8);
#endif
}

static void draw_idle(void)
{
    _draw_idle(0, TED_SCREEN_TEXTCOLS - 1);
}

static void draw_idle_cached(raster_cache_t *cache, unsigned int xs,
                             unsigned int xe)
{
    _draw_idle(xs, xe);
}

static void draw_idle_foreground(unsigned int start_char,
                                 unsigned int end_char)
{
    BYTE *p;
    BYTE c;
    BYTE d = 0;
    unsigned int i;

    p = GFX_PTR();
    c = 0;
    if (!ted.raster.blank_enabled) {
        d = (BYTE)ted.idle_data;
    }

    for (i = start_char; i <= end_char; i++) {
        DRAW_STD_TEXT_BYTE(p + i * 8, d, c);
    }
}

static void setup_modes(void)
{
    raster_modes_set(ted.raster.modes, TED_NORMAL_TEXT_MODE,
                     get_std_text,
                     draw_std_text_cached,
                     draw_std_text,
                     draw_std_background,
                     draw_std_text_foreground);

    raster_modes_set(ted.raster.modes, TED_MULTICOLOR_TEXT_MODE,
                     get_mc_text,
                     draw_mc_text_cached,
                     draw_mc_text,
                     draw_std_background,
                     draw_mc_text_foreground);

    raster_modes_set(ted.raster.modes, TED_HIRES_BITMAP_MODE,
                     get_hires_bitmap,
                     draw_hires_bitmap_cached,
                     draw_hires_bitmap,
                     draw_std_background,
                     draw_hires_bitmap_foreground);

    raster_modes_set(ted.raster.modes, TED_MULTICOLOR_BITMAP_MODE,
                     get_mc_bitmap,
                     draw_mc_bitmap_cached,
                     draw_mc_bitmap,
                     draw_std_background,
                     draw_mc_bitmap_foreground);

    raster_modes_set(ted.raster.modes, TED_EXTENDED_TEXT_MODE,
                     get_ext_text,
                     draw_ext_text_cached,
                     draw_ext_text,
                     draw_std_background,
                     draw_ext_text_foreground);

    raster_modes_set(ted.raster.modes, TED_IDLE_MODE,
                     get_idle,
                     draw_idle_cached,
                     draw_idle,
                     draw_std_background,
                     draw_idle_foreground);

    raster_modes_set(ted.raster.modes, TED_ILLEGAL_TEXT_MODE,
                     get_black,
                     draw_black_cached,
                     draw_black,
                     draw_std_background,
                     draw_black_foreground);

    raster_modes_set(ted.raster.modes, TED_ILLEGAL_BITMAP_MODE_1,
                     get_black,
                     draw_black_cached,
                     draw_black,
                     draw_std_background,
                     draw_black_foreground);

    raster_modes_set(ted.raster.modes, TED_ILLEGAL_BITMAP_MODE_2,
                     get_black,
                     draw_black_cached,
                     draw_black,
                     draw_std_background,
                     draw_black_foreground);
}

/* Initialize the drawing tables.  */
static void init_drawing_tables(void)
{
    DWORD i;
    unsigned int f, b;
    const BYTE tmptable[4] = { 0, 4, 5, 3 };

    for (i = 0; i <= 0xf; i++) {
        for (f = 0; f <= 0x7f; f++) {
            for (b = 0; b <= 0x7f; b++) {
                BYTE fp, bp;
                BYTE *p;
                int offset;

                fp = f;
                bp = b;
                offset = (f << 11) | (b << 4);
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
    }
}

void ted_draw_init(void)
{
    init_drawing_tables();

    setup_modes();
}
