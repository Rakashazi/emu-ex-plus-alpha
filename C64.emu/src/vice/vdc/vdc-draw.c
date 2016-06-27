/*
 * vdc-draw.c - Rendering for the MOS 8563 (VDC) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Markus Brenner <markus@brenner.de>
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

#include "raster-cache-const.h"
#include "raster-cache-fill.h"
#include "raster-cache.h"
#include "raster-modes.h"
#include "types.h"
#include "vdc-draw.h"
#include "vdc-resources.h"
#include "vdc.h"
#include "vdctypes.h"

/* The following tables are used to speed up the drawing.  We do not use
   multi-dimensional arrays as we can optimize better this way...  */

/* foreground(4) | background(4) | nibble(4) -> 4 pixels.  */
static DWORD hr_table[16 * 16 * 16];

/* foreground(4) | background(4) | nibble(4) -> 8 pixels (double width)
   pdl table is the low (right) nibble into 4 bytes, pdh the high (left) */
static DWORD pdl_table[16 * 16 * 16];
static DWORD pdh_table[16 * 16 * 16];


/* solid cursor (no blink), no cursor, 1/16 refresh blink, 1/32 refresh blink */
static const BYTE crsrblink[4] = { 0x01, 0x00, 0x08, 0x10 };

/* These functions draw the background from `start_pixel' to `end_pixel'.  */
/*
static void draw_std_background(unsigned int start_pixel,
                                unsigned int end_pixel)
{
    memset(vdc.raster.draw_buffer_ptr + start_pixel,
           vdc.raster.idle_background_color,
           end_pixel - start_pixel + 1);
}
*/

/* Initialize the drawing tables.  */
static void init_drawing_tables(void)
{
    DWORD i;
    unsigned int f, b;

    for (i = 0; i <= 0xf; i++) {
        for (f = 0; f <= 0xf; f++) {
            for (b = 0; b <= 0xf; b++) {
                BYTE fp, bp;
                BYTE *p;
                int offset;

                fp = f;
                bp = b;
                offset = (f << 8) | (b << 4) | i;
                
                p = (BYTE *)(hr_table + offset);
                *p = i & 0x8 ? fp : bp;
                *(p + 1) = i & 0x4 ? fp : bp;
                *(p + 2) = i & 0x2 ? fp : bp;
                *(p + 3) = i & 0x1 ? fp : bp;

                p = (BYTE *)(pdh_table + offset);
                *p = i & 0x8 ? fp : bp;
                *(p + 1) = i & 0x8 ? fp : bp;
                *(p + 2) = i & 0x4 ? fp : bp;
                *(p + 3) = i & 0x4 ? fp : bp;

                p = (BYTE *)(pdl_table + offset);
                *p = i & 0x2 ? fp : bp;
                *(p + 1) = i & 0x2 ? fp : bp;
                *(p + 2) = i & 0x1 ? fp : bp;
                *(p + 3) = i & 0x1 ? fp : bp;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

inline static BYTE get_attr_char_data(BYTE c, BYTE a, int l, BYTE *char_mem,
                                      int bytes_per_char, int blink,
                                      int revers, int curpos, int index)
/* c = character, a = attributes, l = line of char required , char_mem = pointer to character memory
bytes_per_char = number of bytes per char definition
blink / revers - these don't seem to be used
curpos = where in the screen memory the cursor is
index = where in the screen memory we are, to check against the cursor */
{
    BYTE data;
    /* bit mask used for register 22, to determine the number of pixels in a char actually displayed */
    /* The 8th value is 0x00 (no pixels) instead of 0xFF (all pixels) as might be expected as this results in blank chars on real VDC */
    static const BYTE mask[16] = {
        0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static const BYTE semigfxtest[16] = {
        0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const BYTE semigfxmask[16] = {
        0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (a & VDC_ALTCHARSET_ATTR) {
        /* swich to alternate charset if appropriate attribute bit set */
        char_mem += 0x1000;
    }

    if (l > (signed)vdc.regs[23]) {
        /* Return nothing if > Vertical Character Size */
        data = 0x00;
    } else {
        /* mask against r[22] - pixels per char mask */
        data = char_mem[(c * bytes_per_char) + l] & mask[vdc.regs[22] & 0x0F];
    }

    if ((l == (signed)vdc.regs[29]) && (a & VDC_UNDERLINE_ATTR)) {
        /* TODO - figure out if the pixels per char applies to the underline */
        data = 0xFF;
    }

    if ((a & VDC_FLASH_ATTR) && (vdc.attribute_blink)) {
        /* underline byte also blinks! */
        data = 0x00;
    }

    if (vdc.regs[25] & 0x20) {
        /* Semi-graphics mode */
        if (data & semigfxtest[vdc.regs[22] & 0x0F]) {
            /* if the far right pixel is on.. */
            data |= semigfxmask[vdc.regs[22] & 0x0F];
            /* .. mask the rest of the right hand side on */
        }
    }

    if (a & VDC_REVERSE_ATTR) {
        /* reverse attribute set */
        data ^= 0xFF;
    }

    if (vdc.regs[24] & 0x40) {
        /* Reverse screen bit */
        data ^= 0xFF;
    }

    /* on a 80x25 text screen (2000 characters) this is only true for 1 character. */
    if (curpos == index) {
        /* invert anything at all? */
        if ((vdc.frame_counter | 1) & crsrblink[(vdc.regs[10] >> 5) & 3]) {
            /* invert current byte of the character? */
            if ((l >= (signed)(vdc.regs[10] & 0x1F)) && (l < (signed)(vdc.regs[11] & 0x1F))) {
                /* The VDC cursor reverses the char */
                data ^= 0xFF;
            }
        }
    }

    return data;
}

inline static int cache_data_fill_attr_text(BYTE *dest,
                                            const BYTE *src,
                                            BYTE *attr,
                                            BYTE *char_mem,
                                            int bytes_per_char,
                                            unsigned int length,
                                            int l,
                                            unsigned int *xs,
                                            unsigned int *xe,
                                            int no_check,
                                            int blink,
                                            int revers,
                                            int curpos)
{
    unsigned int i;

    /* Fill (*dest) with (length) bytes of character data from (*src)
    - VDC screen memory - using attributes (*attr) - VDC attribute memory -,
    at vertical character reference (l) from VDC memory character set
    (*char_mem) */

    if (no_check) {
        /* fill *dest regardless of any changes */
        /* xs/xe seem to the start & end of any data that was updated/filled */
        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++, src++, attr++) {
            dest[i] = get_attr_char_data(src[0], attr[0], l, char_mem,
                                         bytes_per_char, blink, revers,
                                         curpos, i);
        }
        /* dest data was updated */
        return 1;
    } else {
        BYTE b;
        /* compare destination to data to look for any differences */
        for (i = 0; i < length; i++, src++, attr++) {
            if (dest[i] != get_attr_char_data(src[0], attr[0], l, char_mem,
                                              bytes_per_char, blink, revers, curpos, i)) {
                break;
            }
        }
        if (i < length) {
            /* we found a difference */
            *xs = *xe = i;
            /* xs/xe set start/end address for modified data */
            /* compare & update *dest, adjusting *xe if needed */
            for (; i < length; i++, src++, attr++) {
                b = get_attr_char_data(src[0], attr[0], l, char_mem,
                                       bytes_per_char, blink, revers,
                                       curpos, i);
                if (dest[i] != b) {
                    dest[i] = b;
                    *xe = i;
                }
            }
            /* dest data was updated */
            return 1;
        } else {
            /* nothing changed from last run */
            return 0;
        }
    }
}

inline static int cache_data_fill_attr_text_const(BYTE *dest,
                                                  const BYTE *src,
                                                  BYTE attr,
                                                  BYTE *char_mem,
                                                  int bytes_per_char,
                                                  unsigned int length,
                                                  int l,
                                                  unsigned int *xs,
                                                  unsigned int *xe,
                                                  int no_check,
                                                  int blink,
                                                  int revers,
                                                  int curpos)
{
    unsigned int i;

    /* This route is basically identical to cache_data_fill_attr_text(),
       except that "attr" is a constant value rather than a pointer into VDC attribute memory,
       so every character byte has the same attributes. */

    if (no_check) {
        /* fill *dest regardless of any changes */
        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++, src++) {
            dest[i] = get_attr_char_data(src[0], attr, l, char_mem,
                                         bytes_per_char, blink, revers,
                                         curpos, i);
        }
        /* dest data was updated */
        return 1;
    } else {
        BYTE b;

        for (i = 0; i < length; i++, src++) {
            if (dest[i] != get_attr_char_data(src[0], attr, l, char_mem,
                                              bytes_per_char, blink, revers, curpos, i)) {
                break;
            }
        }
        if (i < length) {
            /* we found a difference */
            *xs = *xe = i;
            /* xs/xe set start/end address for modified data */
            /* compare & update *dest, adjusting *xe if needed */
            for (; i < length; i++, src++) {
                b = get_attr_char_data(src[0], attr, l, char_mem,
                                       bytes_per_char, blink, revers,
                                       curpos, i);
                if (dest[i] != b) {
                    dest[i] = b;
                    *xe = i;
                }
            }
            /* dest data was updated */
            return 1;
        } else {
            /* nothing changed from last run */
            return 0;
        }
    }
}

inline static int cache_data_fill(BYTE *dest,
                                  const BYTE *src,
                                  unsigned int length,
                                  int src_step,
                                  unsigned int *xs,
                                  unsigned int *xe,
                                  int no_check,
                                  int reverse)
/* Bitmap mode - fill (*dest) with (length) bytes of bitmap data from (*src) */
{
    unsigned int i;

    if (no_check) {
        *xs = 0;
        *xe = length - 1;
        for (i = 0; i < length; i++, src += src_step) {
            /* vdc memory is a simple linear bitmap so copy it directly, reversing if needed */
            dest[i] = src[0] ^ reverse;
        }
        return 1;
    } else {
        for (i = 0; i < length && dest[i] == (src[0] ^ reverse); i++, src += src_step) {
            /* do nothing */
        }

        if (i < length) {
            *xs = *xe = i;

            for (; i < length; i++, src += src_step) {
                if (dest[i] != (src[0] ^ reverse)) {
                    dest[i] = src[0] ^ reverse;
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

static int get_std_text(raster_cache_t *cache, unsigned int *xs, unsigned int *xe, int rr)
/* aka raster_modes_fill_cache() in raster */
{
    /* fill the line cache in text mode.
       The VDC combines text mode from
       a) the video RAM
          in conjunction with
       b) the character RAM

       c) the attribute RAM
    */

    /* r=return value, cursor_pos=the cursor position in screen memory so that it can be drawn correctly */
    int r, cursor_pos;

    cursor_pos = vdc.crsrpos - vdc.screen_adr - vdc.mem_counter;

    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        /* get the character definition data, with any attributes applied from attribute memory, into the raster cache foreground_data */
        r = cache_data_fill_attr_text(cache->foreground_data,
                                      vdc.ram + vdc.screen_adr + vdc.mem_counter,
                                      vdc.ram + vdc.attribute_adr + vdc.mem_counter,
                                      vdc.ram + vdc.chargen_adr,
                                      vdc.bytes_per_char,
                                      vdc.screen_text_cols,
                                      vdc.raster.ycounter,
                                      xs, xe,
                                      rr,
                                      0,
                                      0,
                                      cursor_pos);
        /* fill the raster cache color_data_1 with the attributes from vdc memory */
        r |= raster_cache_data_fill(cache->color_data_1,
                                    vdc.ram + vdc.attribute_adr + vdc.mem_counter,
                                    vdc.screen_text_cols,
                                    xs, xe,
                                    rr);
    } else {
        /* monochrome mode - attributes from register 26 */
        /* get the character definition data, fixed attributes (only background colour, which doesn't actually do anything to these functions!) */
        r = cache_data_fill_attr_text_const(cache->foreground_data,
                                            vdc.ram + vdc.screen_adr + vdc.mem_counter,
                                            (BYTE)(vdc.regs[26] & 0x0f),
                                            vdc.ram + vdc.chargen_adr,
                                            vdc.bytes_per_char,
                                            (int)vdc.screen_text_cols,
                                            vdc.raster.ycounter,
                                            xs, xe,
                                            rr,
                                            0,
                                            0,
                                            cursor_pos);
        /* fill the raster cache color_data_1 with the foreground colour from vdc reg 26 */
        r |= raster_cache_data_fill_const(cache->color_data_1,
                                          (BYTE)(vdc.regs[26] >> 4),
                                          (int)vdc.screen_text_cols,
                                          xs, xe,
                                          rr);
    }

    return r;
}

static void draw_std_text_cached(raster_cache_t *cache, unsigned int xs,
                                 unsigned int xe)
/* aka raster_modes_draw_line_cached() in raster */
{
    BYTE *p;
    DWORD *table_ptr, *pdl_ptr, *pdh_ptr;

    unsigned int i, charwidth;
    if (vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        charwidth = 2 * (vdc.regs[22] >> 4);
    } else { /* 80 column mode */
        charwidth = 1 + (vdc.regs[22] >> 4);
    }
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        - (vdc.regs[22] >> 4)
        + ((vdc.regs[25] & 0x10) ? 1 : 0)
        + vdc.xsmooth
        + xs * charwidth;
    table_ptr = hr_table + ((vdc.regs[26] & 0x0f) << 4);
    pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0f) << 4);
    pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0f) << 4);

    if (vdc.regs[25] & 0x10) { /* double pixel mode */
        for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) {
            DWORD *pdwl = pdl_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            DWORD *pdwh = pdh_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            int d = cache->foreground_data[i];
            *((DWORD *)p) = *(pdwh + (d >> 4));
            *((DWORD *)p + 1) = *(pdwl + (d >> 4));
            *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
            *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
        }
    } else { /* normal text size */
        for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) { /* FIXME rendering in the intercharacter gap when charwidth >8 */
            DWORD *ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            int d = cache->foreground_data[i];
            *((DWORD *)p) = *(ptr + (d >> 4));
            *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
        }
    }

    /* fill the last few pixels of the display with bg colour if smooth scroll != 0 - if needed */
    if (i == vdc.screen_text_cols) {
        for (i = vdc.xsmooth; i < (unsigned)(vdc.regs[22] >> 4); i++, p++) {
            *p = (vdc.regs[26] & 0x0f);
        }
    }
}

static void draw_std_text(void)
/* raster_modes_draw_line() in raster - draw text mode when cache is not used
   This draws one raster line of text directly into the raster buffer
   (vdc.raster.draw_buffer_ptr), which is one byte per pixel, based on the VDC
   screen, attr(ibute) and char(set) ram (which are one byte per 8 pixels */
{
    BYTE *p;
    DWORD *table_ptr, *pdl_ptr, *pdh_ptr;
    BYTE *attr_ptr, *screen_ptr, *char_ptr;

    unsigned int i, d;
    unsigned int cpos = 0xffff;

    cpos = vdc.crsrpos - vdc.screen_adr - vdc.mem_counter;

    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        - (vdc.regs[22] >> 4)
        + ((vdc.regs[25] & 0x10) ? 1 : 0)
        + vdc.xsmooth;

    attr_ptr = vdc.ram + vdc.attribute_adr + vdc.mem_counter;
    screen_ptr = vdc.ram + vdc.screen_adr + vdc.mem_counter;
    char_ptr = vdc.ram + vdc.chargen_adr + vdc.raster.ycounter;

    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        /* regs[26] & 0xf is the background colour */
        table_ptr = hr_table + ((vdc.regs[26] & 0x0f) << 4);
        pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0f) << 4);
        pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0f) << 4);
        for (i = 0; i < vdc.screen_text_cols; i++, p += ((vdc.regs[25] & 0x10) ? 16 : 8)) {
            d = *(char_ptr
                  + ((*(attr_ptr + i) & VDC_ALTCHARSET_ATTR) ? 0x1000 : 0)
                  + (*(screen_ptr + i) * vdc.bytes_per_char));

            /* set underline if the underline attrib is set for this char */
            if ((vdc.raster.ycounter == vdc.regs[29]) && (*(attr_ptr + i) & VDC_UNDERLINE_ATTR)) {
                /* TODO - figure out if the pixels per char applies to the underline */
                d = 0xFF;
            }

            /* blink if the blink attribute is set for this char */
            if (vdc.attribute_blink && (*(attr_ptr + i) & VDC_FLASH_ATTR)) {
                d = 0x00;
            }

            /* reverse if the reverse attribute is set for this char */
            if (*(attr_ptr + i) & VDC_REVERSE_ATTR) {
                d ^= 0xff;
            }

            if (cpos == i) { /* handle cursor if this is the cursor */
                if ((vdc.frame_counter | 1) & crsrblink[(vdc.regs[10] >> 5) & 3]) {
                    /* invert current byte of the character if we are within the cursor area */
                    if ((vdc.raster.ycounter >= (unsigned)(vdc.regs[10] & 0x1F)) /* top of cursor */
                    && (vdc.raster.ycounter < (unsigned)(vdc.regs[11] & 0x1F))) { /* bottom of cursor */
                        /* The VDC cursor reverses the char */
                        d ^= 0xFF;
                    }
                }
            }

            if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
                d ^= 0xff;
            }

            /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
            if (vdc.regs[25] & 0x10) { /* double pixel mode */
                DWORD *pdwl = pdl_ptr + ((*(attr_ptr + i) & 0x0f) << 8);
                DWORD *pdwh = pdh_ptr + ((*(attr_ptr + i) & 0x0f) << 8);
                *((DWORD *)p) = *(pdwh + (d >> 4));
                *((DWORD *)p + 1) = *(pdwl + (d >> 4));
                *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
                *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
            } else { /* normal text size */
                DWORD *ptr = table_ptr + ((*(attr_ptr + i) & 0x0f) << 8);
                *((DWORD *)p) = *(ptr + (d >> 4));
                *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
            }
        }
    } else {
        /* monochrome mode - attributes from register 26 */
        DWORD *ptr = hr_table + (vdc.regs[26] << 4);
        DWORD *pdwl = pdl_table + (vdc.regs[26] << 4);  /* Pointers into the lookup tables */
        DWORD *pdwh = pdh_table + (vdc.regs[26] << 4);
        for (i = 0; i < vdc.screen_text_cols; i++, p += ((vdc.regs[25] & 0x10) ? 16 : 8)) {
            d = *(char_ptr + (*(screen_ptr + i) * vdc.bytes_per_char));

            if (cpos == i) { /* handle cursor if this is the cursor */
                if ((vdc.frame_counter | 1) & crsrblink[(vdc.regs[10] >> 5) & 3]) {
                    /* invert current byte of the character if we are within the cursor area */
                    if ((vdc.raster.ycounter >= (unsigned)(vdc.regs[10] & 0x1F)) /* top of cursor */
                    && (vdc.raster.ycounter < (unsigned)(vdc.regs[11] & 0x1F))) { /* bottom of cursor */
                        /* The VDC cursor reverses the char */
                        d ^= 0xFF;
                    }
                }
            }

            if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
                d ^= 0xff;
            }

            /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
            if (vdc.regs[25] & 0x10) { /* double pixel mode */
                *((DWORD *)p) = *(pdwh + (d >> 4));
                *((DWORD *)p + 1) = *(pdwl + (d >> 4));
                *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
                *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
            } else { /* normal text size */
                *((DWORD *)p) = *(ptr + (d >> 4));
                *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
            }
        }
    }
    /* fill the last few pixels of the display with bg colour if smooth scroll != 0 */
    for (i = vdc.xsmooth; i < (unsigned)(vdc.regs[22] >> 4); i++, p++) {
        *p = (vdc.regs[26] & 0x0f);
    }
}


static int get_std_bitmap(raster_cache_t *cache, unsigned int *xs,
                          unsigned int *xe, int rr)
/* aka raster_modes_fill_cache() in raster */
{
    /* r = return value */
    int r;

    r = cache_data_fill(cache->foreground_data,
                        vdc.ram + vdc.screen_adr + vdc.bitmap_counter,
                        vdc.screen_text_cols + 1,
                        1,
                        xs, xe,
                        rr,
                        (vdc.regs[24] & VDC_REVERSE_ATTR) ? 0xff : 0x0);

    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        r |= raster_cache_data_fill(cache->color_data_1,
                                    vdc.ram + vdc.attribute_adr
                                    + vdc.mem_counter + vdc.attribute_offset,
                                    vdc.screen_text_cols + 1,
                                    xs, xe,
                                    rr);
    } else {
        /* monochrome mode - attributes from register 26 */
        r |= raster_cache_data_fill_const(cache->color_data_1,
                                          (BYTE)(vdc.regs[26] >> 4),
                                          (int)vdc.screen_text_cols + 1,
                                          xs, xe,
                                          rr);
    }
    return r;
}

static void draw_std_bitmap_cached(raster_cache_t *cache, unsigned int xs,
                                   unsigned int xe)
/* raster_modes_draw_line_cached() in raster */
{
    BYTE *p;
    DWORD *table_ptr, *pdl_ptr, *pdh_ptr;
    DWORD *ptr, *pdwl, *pdwh;

    unsigned int i, d, j, fg, bg, charwidth;
    if (vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        charwidth = 2 * (vdc.regs[22] >> 4);
    } else { /* 80 column mode */
        charwidth = 1 + (vdc.regs[22] >> 4);
    }
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        + vdc.xsmooth
        - (vdc.regs[22] >> 4)
        + ((vdc.regs[25] & 0x10) ? 1 : 0)
        + xs * charwidth;

    /* TODO: See if we even need to split these renderers between attr/mono, because the attr data is filled either way. draw_std_text_cached mode() doesn't differentiate */
    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) {
                d = cache->foreground_data[i];
                pdwl = pdl_table + ((cache->color_data_1[i] & 0x0f) << 8) + (cache->color_data_1[i] & 0xf0);
                pdwh = pdh_table + ((cache->color_data_1[i] & 0x0f) << 8) + (cache->color_data_1[i] & 0xf0);
                *((DWORD *)p) = *(pdwh + (d >> 4));
                *((DWORD *)p + 1) = *(pdwl + (d >> 4));
                *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
                *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
            }
        } else { /* normal text size */
            for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) {
                d = cache->foreground_data[i];

                table_ptr = hr_table + (cache->color_data_1[i] & 0xf0);
                ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);

                *((DWORD *)p) = *(ptr + (d >> 4));
                *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
            }
        }
    } else {
        /* monochrome mode - attributes from register 26 */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0f) << 4);
            pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0f) << 4);
            
            for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) {
                d = cache->foreground_data[i];
                pdwl = pdl_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
                pdwh = pdh_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
                *((DWORD *)p) = *(pdwh + (d >> 4));
                *((DWORD *)p + 1) = *(pdwl + (d >> 4));
                *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
                *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
            }
        } else { /* normal text size */
            table_ptr = hr_table + ((vdc.regs[26] & 0x0f) << 4);

            for (i = xs; i <= (unsigned int)xe; i++, p += charwidth) {
                d = cache->foreground_data[i];

                ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);

                *((DWORD *)p) = *(ptr + (d >> 4));
                *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
            }
        }
    }

    /* fill the last few pixels of the display with bg colour if xsmooth scroll != maximum  */
    d = cache->foreground_data[i];
    if (vdc.regs[24] & VDC_REVERSE_ATTR) {
        /* reverse screen bit */
        d ^= 0xff;
    }
    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        fg = cache->color_data_1[i] >> 4;
        bg = cache->color_data_1[i] & 0x0F;
    } else {
        /* monochrome mode - attributes from register 26 */
        bg = vdc.regs[26] & 0x0F;
        fg = vdc.regs[26] >> 4;
    }
    for (i = vdc.xsmooth, j = 0x80; i < (unsigned)(vdc.regs[22] >> 4); i++, p++, j >>= 1) {
        if (d & j) {
            /* foreground */
            *p = fg;
        } else {
            *p = bg;
        }
    }
}


void draw_std_bitmap(void)
/* raster_modes_draw_line() in raster - draw bitmap mode when cache is not used
   See draw_std_text(), this is for bitmap mode. */
{
    BYTE *p;
    BYTE *attr_ptr, *bitmap_ptr;

    unsigned int i, d, j, fg, bg;

    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        - (vdc.regs[22] >> 4)
        + ((vdc.regs[25] & 0x10) ? 1 : 0)
        + vdc.xsmooth;

    attr_ptr = vdc.ram + vdc.attribute_adr + vdc.mem_counter + vdc.attribute_offset;
    bitmap_ptr = vdc.ram + vdc.screen_adr + vdc.bitmap_counter;

    for (i = 0; i < vdc.mem_counter_inc; i++, p += ((vdc.regs[25] & 0x10) ? 16 : 8)) {
        DWORD *ptr, *pdwl, *pdwh;

        if (vdc.regs[25] & 0x40) {
            /* attribute mode */
            ptr = hr_table + (*(attr_ptr + i) & 0xf0) + ((*(attr_ptr + i) & 0x0f) << 8);
            pdwl = pdl_table + (*(attr_ptr + i) & 0xf0) + ((*(attr_ptr + i) & 0x0f) << 8);
            pdwh = pdh_table + (*(attr_ptr + i) & 0xf0) + ((*(attr_ptr + i) & 0x0f) << 8);
        } else {
            /* monochrome mode - attributes from register 26 */
            ptr = hr_table + (vdc.regs[26] << 4);
            pdwl = pdl_table + (vdc.regs[26] << 4);  /* Pointers into the lookup tables */
            pdwh = pdh_table + (vdc.regs[26] << 4);
        }

        d = *(bitmap_ptr + i); /* grab the data byte from the bitmap */

        if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
            d ^= 0xff;
        }

        /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            *((DWORD *)p) = *(pdwh + (d >> 4));
            *((DWORD *)p + 1) = *(pdwl + (d >> 4));
            *((DWORD *)p + 2) = *(pdwh + (d & 0x0f));
            *((DWORD *)p + 3) = *(pdwl + (d & 0x0f));
        } else { /* normal pixel size */
            *((DWORD *)p) = *(ptr + (d >> 4));
            *((DWORD *)p + 1) = *(ptr + (d & 0x0f));
        }
    }

    /* fill the last few pixels of the display with bg colour if xsmooth scroll != maximum  */
    d = *(bitmap_ptr + i);
    if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* reverse screen bit */
        d ^= 0xff;
    }
    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        fg = *(attr_ptr + i) >> 4;
        bg = *(attr_ptr + i) & 0x0F;
    } else {
        /* monochrome mode - attributes from register 26 */
        fg = vdc.regs[26] >> 4;
        bg = vdc.regs[26] & 0x0F;
    }
    for (i = vdc.xsmooth, j = 0x80; i < (unsigned)(vdc.regs[22] >> 4); i++, p++, j >>= 1) {
        if (d & j) {
            /* foreground */
            *p = fg;
        } else {
            *p = bg;
        }
    }
}


static int get_idle(raster_cache_t *cache, unsigned int *xs, unsigned int *xe,
                    int rr)
/* aka raster_modes_fill_cache() in raster */
{
    if (rr || (vdc.regs[26] >> 4) != cache->color_data_1[0]) {
        *xs = 0;
        *xe = vdc.screen_text_cols;
        cache->color_data_1[0] = vdc.regs[26] >> 4;
        return 1;
    }

    return 0;
}

static void draw_idle_cached(raster_cache_t *cache, unsigned int xs,
                             unsigned int xe)
/* raster_modes_draw_line_cached() in raster */
{
    BYTE *p;
    DWORD idleval;

    unsigned int i;

    p = vdc.raster.draw_buffer_ptr + vdc.border_width
        + vdc.raster.xsmooth + xs * 8;

    idleval = *(hr_table + ((cache->color_data_1[0] & 0x0f) << 8));

    for (i = xs; i <= (unsigned int)xe; i++, p += 8) {
        *((DWORD *)p) = idleval;
        *((DWORD *)p + 1) = idleval;
    }
}

static void draw_idle(void)
/* raster_modes_draw_line() in raster - draw idle mode (just border) when cache is not used */
{ /* TODO - can't we just memset()?? Or just let the border code in raster look after this?? */
    BYTE *p;
    DWORD idleval;

    unsigned int i;

    p = vdc.raster.draw_buffer_ptr + vdc.border_width
        + vdc.raster.xsmooth;

    /* border colour is just the screen background colour from reg 26 bits 0-3 */
    idleval = *(hr_table + ((vdc.regs[26] & 0x0f) << 4));

    for (i = 0; i < vdc.mem_counter_inc; i++, p += ((vdc.regs[25] & 0x10) ? 16 : 8)) {
        *((DWORD *)p) = idleval;
        *((DWORD *)p + 1) = idleval;
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            *((DWORD *)p + 2) = idleval;
            *((DWORD *)p + 3) = idleval;
        }
    }
}


static void setup_modes(void)
{
    raster_modes_set(vdc.raster.modes, VDC_TEXT_MODE,
                     get_std_text,                      /* raster_modes_fill_cache() in raster */
                     draw_std_text_cached,              /* raster_modes_draw_line_cached() in raster */
                     draw_std_text,                     /* raster_modes_draw_line() in raster */
                     NULL,                              /* draw_std_background */
                     NULL);                             /* draw_std_text_foreground */

    raster_modes_set(vdc.raster.modes, VDC_BITMAP_MODE,
                     get_std_bitmap,                    /* aka raster_modes_fill_cache() in raster */
                     draw_std_bitmap_cached,            /* raster_modes_draw_line_cached() in raster */
                     draw_std_bitmap,                   /* raster_modes_draw_line() in raster */
                     NULL,                              /* draw_std_background */
                     NULL);                             /* draw_std_text_foreground */

    raster_modes_set(vdc.raster.modes, VDC_IDLE_MODE,
                     get_idle,                          /* aka raster_modes_fill_cache() in raster */
                     draw_idle_cached,                  /* raster_modes_draw_line_cached() in raster */
                     draw_idle,                         /* raster_modes_draw_line() in raster */
                     NULL,                              /* draw_std_background */
                     NULL);                             /*draw_std_text_foreground */
}

void vdc_draw_init(void)
{
    init_drawing_tables();

    setup_modes();
}
