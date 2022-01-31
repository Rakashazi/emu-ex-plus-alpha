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
#include "vdc-mem.h"
#include "vdc-resources.h"
#include "vdc.h"
#include "vdctypes.h"

/* The following tables are used to speed up the drawing.  We do not use
   multi-dimensional arrays as we can optimize better this way...  */

/* foreground(4) | background(4) | nibble(4) -> 4 pixels.  */
static uint32_t hr_table[16 * 16 * 16];

/* foreground(4) | background(4) | nibble(4) -> 8 pixels (double width)
   pdl table is the low (right) nibble into 4 bytes, pdh the high (left) */
static uint32_t pdl_table[16 * 16 * 16];
static uint32_t pdh_table[16 * 16 * 16];


/* solid cursor (no blink), no cursor, 1/16 refresh blink, 1/32 refresh blink */
static const uint8_t crsrblink[4] = { 0x01, 0x00, 0x08, 0x10 };

/* bit mask used for register 22, to determine the number of pixels in a char actually displayed */
static const uint8_t mask[16] = {
    0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
/* Tables to help with semigraphics mode, which extends the rightmost active bit across the rest of the character and into any inter character gap */
static const uint8_t semigfxtest[16] = {
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t semigfxmask[16] = {
    0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t displayedpixmask[17] = {
    0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF
};

static const int8_t displayedwidth[256] = {
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
     1, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
     1, 2, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
     1, 2, 3, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     1, 2, 3, 4, 0, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
     1, 2, 3, 4, 5, 0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
     1, 2, 3, 4, 5, 6, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7,
     1, 2, 3, 4, 5, 6, 7, 0, 8, 8, 8, 8, 8, 8, 8, 8,
     2, 3, 4, 5, 6, 7, 8, 0, 1, 8, 8, 8, 8, 8, 8, 8,
     3, 4, 5, 6, 7, 8, 8, 0, 1, 2, 8, 8, 8, 8, 8, 8,
     4, 5, 6, 7, 8, 8, 8, 0, 1, 2, 3, 8, 8, 8, 8, 8,
     5, 6, 7, 8, 8, 8, 8, 0, 1, 2, 3, 4, 8, 8, 8, 8,
     6, 7, 8, 8, 8, 8, 8, 0, 1, 2, 3, 4, 5, 8, 8, 8,
     7, 8, 8, 8, 8, 8, 8, 0, 1, 2, 3, 4, 5, 6, 8, 8,
     8, 8, 8, 8, 8, 8, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8,
     8, 8, 8, 8, 8, 8, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8
};

static const uint8_t semigfxtype[256] = {
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,   0,   0,   0,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,0xFF,   0,   0,   0,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,   0,0xFF,0xFF,   0,   0,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,0xFF,0xFF,   0,   0,   0,0xFF,0xFF,0xFF,   0,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,0xFF,   0,   0,   0,   0,0xFF,0xFF,0xFF,0xFF,   0,   0,   0,   0,
    0xFF,0xFF,0xFF,   0,   0,   0,   0,   0,0xFF,0xFF,0xFF,0xFF,0xFF,   0,   0,   0,
    0xFF,0xFF,   0,   0,   0,   0,   0,   0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,   0,
    0xFF,   0,   0,   0,   0,   0,   0,   0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   0,
       0,   0,   0,   0,   0,   0,   0,   0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

static unsigned int dmask;           /* used to mask off bits in the rendered character, if for example we are only showing 4pixels dmask=0xF0 */
static unsigned int d2mask;          /* used to mask off bits in the intercharacter gap, if there is one, so that we only display the 'correct' number of pixels */
static unsigned int semi_gfx_test;   /* a combo flag & bitmask, if 0 there is no semi-graphics effect. >0 it's the bit mask to check against, e.g. 0x08 for bit 3 */
static unsigned int semi_gfx_mask;   /* used to mask 'on' the remainder of the character, e.g. for semi_gfx_test 0x08, semi_gfx_mask= 0x07 to mask on bits 0-2 inclusive */
static unsigned int semi_gfx_type;   /* 0 = semi-graphics does not extend through intercharacter gap, 0xFF = it does */      


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
    uint32_t i;
    unsigned int f, b;

    for (i = 0; i <= 0xf; i++) {
        for (f = 0; f <= 0xf; f++) {
            for (b = 0; b <= 0xf; b++) {
                uint8_t fp, bp;
                uint8_t *p;
                int offset;

                fp = f;
                bp = b;
                offset = (f << 8) | (b << 4) | i;

                p = (uint8_t *)(hr_table + offset);
                *p = i & 0x8 ? fp : bp;
                *(p + 1) = i & 0x4 ? fp : bp;
                *(p + 2) = i & 0x2 ? fp : bp;
                *(p + 3) = i & 0x1 ? fp : bp;

                p = (uint8_t *)(pdh_table + offset);
                *p = i & 0x8 ? fp : bp;
                *(p + 1) = i & 0x8 ? fp : bp;
                *(p + 2) = i & 0x4 ? fp : bp;
                *(p + 3) = i & 0x4 ? fp : bp;

                p = (uint8_t *)(pdl_table + offset);
                *p = i & 0x2 ? fp : bp;
                *(p + 1) = i & 0x2 ? fp : bp;
                *(p + 2) = i & 0x1 ? fp : bp;
                *(p + 3) = i & 0x1 ? fp : bp;
            }
        }
    }
}


/* Calculate the displayed character width, and bit-masks for the character byte (d) & inter-character gap byte (d2) */
/* Various special cases here to cover weird observed behaviour */
static void calculate_draw_masks(void)
{
    /*  For the renderer to work we need to set shared variables:
        dmask - used to mask off bits in the rendered character, if for example we are only showing 4pixels dmask=0xF0
        d2mask - used to mask off bits in the intercharacter gap, if there is one, so that we only display the 'correct' number of pixels
        semi_gfx_test - a combo flag & bitmask, if 0 there is no semi-graphics effect. >0 it's the bit mask to check against, e.g. 0x08 for bit 3
        semi_gfx_mask - used to mask 'on' the remainder of the character, e.g. for semi_gfx_test 0x08, semi_gfx_mask= 0x07 to mask on bits 0-2 inclusive
        semi_gfx_type - flag/mask for the intercharacter gap, 0 = semi-graphics does not extend through intercharacter gap, 0xFF = it does
        
        i, displayedpixels is not required, just temp values used in calculations
    */
    
    unsigned int i;
    int displayedpixels;
    
    if (vdc.regs[25] & 0x10) { /* double pixel / 40 column mode */
        /* half the values for 40column mode are 0x10 further down, so compensate the table lookup */
        if (!(vdc.regs[22] & 0x08) && (vdc.regs[22] >= 0x10)) {
            i = vdc.regs[22] - 0x10;
        } else {
            i = vdc.regs[22];
        }
        displayedpixels = displayedwidth[i];
        if (displayedpixels < 0) {  /* invalid, i.e displays nothing but black. we don't support this quite yet so just set the displayed width to 0 */
            displayedpixels = 0;
            dmask = 0x00;   /* show no pixels */
            semi_gfx_test = 0;  /* always fail, so don't extend through any gap */
        } else {
            if ((vdc.regs[25] & 0x20 )  /* Semi-graphics mode */
             || (((vdc.regs[22] >> 4) & 0x0F) == (vdc.regs[22] & 0x0F))) { /* extra weirdness - in 40col mode if displayed width = total width, semigfx is enabled either way */
                if (((vdc.regs[22] >> 4) & 0x0F) == (vdc.regs[22] & 0x0F)) { /* in extra weirdness the width is always at least 1 */
                    if ((vdc.regs[22] & 0x0F) <= 8 ) {
                        displayedpixels = 1;
                    }
                } else if (displayedpixels == 0) {
                    displayedpixels = 8;    /* there is no width 0 in semigfx mode, so override it if it happens, e.g. R22:0-3 = 7 */
                }
                semi_gfx_test = semigfxtest[displayedpixels - 1];   /* .. set it as a mask to check the appropriate bit */
                semi_gfx_type = semigfxtype[i];  /* 0 if semigfx mode is active but doesn't extend through intercharacter gap. 0xFF otherwise, doubles as a mask */
            } else {    /* regular text mode */
                semi_gfx_test = 0;  /* always fail, so don't extend through any gap */
            }
            dmask = displayedpixmask[displayedpixels];
        }
        if ((vdc.regs[22] >> 4) > 8) { /* if there is inter-char gap then...    */
            d2mask = displayedpixmask[(vdc.regs[22] >> 4) - 6];  /* ..calculate the mask to apply to display the correct # of pixels but no more */
        } else {
            d2mask = 0x00;  /* this shouldn't matter as it shouldn't try to render the interchar gap because there isn't one, but setting it to be thorough */
        }
    } else {    /* normal / 80 column mode */
        displayedpixels = displayedwidth[vdc.regs[22]];
        if (displayedpixels < 0) {  /* invalid, i.e displays nothing but black. we don't support this quite yet so just set the displayed width to 0 */
            displayedpixels = 0;
            dmask = 0x00;   /* show no pixels */
            semi_gfx_test = 0;  /* always fail, so don't extend through any gap */
        } else {
            if (vdc.regs[25] & 0x20) {  /* Semi-graphics mode */
                if (displayedpixels == 0) {
                    displayedpixels = 8;    /* there is no width 0 in semigfx mode, so override it if it happens, e.g. R22:0-3 = 7 */
                }
                semi_gfx_test = semigfxtest[displayedpixels - 1];   /* .. set it as a mask to check the appropriate bit */
                semi_gfx_type = semigfxtype[vdc.regs[22]];  /* 0 if semigfx mode is active but doesn't extend through intercharacter gap. 0xFF otherwise, doubles as a mask */
            } else {    /* regular text mode */
                semi_gfx_test = 0;  /* always fail, so don't extend through any gap */
            }
            dmask = displayedpixmask[displayedpixels];
        }
        if ((vdc.regs[22] >> 4) >= 8) { /* if there is inter-char gap then... */
            d2mask = displayedpixmask[(vdc.regs[22] >> 4) - 6];  /* ..calculate the mask to apply to display the correct # of pixels but no more */
        } else {
            d2mask = 0x00;  /* this shouldn't matter as it shouldn't try to render the interchar gap because there isn't one, but setting it to be thorough */
        }
    }
    semi_gfx_mask = semigfxmask[displayedpixels - 1];   /* mask for any extra pixels to turn on if semigrfx mode is active */
    
}


/*-----------------------------------------------------------------------*/

inline static uint8_t get_attr_char_data(uint8_t c, uint8_t a, int l, uint8_t *char_mem,
                                      int bytes_per_char, int blink,
                                      int revers, int curpos, int index)
/* c = character, a = attributes, l = line of char required , char_mem = pointer to character memory
bytes_per_char = number of bytes per char definition
blink / revers - these don't seem to be used
curpos = where in the screen memory the cursor is
index = where in the screen memory we are, to check against the cursor */
{
    uint8_t data;
    if (a & VDC_ALTCHARSET_ATTR) {
        /* swich to alternate charset if appropriate attribute bit set */
        char_mem += 0x100 * vdc.bytes_per_char; /* 0x1000 or 0x2000, depending on character height */
    }

    if (l > (signed)vdc.regs[23]) {
        /* Return nothing if > Vertical Character Size */
        data = 0x00;
    } else {
        /* mask against r[22] - pixels per char mask */
        data = char_mem[(c * bytes_per_char) + l] & mask[vdc.regs[22] & 0x0F];
    }

    /* Pixels per char does not apply to the underline but the underline does blink, reverse and extend through inter-character spacing */
    if ((l == (signed)vdc.regs[29]) && (a & VDC_UNDERLINE_ATTR)) {
        data = 0xFF;
    }

    if ((a & VDC_FLASH_ATTR) && (vdc.attribute_blink)) {
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
            if (
            ((l >= (vdc.regs[10] & 0x1F)) && (l < (vdc.regs[11] & 0x1F)))
            || ((l == (vdc.regs[10] & 0x1F)) && (l == (vdc.regs[11] & 0x1F)))
            || (((vdc.regs[10] & 0x1F) > (vdc.regs[11] & 0x1F)) && ((l >= (vdc.regs[10] & 0x1F)) || (l < (vdc.regs[11] & 0x1F))))
            ) {
                /* The VDC cursor reverses the char */
                data ^= 0xFF;
            }
        }
    }

    return data;
}

inline static int cache_data_fill_attr_text(uint8_t *dest,
                                            const uint8_t *src,
                                            uint8_t *attr,
                                            uint8_t *char_mem,
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
        uint8_t b;
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

inline static int cache_data_fill_attr_text_const(uint8_t *dest,
                                                  const uint8_t *src,
                                                  uint8_t attr,
                                                  uint8_t *char_mem,
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
        uint8_t b;

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

inline static int cache_data_fill(uint8_t *dest,
                                  const uint8_t *src,
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
                                            (uint8_t)(vdc.regs[26] & 0x0f),
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
                                          (uint8_t)(vdc.regs[26] >> 4),
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
    uint8_t *p, *q;
    uint32_t *table_ptr, *pdl_ptr, *pdh_ptr;

    unsigned int i;
    int icsi = -1;  /* Inter Character Spacing Index - used as a combo flag/index as to whether there is any intercharacter gap to render */
    
    if (vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        if (vdc.charwidth > 16) {   /* Is there inter character spacing to render? */
            icsi = vdc.charwidth / 2 - 8;
        }
    } else { /* 80 column mode */
        if (vdc.charwidth > 8) {    /* Is there inter character spacing to render? */
            icsi = vdc.charwidth - 8;
        }
    }
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        + ((vdc.regs[25] & 0x10) ? 2 : 0)
        + vdc.xsmooth * ((vdc.regs[25] & 0x10) ? 2 : 1)
        - (vdc.regs[22] >> 4) * ((vdc.regs[25] & 0x10) ? 2 : 1)
        + xs * vdc.charwidth;
    table_ptr = hr_table + ((vdc.regs[26] & 0x0f) << 4);
    pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0f) << 4);
    pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0f) << 4);

    if (vdc.regs[25] & 0x10) { /* double pixel mode */
        for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
            uint32_t *pdwl = pdl_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            uint32_t *pdwh = pdh_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            int d = cache->foreground_data[i];
            *((uint32_t *)p) = *(pdwh + (d >> 4));
            *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
            *((uint32_t *)p + 2) = *(pdwh + (d & 0x0f));
            *((uint32_t *)p + 3) = *(pdwl + (d & 0x0f));
            if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                q = p + 16;
                if ((vdc.regs[25] & 0x20) && (d & semigfxtest[vdc.regs[22] & 0x0F])) { /* If semi-graphics mode and the rightmost active bit is set */
                    d = mask[icsi];   /* .. figure out how big it is based on the width of the gap */
                } else { /* otherwise just draw the background */
                    d = 0;
                }
                if (cache->color_data_1[i] & VDC_REVERSE_ATTR) { /* reverse if the reverse attribute is set for this char */
                    d ^= 0xff;
                }
                if (vdc.regs[24] & VDC_REVERSE_ATTR) {  /* whole screen reverse */
                    d ^= 0xff;
                }
                *((uint32_t *)q) = *(pdwh + (d >> 4));
                *((uint32_t *)q + 1) = *(pdwl + (d >> 4));
                *((uint32_t *)q + 2) = *(pdwh + (d & 0x0f));
                *((uint32_t *)q + 3) = *(pdwl + (d & 0x0f));
            }
        }
    } else { /* normal text size */
        for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
            uint32_t *ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
            int d = cache->foreground_data[i];
            *((uint32_t *)p) = *(ptr + (d >> 4));
            *((uint32_t *)p + 1) = *(ptr + (d & 0x0f));
            if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                q = p + 8;
                if ((vdc.regs[25] & 0x20) && (d & semigfxtest[vdc.regs[22] & 0x0F])) { /* If semi-graphics mode and the rightmost active bit is set */
                    d = mask[icsi];   /* .. figure out how big it is based on the width of the gap */
                } else { /* otherwise just draw the background */
                    d = 0;
                }
                if (cache->color_data_1[i] & VDC_REVERSE_ATTR) { /* reverse if the reverse attribute is set for this char */
                    d ^= 0xff;
                }
                if (vdc.regs[24] & VDC_REVERSE_ATTR) {  /* whole screen reverse */
                    d ^= 0xff;
                }
                *((uint32_t *)q) = *(ptr + (d >> 4));
                *((uint32_t *)q + 1) = *(ptr + (d & 0x0f));
            }
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
    uint8_t *p, *q;
    uint32_t *table_ptr, *pdl_ptr, *pdh_ptr, char_index;
    uint8_t *attr_ptr, *screen_ptr;

    unsigned int i, d, d2;
    unsigned int cpos = 0xFFFF;
    int icsi = -1;  /* Inter Character Spacing Index - used as a combo flag/index as to whether there is any intercharacter gap to render */
    
    cpos = vdc.crsrpos - vdc.screen_adr - vdc.mem_counter;

    if(vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        if (vdc.charwidth > 16) {   /* Is there inter character spacing to render? */
            icsi = vdc.charwidth / 2 - 8;
        }
    } else { /* 80 column mode */
        if (vdc.charwidth > 8) {    /* Is there inter character spacing to render? */
            icsi = vdc.charwidth - 8;
        }
    }
    
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        + ((vdc.regs[25] & 0x10) ? 2 : 0)
        + vdc.xsmooth * ((vdc.regs[25] & 0x10) ? 2 : 1)
        - (vdc.regs[22] >> 4) * ((vdc.regs[25] & 0x10) ? 2 : 1);

    /* attr_ptr = vdc.ram + ((vdc.attribute_adr + vdc.mem_counter) & vdc.vdc_address_mask);*/    /* keep pre-buffer pointer set-up for testing */
    attr_ptr = &vdc.attrbuf[vdc.attrbufdraw];
    /* screen_ptr = vdc.ram + ((vdc.screen_adr + vdc.mem_counter) & vdc.vdc_address_mask);*/ /* as above */
    screen_ptr = &vdc.scrnbuf[vdc.attrbufdraw];
    char_index = vdc.chargen_adr + vdc.raster.ycounter;
    
    calculate_draw_masks();
    
    /* Now actually render everything */
    if (vdc.regs[25] & 0x40) {  /* Attribute mode - background colour from regs[26] but foreground from attribute ram */
        table_ptr = hr_table + ((vdc.regs[26] & 0x0F) << 4);    /* regs[26] & 0xF is the background colour */
        pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0F) << 4);
        pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0F) << 4);
        for (i = 0; i < vdc.screen_text_cols; i++, p += vdc.charwidth) {
            if (vdc.raster.ycounter > (signed)vdc.regs[23]) {
                /* Return nothing if > Vertical Character Size */
                d = 0x00;
            } else {
                d = vdc_ram_read(char_index
                  + ((*(attr_ptr + i) & VDC_ALTCHARSET_ATTR) ? 0x100 * vdc.bytes_per_char : 0) /* the offset to the alternate character set is either 0x1000 or 0x2000, depending on the character size (16 or 32) */
                  + (*(screen_ptr + i) * vdc.bytes_per_char));
            }
            d &= dmask; /* mask off to active pixels only */
            d2 = 0x00;
            
            /* set underline if the underline attrib is set for this char */
            /* Pixels per char does not apply to the underline but the underline does blink, reverse and extend through inter-character spacing */
            if ((vdc.raster.ycounter == vdc.regs[29]) && (*(attr_ptr + i) & VDC_UNDERLINE_ATTR)) {
                d = 0xFF;
                d2 = 0xFF;
            }

            /* blink if the blink attribute is set for this char */
            if (vdc.attribute_blink && (*(attr_ptr + i) & VDC_FLASH_ATTR)) {
                d = 0x00;
                d2 = 0x00;
            }

            /* Handle semi-graphics mode. Note semi_gfx_test doubles as a flag, if it's 0 this just falls through */
            if (d & semi_gfx_test) { /* if the far right pixel is on.. */
                d |= semi_gfx_mask;  /* .. mask the rest of the right hand side on */
                d2 = semi_gfx_type;  /* this will get masked off later on, so we just set all (or none) inter-char pixels on for now */
            }
            
            /* reverse if the reverse attribute is set for this char */
            if (*(attr_ptr + i) & VDC_REVERSE_ATTR) {
                d ^= 0xFF;
                d2 ^= 0xFF;
            }

            if (cpos == i) { /* handle cursor if this is the cursor */
                if ((vdc.frame_counter | 1) & crsrblink[(vdc.regs[10] >> 5) & 3]) {
                    /* invert current byte of the character if we are within the cursor area */
                    if (
                    ((vdc.raster.ycounter >= (vdc.regs[10] & 0x1F)) && (vdc.raster.ycounter < (vdc.regs[11] & 0x1F)))
                    || ((vdc.raster.ycounter == (vdc.regs[10] & 0x1F)) && (vdc.raster.ycounter == (vdc.regs[11] & 0x1F)))
                    || (((vdc.regs[10] & 0x1F) > (vdc.regs[11] & 0x1F)) && ((vdc.raster.ycounter >= (vdc.regs[10] & 0x1F)) || (vdc.raster.ycounter < (vdc.regs[11] & 0x1F))))
                    ) {
                        /* The VDC cursor reverses the char */
                        d ^= 0xFF;
                        d2 ^= 0xFF;
                    }
                }
            }

            if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
                d ^= 0xFF;
                d2 ^= 0xFF;
            }
            
            d2 &= d2mask;   /* Mask off any extra "on" pixels from the inter-character gap */

            /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
            if (vdc.regs[25] & 0x10) { /* double pixel mode */
                uint32_t *pdwl = pdl_ptr + ((*(attr_ptr + i) & 0x0F) << 8);
                uint32_t *pdwh = pdh_ptr + ((*(attr_ptr + i) & 0x0F) << 8);
                *((uint32_t *)p) = *(pdwh + (d >> 4));
                *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
                *((uint32_t *)p + 2) = *(pdwh + (d & 0x0F));
                *((uint32_t *)p + 3) = *(pdwl + (d & 0x0F));
                if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                    q = p + 16;
                    *((uint32_t *)q) = *(pdwh + (d2 >> 4));
                    *((uint32_t *)q + 1) = *(pdwl + (d2 >> 4));
                    *((uint32_t *)q + 2) = *(pdwh + (d2 & 0x0F));
                    *((uint32_t *)q + 3) = *(pdwl + (d2 & 0x0F));
                }
            } else { /* normal text size */
                uint32_t *ptr = table_ptr + ((*(attr_ptr + i) & 0x0F) << 8);
                *((uint32_t *)p) = *(ptr + (d >> 4));
                *((uint32_t *)p + 1) = *(ptr + (d & 0x0F));
                if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                    q = p + 8;
                    *((uint32_t *)q) = *(ptr + (d2 >> 4));
                    *((uint32_t *)q + 1) = *(ptr + (d2 & 0x0F));
                }
            }
        }
    } else {    /* Monochrome mode - foreground & background colours both from register 26 */
        uint32_t *ptr = hr_table + (vdc.regs[26] << 4);
        uint32_t *pdwl = pdl_table + (vdc.regs[26] << 4);  /* Pointers into the lookup tables */
        uint32_t *pdwh = pdh_table + (vdc.regs[26] << 4);
        for (i = 0; i < vdc.screen_text_cols; i++, p += vdc.charwidth) {
            if (vdc.raster.ycounter > (signed)vdc.regs[23]) {
                /* Return nothing if > Vertical Character Size */
                d = 0x00;
            } else {
                d = vdc_ram_read(char_index
                  + (*(screen_ptr + i) * vdc.bytes_per_char));
            }
            d &= dmask; /* mask off to active pixels only */
            d2 = 0x00;

            /* Handle semi-graphics mode. Note semi_gfx_test doubles as a flag, if it's 0 this just falls through */
            if (d & semi_gfx_test) { /* if the far right pixel is on.. */
                d |= semi_gfx_mask;  /* .. mask the rest of the right hand side on */
                d2 = semi_gfx_type;  /* this will get masked off later on, so we just set all (or none) inter-char pixels on for now */
            }
            
            if (cpos == i) { /* handle cursor if this is the cursor */
                if ((vdc.frame_counter | 1) & crsrblink[(vdc.regs[10] >> 5) & 3]) {
                    /* invert current byte of the character if we are within the cursor area */
                    if (
                    ((vdc.raster.ycounter >= (vdc.regs[10] & 0x1F)) && (vdc.raster.ycounter < (vdc.regs[11] & 0x1F)))
                    || ((vdc.raster.ycounter == (vdc.regs[10] & 0x1F)) && (vdc.raster.ycounter == (vdc.regs[11] & 0x1F)))
                    || (((vdc.regs[10] & 0x1F) > (vdc.regs[11] & 0x1F)) && ((vdc.raster.ycounter >= (vdc.regs[10] & 0x1F)) || (vdc.raster.ycounter < (vdc.regs[11] & 0x1F))))
                    ) {
                        /* The VDC cursor reverses the char */
                        d ^= 0xFF;
                        d2 ^= 0xFF;
                    }
                }
            }

            if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
                d ^= 0xFF;
                d2 ^= 0xFF;
            }

            d2 &= d2mask;   /* Mask off any extra "on" pixels from the inter-character gap */
            
            /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
            if (vdc.regs[25] & 0x10) { /* double pixel mode */
                *((uint32_t *)p) = *(pdwh + (d >> 4));
                *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
                *((uint32_t *)p + 2) = *(pdwh + (d & 0x0F));
                *((uint32_t *)p + 3) = *(pdwl + (d & 0x0F));
                if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                    q = p + 16;
                    *((uint32_t *)q) = *(pdwh + (d2 >> 4));
                    *((uint32_t *)q + 1) = *(pdwl + (d2 >> 4));
                    *((uint32_t *)q + 2) = *(pdwh + (d2 & 0x0F));
                    *((uint32_t *)q + 3) = *(pdwl + (d2 & 0x0F));
                }
            } else { /* normal text size */
                *((uint32_t *)p) = *(ptr + (d >> 4));
                *((uint32_t *)p + 1) = *(ptr + (d & 0x0F));
                if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                    q = p + 8;
                    *((uint32_t *)q) = *(ptr + (d2 >> 4));
                    *((uint32_t *)q + 1) = *(ptr + (d2 & 0x0F));
                }
            }
        }
    }
    /* fill the last few pixels of the display with bg colour if smooth scroll != 0 */
    for (i = vdc.xsmooth; i < (unsigned)(vdc.regs[22] >> 4); i++, p++) {
        *p = (vdc.regs[26] & 0x0F);
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
                                    + vdc.mem_counter,
                                    vdc.screen_text_cols + 1,
                                    xs, xe,
                                    rr);
    } else {
        /* monochrome mode - attributes from register 26 */
        r |= raster_cache_data_fill_const(cache->color_data_1,
                                          (uint8_t)(vdc.regs[26] >> 4),
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
    uint8_t *p;
    uint32_t *table_ptr, *pdl_ptr, *pdh_ptr;
    uint32_t *ptr, *pdwl, *pdwh;

    unsigned int i, d, j, fg, bg;
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        + ((vdc.regs[25] & 0x10) ? 2 : 0)
        + vdc.xsmooth * ((vdc.regs[25] & 0x10) ? 2 : 1)
        - (vdc.regs[22] >> 4) * ((vdc.regs[25] & 0x10) ? 2 : 1)
        + xs * vdc.charwidth;

    /* TODO: See if we even need to split these renderers between attr/mono, because the attr data is filled either way. draw_std_text_cached mode() doesn't differentiate */
    if (vdc.regs[25] & 0x40) {
        /* attribute mode */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
                d = cache->foreground_data[i];
                pdwl = pdl_table + ((cache->color_data_1[i] & 0x0f) << 8) + (cache->color_data_1[i] & 0xf0);
                pdwh = pdh_table + ((cache->color_data_1[i] & 0x0f) << 8) + (cache->color_data_1[i] & 0xf0);
                *((uint32_t *)p) = *(pdwh + (d >> 4));
                *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
                *((uint32_t *)p + 2) = *(pdwh + (d & 0x0f));
                *((uint32_t *)p + 3) = *(pdwl + (d & 0x0f));
            }
        } else { /* normal text size */
            for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
                d = cache->foreground_data[i];

                table_ptr = hr_table + (cache->color_data_1[i] & 0xf0);
                ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);

                *((uint32_t *)p) = *(ptr + (d >> 4));
                *((uint32_t *)p + 1) = *(ptr + (d & 0x0f));
            }
        }
    } else {
        /* monochrome mode - attributes from register 26 */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            pdl_ptr = pdl_table + ((vdc.regs[26] & 0x0f) << 4);
            pdh_ptr = pdh_table + ((vdc.regs[26] & 0x0f) << 4);

            for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
                d = cache->foreground_data[i];
                pdwl = pdl_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
                pdwh = pdh_ptr + ((cache->color_data_1[i] & 0x0f) << 8);
                *((uint32_t *)p) = *(pdwh + (d >> 4));
                *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
                *((uint32_t *)p + 2) = *(pdwh + (d & 0x0f));
                *((uint32_t *)p + 3) = *(pdwl + (d & 0x0f));
            }
        } else { /* normal text size */
            table_ptr = hr_table + ((vdc.regs[26] & 0x0f) << 4);

            for (i = xs; i <= (unsigned int)xe; i++, p += vdc.charwidth) {
                d = cache->foreground_data[i];

                ptr = table_ptr + ((cache->color_data_1[i] & 0x0f) << 8);

                *((uint32_t *)p) = *(ptr + (d >> 4));
                *((uint32_t *)p + 1) = *(ptr + (d & 0x0f));
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


static void draw_std_bitmap(void)
/* raster_modes_draw_line() in raster - draw bitmap mode when cache is not used
   See draw_std_text(), this is for bitmap mode. */
{
    uint8_t *p, *q;
    uint8_t *attr_ptr;

    unsigned int i, d, d2, j, fg, bg, bitmap_index;
    int icsi = -1;  /* Inter Character Spacing Index - used as a combo flag/index as to whether there is any intercharacter gap to render */

    if(vdc.regs[25] & 0x10) { /* double pixel a.k.a 40column mode */
        if (vdc.charwidth > 16) {   /* Is there inter character spacing to render? */
            icsi = vdc.charwidth / 2 - 8;
        }
    } else { /* 80 column mode */
        if (vdc.charwidth > 8) {    /* Is there inter character spacing to render? */
            icsi = vdc.charwidth - 8;
        }
    }
    
    p = vdc.raster.draw_buffer_ptr
        + vdc.border_width
        + ((vdc.regs[25] & 0x10) ? 2 : 0)
        + vdc.xsmooth * ((vdc.regs[25] & 0x10) ? 2 : 1)
        - (vdc.regs[22] >> 4) * ((vdc.regs[25] & 0x10) ? 2 : 1);

    /*attr_ptr = vdc.ram + ((vdc.attribute_adr + vdc.mem_counter) & vdc.vdc_address_mask);*/    /* keep pre-buffer pointer set-up for testing */
    attr_ptr = &vdc.attrbuf[vdc.attrbufdraw];
    bitmap_index = vdc.screen_adr + vdc.bitmap_counter;

    calculate_draw_masks();
    
    for (i = 0; i < vdc.mem_counter_inc; i++, p += vdc.charwidth) {
        uint32_t *ptr, *pdwl, *pdwh;

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

        if (vdc.raster.ycounter > (signed)vdc.regs[23]) {
            /* Return nothing if > Vertical Character Size */
            d = 0x00;
        } else {
            d = vdc_ram_read(bitmap_index + i); /* grab the data byte from the bitmap */
        }
        d &= dmask; /* mask off to active pixels only */
        d2 = 0x00;
        
        /* Handle semi-graphics mode. Note semi_gfx_test doubles as a flag, if it's 0 this just falls through */
        if (d & semi_gfx_test) { /* if the far right pixel is on.. */
            d |= semi_gfx_mask;  /* .. mask the rest of the right hand side on */
            d2 = semi_gfx_type;  /* this will get masked off later on, so we just set all (or none) inter-char pixels on for now */
        }
        
        if (vdc.regs[24] & VDC_REVERSE_ATTR) { /* whole screen reverse */
            d ^= 0xff;
            d2 ^= 0xFF;
        }
        
        d2 &= d2mask;   /* Mask off any extra "on" pixels from the inter-character gap */

        /* actually render the byte into 8 bytes of colour pixels using the lookup tables */
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            *((uint32_t *)p) = *(pdwh + (d >> 4));
            *((uint32_t *)p + 1) = *(pdwl + (d >> 4));
            *((uint32_t *)p + 2) = *(pdwh + (d & 0x0f));
            *((uint32_t *)p + 3) = *(pdwl + (d & 0x0f));
            if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                q = p + 16;
                *((uint32_t *)q) = *(pdwh + (d2 >> 4));
                *((uint32_t *)q + 1) = *(pdwl + (d2 >> 4));
                *((uint32_t *)q + 2) = *(pdwh + (d2 & 0x0F));
                *((uint32_t *)q + 3) = *(pdwl + (d2 & 0x0F));
            }
        } else { /* normal pixel size */
            *((uint32_t *)p) = *(ptr + (d >> 4));
            *((uint32_t *)p + 1) = *(ptr + (d & 0x0f));
            if (icsi >= 0) {    /* if there's inter character spacing, then render it */
                q = p + 8;
                *((uint32_t *)q) = *(ptr + (d2 >> 4));
                *((uint32_t *)q + 1) = *(ptr + (d2 & 0x0F));
            }
        }
    }

    /* fill the last few pixels of the display with bg colour if xsmooth scroll != maximum  */
    d = vdc_ram_read(bitmap_index + i); /* grab the data byte from the bitmap */
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
    uint8_t *p;
    uint32_t idleval;

    unsigned int i;

    p = vdc.raster.draw_buffer_ptr + vdc.border_width
        + vdc.raster.xsmooth + xs * 8;

    idleval = *(hr_table + ((cache->color_data_1[0] & 0x0f) << 8));

    for (i = xs; i <= (unsigned int)xe; i++, p += 8) {
        *((uint32_t *)p) = idleval;
        *((uint32_t *)p + 1) = idleval;
    }
}

static void draw_idle(void)
/* raster_modes_draw_line() in raster - draw idle mode (just border) when cache is not used */
{ /* TODO - can't we just memset()?? Or just let the border code in raster look after this?? */
    uint8_t *p;
    uint32_t idleval;

    unsigned int i;

    p = vdc.raster.draw_buffer_ptr + vdc.border_width
        + vdc.raster.xsmooth;

    /* border colour is just the screen background colour from reg 26 bits 0-3 */
    idleval = *(hr_table + ((vdc.regs[26] & 0x0f) << 4));

    for (i = 0; i < vdc.mem_counter_inc; i++, p += ((vdc.regs[25] & 0x10) ? 16 : 8)) {
        *((uint32_t *)p) = idleval;
        *((uint32_t *)p + 1) = idleval;
        if (vdc.regs[25] & 0x10) { /* double pixel mode */
            *((uint32_t *)p + 2) = idleval;
            *((uint32_t *)p + 3) = idleval;
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
