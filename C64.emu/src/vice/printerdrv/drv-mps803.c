/*
 * drv-mps803.c - MPS printer driver.
 *
 * Written by
 *  Thomas Bretz <tbretz@gsi.de>
 *  Andreas Boose <viceteam@t-online.de>
 *  groepaz <groepaz@gmx.net>
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

/* #define DEBUG_MPS803 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "driver-select.h"
#include "drv-mps803.h"
#include "log.h"
#include "output-select.h"
#include "output.h"
#include "palette.h"
#include "printer.h"
#include "sysfile.h"
#include "types.h"

#ifdef DEBUG_MPS803
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

/* MPS printer engine.

    adapted from old MPS803, can be extended to handle most (if not all) b/w
    CBM printers.

    NOTE: Double spacing (lfn option):

    The Manual states that "if the logical file number is > 127, any printed
    lines will be double spaced.". This is not really what happens. The kernal
    does (unfortunately) NOT translate all CR or LF characters into an extra
    line feed - instead it produces an extra linefeed at the end of a PRINT
    statement (or at UNLISTEN time?). In any case, this is a kernal feature,
    and not something the printer emulation has to deal with.

    Open Questions:
        - what happens when using secondary addresses that are not supported?
        - what is/are the exact conditions when the chargen select is reset to
          what the secondary address implies?
        - is the local state of chargen select (when changed via control chars)
          preserved when printing one line, but using different secondary
          addresses?
*/

#define MAX_ROM_SIZE (8 * 1024)

/*
               char         char            chars/ dots/   dot pitch
           height width  height width         line  line   vert.  hor.
           pins   dots   inch   inch

2022          7       6  0.11   0.10          80    480
2023          7       6  0.11   0.10          80    480
4023          8       8  0.094  0.08          80    640

8023P         8       5  0.116  0.08         136    680

802/1526      8       8  0.094  0.08          80    640
MPS1000

early 801     7       6                       80    480
803           7       6  0.09   0.08          80    480    1/72"   1/60"

803:
  Char Column spacing: 10 characters/inch = 60 dots/inch
  Char Line spacing:   6 Char lines/inch (USA) or 8 Char lines/inch (europe)
                       72/7 "Char" lines/inch in bit image printing
  "Paper Feed Pitch select" (1/6" or 1/8") is a physical switch

US Letter paper is   8.5" x 11.00"
A4 paper is         8.25" x 11.75"
*/
#define MAX_CHARS_PER_LINE  136
#define MAX_BYTES_PER_CHAR  8
#define MAX_COLS_PER_CHAR   8
#define MAX_ROWS_PER_CHAR   8

/* Determining the proper value for the page height doesn't seem to be easy, it
 * is not really defined :/
 *
 * The theoretical maximum for a US Letter page would be ~ 792 dots
 *
 * "The Print Shop" needs 70 char rows (actually 693 dots high page) to be able
 * to fit a greetings card on a single page
 */
#define MPS803_PAGE_HEIGHT_CHARACTERS  70
#define MPS803_PAGE_WIDTH_CHARACTERS  80

#define MPS803_PAGE_WIDTH_DOTS     (MPS803_PAGE_WIDTH_CHARACTERS * 6)
#define MPS803_PAGE_HEIGHT_DOTS    (MPS803_PAGE_HEIGHT_CHARACTERS * 10)

#define MPS802_BYTES_PER_CHAR   8
#define MPS803_BYTES_PER_CHAR   6
#define C2022_BYTES_PER_CHAR    6
#define C4023_BYTES_PER_CHAR    8

/*
           SA= 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21

2022           *  *  *  *  *  *  *
2023           *  *  *  *  *  *
4023           *  *  *  *  *  *  *  *  *  *  *

8023P          *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *     *  *        *

MPS802         *  *  *  *  *  *  *  *     *  *
MPS1000        *  *  *  *  *  *  *  *     *  *

early MPS801   *                 *  *  *     *
MPS803         *                    *

SA= 0: Print data exactly as received
       (mps801,mps803: Select graphic mode)
SA= 1: Print data in previously defined format
SA= 2: save format data
SA= 3: set lines per page
SA= 4: Format Error request
SA= 5: define user programmable character bitmap (character 254)
SA= 6: Setting spacing between lines
SA= 7: Select business mode
SA= 8: Select graphic mode
SA= 9: prevent error messages
SA=10: Reset the printer

SA=11: Set unidirectional printing.
SA=12: Set bidirectional printing.
SA=13: Set 15 cpi. (condense mode)
SA=14: Set 10 cpi. (reset condense mode)
SA=15: Enable correspondence (overstrike, pseudo letter quality) mode.

SA=17: Print bit image graphics.
SA=18: Print received bit image graphics again.

SA=21: Disable correspondence (overstrike, pseudo letter quality) mode.
       (To disable send on SA=21, then SA=14)
 */
#define SA_FEATURE_PROG_CHAR_254    0x01    /* programable character, SA=5 */

/*
    The manuals specify the following printer control codes:

      code=   1   2   8  10  12  13  14  15  16  17  18  19  26  27  29  34 129 141 145 146 147 159 160 254

2022          *           *       *               *   *   *           *   *   *   *   *   *   *           *
2023          *           *       *               *   *   *           *   *   *   *   *   *   *           *
4023          *           *       *               *   *   *           *   *   *   *   *   *   *           *

8023P         *           *                       *   *   *           *   *   *   *   *   *   *           *

MPS802                    *       *   *           *   *   *           *   *   *   *   *   *   *           *
MPS1000       *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *       *   *   *   *   *   *   *

MPS801                *   *       *   *   *   *   *   *       *   *                   *   *
MPS803                *   *       *   *   *   *   *   *       *   *       *           *   *

  1: Enhance (increase width)
     (MPS1000: Single density (480 DPL) bit image graphics)
  2: (MPS1000: Double density (960 DPL) bit image graphics)
  8: Enter Graphic Mode (Bit Image Printing)
     (MPS1000: Bit image (7-vertical dot) with 7/72" line feed)
 10: Line Feed
     (MPS1000: with carriage return)
 12: Form Feed
 13: Carriage Return
     (MPS1000: with line feed)
 14: Enhance ON (Enter Double Width Character Mode)
 15: Enter Standard Character Mode
     (mps803: enhance off)
 16: Tab setting the Print Head ("NHNL")
 17: Lowercase / Business Mode (Enter Cursor Down Mode)
 18: Reverse ON
 19: Paging OFF (HOME)
 26: Repeat Graphics Selected (Bit Image Repeat)
 27: specify Dot Address (must follow Print Head Tab Code)
 29: Skip Space
 34: Quote
129: Enhance OFF

141: Carriage Return without Line Feed
145: Uppercase / Graphics Mode (Enter Cursor Up Mode)
146: Reverse OFF
147: Paging ON (CLR)

159: NLQ OFF
160: Prints blank alpha field in formatting print

254: print programmable character
 */

#define CTRL_FEATURE_OLD_ENHANCE        0x01    /* old-style enhance using ctrl 1/129 */
#define CTRL_FEATURE_NEW_ENHANCE        0x02    /* new-style enhance using ctrl 14/15 */
#define CTRL_FEATURE_BIT_IMAGE_PRINTING 0x04    /* ctrl 8 */
#define CTRL_FEATURE_CR_WITHOUT_LF      0x08    /* carriage return without linefeed, ctrl 141 */

struct mps_s {
    uint8_t line[MAX_CHARS_PER_LINE * MAX_COLS_PER_CHAR][MAX_ROWS_PER_CHAR]; /* make sure max size fits in */
    int char_height;    /* printer head number of pins */
    int char_width;     /* horizontal dots per char */
    int page_width_dots;
    int page_height_dots;
    int lookup_method;
    int model_sa_features;
    int model_ctrl_features;
    int repeatn;
    int pos;
    int tab;
    uint8_t tabc[3];
    int begin_line;
    int mode_global;            /* global mode */
    int char_col_repeat;        /* how often to repeat cols in double-width mode */

    /* for SA = 5 */
    int custom_char_ptr;
    int custom_char[8];  /* not uint8_t so we can handle > 8 pins later */
    int custom_char_set;

    uint8_t charset[512 * MAX_BYTES_PER_CHAR]; /* full charset */
    uint8_t rom[MAX_ROM_SIZE]; /* full printer rom */
};
typedef struct mps_s mps_t;

static mps_t drv_mps803[NUM_OUTPUT_SELECT];

/* FIXME: palette per mps_t? */
static palette_t *palette = NULL;

/* FIXME: log per mps_t? */
static log_t drv803_log = LOG_ERR;

/* global mode */
#define MPS_REVERSE  0x01
#define MPS_CRSRUP   0x02 /* set in gfxmode (default) unset in businessmode */
#define MPS_BITMODE  0x04
#define MPS_DBLWDTH  0x08
#define MPS_REPEAT   0x10
#define MPS_ESC      0x20
#define MPS_QUOTED   0x40 /* odd number of quotes in line (textmode) */

#define MPS_BUSINESS 0x80 /* opened with SA = 7 in business mode */
#define MPS_GRAPHICS 0x00 /* opened with SA = 0 in graphics mode */

/* ------------------------------------------------------------------------- */

/* get one dot to print (for character)

   the charset is N columns (=bytes) per char, lsb -> msb containling line 0 - 7

   this is used in the printer engine
*/
static int get_charset_bit(mps_t *mps, int nr, unsigned int col, unsigned int row)
{
    int reverse = (mps->mode_global & MPS_REVERSE) ? 1 : 0;
    int bytes_per_char = mps->char_width;

    /* FIXME: custom char */
    if (nr > 999) {
        return mps->custom_char[col] & (1 << ((mps->char_height - 1) - row)) ? !reverse : reverse;
    }
    return mps->charset[(nr * bytes_per_char) + col] & (1 << ((mps->char_height - 1) - row)) ? !reverse : reverse;
}

/* get one dot from ROM (for character)

   this is only used in the initial loading/conversion
 */
static int get_bit_from_rom(mps_t *mps, int offs, int ch, int col, int row)
{
    int bytes_per_char = mps->char_width;
    switch (mps->lookup_method) {
        case 0: /* mps801, mps803 */
            return mps->rom[offs + ((ch * bytes_per_char) + (    col))] & (1 << (    row));
        case 1: /* mps802, 4023, 8023 */
            return mps->rom[offs + ((ch * bytes_per_char) + (    col))] & (1 << (7 - row));
        case 2: /* 2022 */
            return mps->rom[offs + ((ch * bytes_per_char) + (    col))] & (1 << (6 - row));
    }
    return 0;
}

#ifdef DEBUG_MPS803
static void dump_printer_charset(mps_t *mps) {
#if 0
    int bytes_per_char = mps->char_width;
    int ch;
    int row, col, bit;
    int x,y;
    printf("using printer charset: %dx%d\n", mps->char_width, mps->char_height);
    for (y = 0; y < (512/16); y++) {
        for (row = 0; row < mps->char_height; row++) {
            printf("%04x: ", (y * bytes_per_char * 16) + row);
            for (x = 0; x < 16; x++) {
                ch = x + (y * 16);
                for (col = 0; col < mps->char_width; col++) {
                    bit = get_charset_bit(mps, ch, col, row);
                    printf("%c", bit ? '*' : '.');
                }
                printf(" ");
            }
            printf("\n");
        }
        printf("\n");
    }
#endif
}
#endif

/*      0 1 2 3 4 5 6 7  <- byte offset
 *   1
 *   2
 *   4
 *   8
 *  16
 *  32
 *  64
 * 128
 */

static void convert_rom_char(mps_t *mps, int offs, int destchar, int numchar)
{
    int ch;
    int row, col, bit;
    int bytes_per_char = mps->char_width;

    for (ch = 0; ch < numchar; ch++) {
        for (col = 0; col < mps->char_width; col++) {
            mps->charset[((ch + destchar) * bytes_per_char) + col] = 0;
            for (row = 0; row < mps->char_height; row++) {
                bit = get_bit_from_rom(mps, offs, ch, col, row);
                if (bit) {
                    mps->charset[((ch + destchar) * bytes_per_char) + col] |= (1 << ((mps->char_height - 1) - row));
                }
            }
        }
    }
}

/* used by 2022,2023,3022,8023 */
static void convert_rom_char_192(mps_t *mps, int offs)
{
    int bytes_per_char = mps->char_width;

    /* NOTE: 2022: "pound" does not exist, it is replaced by a backslash! */
    /* uppercase/graphics mode */
    convert_rom_char(mps, offs+(32*1*bytes_per_char), (32*1), 32);
    convert_rom_char(mps, offs+(32*0*bytes_per_char), (32*2), 32);
    convert_rom_char(mps, offs+(32*2*bytes_per_char), (32*3), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char), (32*5), 32);
    convert_rom_char(mps, offs+(32*2*bytes_per_char), (32*6), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char), (32*7), 32);
    convert_rom_char(mps, offs+(((32*2)+30)*bytes_per_char), (32*7)+31, 1); /* pi */

    /* lowercase/business mode */
    convert_rom_char(mps, offs+(32*1*bytes_per_char),256+ (32*1), 32);
    convert_rom_char(mps, offs+(32*4*bytes_per_char),256+ (32*2), 32);
    convert_rom_char(mps, offs+(32*0*bytes_per_char),256+ (32*3), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char),256+ (32*5), 32);
    convert_rom_char(mps, offs+(32*0*bytes_per_char),256+ (32*6), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char),256+ (32*7), 32);

    convert_rom_char(mps, offs+(((32*0)+0)*bytes_per_char),256+ (32*2), 1); /* @ */
    convert_rom_char(mps, offs+(((32*0)+27)*bytes_per_char),256+27+ (32*2), 5);
    convert_rom_char(mps, offs+(((32*4)+0)*bytes_per_char),256+ (32*3), 1); /* shift+@ */
    convert_rom_char(mps, offs+(((32*4)+27)*bytes_per_char),256+27+ (32*3), 5);

    convert_rom_char(mps, offs+(((32*3)+7)*bytes_per_char),256+7+ (32*5), 1);
    convert_rom_char(mps, offs+(((32*5)+9)*bytes_per_char),256+9+ (32*5), 1);
    convert_rom_char(mps, offs+(((32*4)+0)*bytes_per_char),256+ (32*6), 1); /* shift+@ */
    convert_rom_char(mps, offs+(((32*4)+27)*bytes_per_char),256+27+ (32*6), 5);
    convert_rom_char(mps, offs+(((32*3)+7)*bytes_per_char),256+7+ (32*7), 1);
    convert_rom_char(mps, offs+(((32*5)+9)*bytes_per_char),256+9+ (32*7), 1);
    convert_rom_char(mps, offs+(((32*4)+30)*bytes_per_char),256+31+ (32*7), 1); /* lowercase pi */
}

/* used by mps803 (160 chars) */
static void convert_rom_char_160(mps_t *mps, int offs)
{
    int bytes_per_char = mps->char_width;

    /* uppercase/graphics mode */
    convert_rom_char(mps, offs+(32*0*bytes_per_char), (32*1), 32);
    convert_rom_char(mps, offs+(32*1*bytes_per_char), (32*2), 32);
    convert_rom_char(mps, offs+(32*4*bytes_per_char), (32*3), 32);

    convert_rom_char(mps, offs+(32*3*bytes_per_char), (32*5), 32);
    convert_rom_char(mps, offs+(32*4*bytes_per_char), (32*6), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char), (32*7), 32);
    convert_rom_char(mps, offs+(((32*4)+30)*bytes_per_char), (32*7)+31, 1); /* pi */

    /* lowercase/business mode */
    convert_rom_char(mps, offs+(32*0*bytes_per_char),256+ (32*1), 32);
    convert_rom_char(mps, offs+(32*2*bytes_per_char),256+ (32*2), 32);
    convert_rom_char(mps, offs+(32*1*bytes_per_char),256+ (32*3), 32);

    convert_rom_char(mps, offs+(32*3*bytes_per_char),256+ (32*5), 32);
    convert_rom_char(mps, offs+(32*1*bytes_per_char),256+ (32*6), 32);
    convert_rom_char(mps, offs+(32*3*bytes_per_char),256+ (32*7), 32);

    convert_rom_char(mps, offs+(((32*1)+ 0)*bytes_per_char),256+    (32*2), 1); /* @ */
    convert_rom_char(mps, offs+(((32*1)+27)*bytes_per_char),256+27+ (32*2), 5);
    convert_rom_char(mps, offs+(((32*2)+ 0)*bytes_per_char),256+    (32*3), 1); /* shift+@ */
    convert_rom_char(mps, offs+(((32*4)+27)*bytes_per_char),256+27+ (32*3), 3);
    convert_rom_char(mps, offs+(((32*2)+27)*bytes_per_char),256+30+ (32*3), 2);

    convert_rom_char(mps, offs+(((32*2)+29)*bytes_per_char),256+9+  (32*5), 1);
    convert_rom_char(mps, offs+(((32*2)+30)*bytes_per_char),256+26+ (32*5), 1);
    convert_rom_char(mps, offs+(((32*2)+ 0)*bytes_per_char),256+    (32*6), 1); /* shift+@ */
    convert_rom_char(mps, offs+(((32*4)+27)*bytes_per_char),256+27+ (32*6), 3);
    convert_rom_char(mps, offs+(((32*2)+27)*bytes_per_char),256+30+ (32*6), 2);
    convert_rom_char(mps, offs+(((32*2)+29)*bytes_per_char),256+9+  (32*7), 1);
    convert_rom_char(mps, offs+(((32*2)+30)*bytes_per_char),256+26+ (32*7), 1);
    convert_rom_char(mps, offs+(((32*2)+27)*bytes_per_char),256+31+ (32*7), 1); /* lowercase pi */
}

/* used by mps801 (164 chars) */
static void convert_rom_char_mps801(mps_t *mps, int offs)
{
    int bytes_per_char = mps->char_width;

    /* uppercase/graphics mode */
    convert_rom_char(mps, offs+0x000, (32*1), 32);
    convert_rom_char(mps, offs+0x100, (32*2), 32);
    convert_rom_char(mps, offs+0x300, (32*3), 32); /* 4 extra chars here! */

    convert_rom_char(mps, offs+0x200, (32*5), 32);
    convert_rom_char(mps, offs+0x300, (32*6), 32);
    convert_rom_char(mps, offs+0x200, (32*7), 32);
    convert_rom_char(mps, offs+0x300+ (30*bytes_per_char), (32*7)+31, 1); /* pi */

    /* lowercase/business mode */
    convert_rom_char(mps, offs+0x000,256+ (32*1), 32);
    convert_rom_char(mps, offs+0x500,256+ (32*2), 32);
    convert_rom_char(mps, offs+0x100,256+ (32*3), 32);

    convert_rom_char(mps, offs+0x200,256+ (32*5), 32);
    convert_rom_char(mps, offs+0x100,256+ (32*6), 32);
    convert_rom_char(mps, offs+0x200,256+ (32*7), 32);

    convert_rom_char(mps, offs+0x100,256+ (32*2), 1); /* @ */
    convert_rom_char(mps, offs+0x100+ (27*bytes_per_char),256+27+ (32*2), 5);
    convert_rom_char(mps, offs+0x500,256+ (32*3), 1); /* shift+@ */
    convert_rom_char(mps, offs+0x500+ (27*bytes_per_char),256+27+ (32*3), 5);

    convert_rom_char(mps, offs+0x300+(35*bytes_per_char),256+9+ (32*5), 1);
    convert_rom_char(mps, offs+0x300+(34*bytes_per_char),256+9+17+ (32*5), 1);
    convert_rom_char(mps, offs+0x500,256+ (32*6), 1); /* shift+@ */
    convert_rom_char(mps, offs+0x500+(27*bytes_per_char),256+27+ (32*6), 5);
    convert_rom_char(mps, offs+0x300+(35*bytes_per_char),256+9+ (32*7), 1);
    convert_rom_char(mps, offs+0x300+(34*bytes_per_char),256+9+17+ (32*7), 1);
    convert_rom_char(mps, offs+0x500+(30*bytes_per_char),256+31+ (32*7), 1); /* lowercase pi */
}

/* ------------------------------------------------------------------------- */

static void set_global_mode(mps_t *mps, unsigned int m)
{
    mps->mode_global |= m;
}

static void unset_global_mode(mps_t *mps, unsigned int m)
{
    mps->mode_global &= ~m;
}

static int get_global_mode(mps_t *mps, unsigned int m)
{
    return mps->mode_global & m;
}

/* FIXME: it is not quite clear how this works exactly */
static int get_chargen_mode(mps_t *mps, unsigned int secondary)
{
    return (secondary == 7) ? MPS_BUSINESS : MPS_GRAPHICS;
    /* return mps->chargen_select[secondary]; */
}

#if 0
static void set_chargen_mode(mps_t *mps, unsigned int secondary, unsigned int m)
{
    mps->chargen_select[secondary] = m;
}
#endif

/* ------------------------------------------------------------------------- */

static void print_cbm_char(mps_t *mps, const uint8_t rawchar)
{
    unsigned int y, x;
    int c, err = 0;
    int n;

    c = (int)rawchar;
    /* 254: print programmable character */
    if (mps->model_sa_features & SA_FEATURE_PROG_CHAR_254) {
        if (c == 254) {
            c = 1000; /* FIXME: tag custom char */
        }
    }

    /* in the ROM, graphics charset comes first, then business */
    if (!get_global_mode(mps, MPS_CRSRUP)) {
        c += 256;
    }

    for (y = 0; y < mps->char_height; y++) {
        if (get_global_mode(mps, MPS_DBLWDTH)) {
#if 0   /* double width (mps803) */
            for (x = 0; x < mps->char_width; x++) {
                if ((mps->pos + x * 2) >= mps->page_width_dots) {
                    err = 1;
                    break;
                }
                mps->line[mps->pos + x * 2][y] = get_charset_bit(mps, c, x, y);
                if ((mps->pos + x * 2 + 1) >= mps->page_width_dots) {
                    err = 1;
                    break;
                }
                mps->line[mps->pos + x * 2 + 1][y] = get_charset_bit(mps, c, x, y);
            }
#endif
#if 1   /* triple width (2022) */
            for (x = 0; x < mps->char_width; x++) {
                for (n = 0; n < mps->char_col_repeat; n++) {
                    if ((mps->pos + x * mps->char_col_repeat + n) >= mps->page_width_dots) {
                        err = 1;
                        break;
                    }
                    mps->line[mps->pos + x * mps->char_col_repeat + n][y] = get_charset_bit(mps, c, x, y);
                }
            }
#endif
        } else {
            for (x = 0; x < mps->char_width; x++) {
                if ((mps->pos + x) >= mps->page_width_dots) {
                    err = 1;
                    break;
                }
                mps->line[mps->pos + x][y] = get_charset_bit(mps, c, x, y);
            }
        }
    }

    if (err) {
        log_error(drv803_log, "Printing beyond limit of %d dots.", mps->page_width_dots);
    }

#if 0   /* double width (mps803) */
    mps->pos += get_global_mode(mps, MPS_DBLWDTH) ? (mps->char_width * 2) : mps->char_width;
#endif
#if 1   /* triple width (2022) */
    mps->pos += get_global_mode(mps, MPS_DBLWDTH) ? (mps->char_width * mps->char_col_repeat) : mps->char_width;
#endif
}

static void write_line(mps_t *mps, unsigned int prnr)
{
    int x, y;

    for (y = 0; y < mps->char_height; y++) {
        for (x = 0; x < mps->page_width_dots; x++) {
            output_select_putc(prnr, (uint8_t)(mps->line[x][y] ? OUTPUT_PIXEL_BLACK : OUTPUT_PIXEL_WHITE));
        }
        output_select_putc(prnr, (uint8_t)(OUTPUT_NEWLINE));
    }

    /*
    mps803:
    line feeds are:
    - 6 lines (USA) or 8 lines (Europe) per inch in character- and double width character mode
      ("Paper Feed Pitch select" (1/6" or 1/8") is a physical switch)
    - 9 lines per inch in bit image graphic print mode
    */

    /* FIXME : handle variable spacing between lines */
    if (!get_global_mode(mps, MPS_BITMODE)) {
        /* bitmode:  9 rows/inch (7lines/row * 9rows/inch=63 lines/inch) */
        /* charmode: 6 rows/inch (7lines/row * 6rows/inch=42 lines/inch) */
        /*   --> 63lines/inch - 42lines/inch = 21lines/inch missing */
        /*   --> 21lines/inch / 9row/inch = 3lines/row missing */
        output_select_putc(prnr, OUTPUT_NEWLINE);
        output_select_putc(prnr, OUTPUT_NEWLINE);
        output_select_putc(prnr, OUTPUT_NEWLINE);
    }

    mps->pos = 0;
}

static void clear_buffer(mps_t *mps)
{
    unsigned int x, y;

    for (x = 0; x < mps->page_width_dots; x++) {
        for (y = 0; y < mps->char_height; y++) {
            mps->line[x][y] = 0;
        }
    }
}

static void print_bitmask(mps_t *mps, unsigned int prnr, const char c)
{
    unsigned int y;
    unsigned int i;

    if (!mps->repeatn) {
        mps->repeatn = 1;
    }

    for (i = 0; i < (unsigned int)(mps->repeatn); i++) {
        if (mps->pos >= mps->page_width_dots) {  /* flush buffer*/
            write_line(mps, prnr);
            clear_buffer(mps);
        }
        for (y = 0; y < mps->char_height; y++) {
            mps->line[mps->pos][y] = c & (1 << (y)) ? 1 : 0;
        }

        mps->pos++;
    }
    mps->repeatn=0;
}

/* ------------------------------------------------------------------------- */

static void warn_ctrl_code(int code)
{
    log_warning(drv803_log, "ctrl code %d not supported by this printer", code);
}

static void mps_engine_putc(mps_t *mps, unsigned int prnr, unsigned int secondary, const uint8_t c)
{
    static int last_secondary = -1;

    if (mps->tab) {     /* decode tab-number*/
        mps->tabc[2 - mps->tab] = c;

        if (mps->tab == 1) {
            mps->pos =
                get_global_mode(mps, MPS_ESC) ?
                mps->tabc[0] << 8 | mps->tabc[1] :
                atoi((char *)mps->tabc) * mps->char_width;

            unset_global_mode(mps, MPS_ESC);
        }

        mps->tab--;
        return;
    }

    if (get_global_mode(mps, MPS_ESC) && (c != 16)) {
        unset_global_mode(mps, MPS_ESC);
    }

    if (get_global_mode(mps, MPS_REPEAT)) {
        mps->repeatn = c;
        unset_global_mode(mps, MPS_REPEAT);
        return;
    }

    if (get_global_mode(mps, MPS_BITMODE) && (c & 128)) {
        print_bitmask(mps, prnr, c);
        return;
    }

    /* FIXME: it is not quite clear under what condition the chargen mode is
              reset to what the secondary address implies. right now we do it
              under the following conditions:
              - the current character is the first character in a logical line,
                ie it follows a CR
              - the secondary address is different to the secondary address used
                for the last character.
    */

    if (mps->begin_line || (last_secondary != secondary)) {
        if (get_chargen_mode(mps, secondary) == MPS_BUSINESS) {
            unset_global_mode(mps, MPS_CRSRUP);
        } else {
            set_global_mode(mps, MPS_CRSRUP);
        }
#if 0
        /* reset the custom char pointer here. in reality it is probably reset
           by unlisten on secondary 5 */
        mps->custom_char_ptr = 0;
#endif
    }
    mps->begin_line = 0;
    last_secondary = secondary;

    /* handle user defined char #254 */
    if (secondary == 5) {
        if (mps->model_sa_features & SA_FEATURE_PROG_CHAR_254) {
            if (mps->custom_char_ptr < mps->char_width) {
                DBG(("define custom char col: %d:%02x %3d", mps->custom_char_ptr, c, c));
                mps->custom_char[mps->custom_char_ptr & 7] = c;
                mps->custom_char_ptr++;
#if 0
                /* FIXME: not sure if the printer really uses a flag for this */
                if (mps->custom_char_ptr == mps->char_width) {
                    mps->custom_char_set = 1;
                    mps->custom_char_ptr = 0;
                }
                /* HACK HACK: this makes user defined chars with less columns than the printer supports work
                            without extra garbage added by CR */
                if ((mps->custom_char_ptr == 7) && ( mps->custom_char[6] == 0x0d)) {
                    log_warning(LOG_DEFAULT, "FIXME: possibly end of custom char data (pos:6)");
                    mps->custom_char_set = 1;
                    mps->custom_char_ptr = 0;
                    mps->custom_char[6] = 0;
                } else if ((mps->custom_char_ptr == 8) && ( mps->custom_char[7] == 0x0d)) {
                    log_warning(LOG_DEFAULT, "FIXME: possibly end of custom char data (pos:7)");
                    mps->custom_char_set = 1;
                    mps->custom_char_ptr = 0;
                    mps->custom_char[7] = 0;
                }
#endif
            } else {
                mps->custom_char_ptr = 0;
            }
            return;
        }
        log_warning(LOG_DEFAULT, "SA=5 (custom char) not supported by selected printer");
    }

    /* it seems that CR works even in quote mode */
    switch (c) {
        case 13: /*  Carriage Return (MPS1000: with line feed)
                        - all data in buffer is printed and paper is advanced one line
                        - turns off reverse and quote mode
                 */
            mps->pos = 0;
#if 0
            if (get_chargen_mode(mps, secondary) == MPS_BUSINESS) {
                unset_global_mode(mps, MPS_CRSRUP);
            } else {
                set_global_mode(mps, MPS_CRSRUP);
            }
#endif
            /* CR resets Quote mode, revers mode, ... */
            unset_global_mode(mps, MPS_QUOTED);
            unset_global_mode(mps, MPS_REVERSE);

            if (mps->model_ctrl_features & CTRL_FEATURE_OLD_ENHANCE) {
                /* 2022 */
                unset_global_mode(mps, MPS_DBLWDTH);
                mps->char_col_repeat = 1;
            }

            write_line(mps, prnr);
            clear_buffer(mps);
            mps->begin_line = 1;
            return;
    }

    /* in text mode ignore most (?) other control chars when quote mode is active */
    if (!get_global_mode(mps, MPS_QUOTED) || get_global_mode(mps, MPS_BITMODE)) {

        switch (c) {
            case 8: /* Enter Graphic Mode (Bit Image Printing)
                       (MPS1000: Bit image (7-vertical dot) with 7/72" line feed) */
                if (mps->model_ctrl_features & CTRL_FEATURE_BIT_IMAGE_PRINTING) {
                    set_global_mode(mps, MPS_BITMODE);
                } else {
                    warn_ctrl_code(8);
                }
                return;

            case 10: /* Line Feed (MPS1000: with carriage return)
                        all data in buffer is printed and paper is advanced one line */
                write_line(mps, prnr);
                clear_buffer(mps);
                return;

            case 13 + 128: /* shift CR: CR without LF (from 4023 printer) */
                if (mps->model_ctrl_features & CTRL_FEATURE_CR_WITHOUT_LF) {
                    mps->pos = 0;
                    if (get_chargen_mode(mps, secondary) == MPS_BUSINESS) {
                        unset_global_mode(mps, MPS_CRSRUP);
                    } else {
                        set_global_mode(mps, MPS_CRSRUP);
                    }
                    /* CR resets Quote mode, revers mode, ... */
                    unset_global_mode(mps, MPS_QUOTED);
                    unset_global_mode(mps, MPS_REVERSE);
                } else {
                    warn_ctrl_code(13 + 128);
                }
                return;

            case 1: /* Enhance (increase width) on (8023)
                       (MPS1000: Single density (480 DPL) bit image graphics) */
                if (mps->model_ctrl_features & CTRL_FEATURE_OLD_ENHANCE) {
                    set_global_mode(mps, MPS_DBLWDTH);
                    if (get_global_mode(mps, MPS_BITMODE)) {
                        unset_global_mode(mps, MPS_BITMODE);
                    }
                    mps->char_col_repeat++;
                } else {
                    warn_ctrl_code(1);
                }
                return;

            case 129: /* EN off (8023) */
                if (mps->model_ctrl_features & CTRL_FEATURE_OLD_ENHANCE) {
                    unset_global_mode(mps, MPS_DBLWDTH);
                    if (get_global_mode(mps, MPS_BITMODE)) {
                        unset_global_mode(mps, MPS_BITMODE);
                    }
                    mps->char_col_repeat = 1;
                } else {
                    warn_ctrl_code(129);
                }
                return;

            case 14: /* Enhance ON (Enter Double Width Character Mode)
                        mps803:
                            - following characters are 12 pixel wide, 7 pixel high
                            - cancels bit image printing, switches to 1/16 inch line feed
                      */
                if (mps->model_ctrl_features & CTRL_FEATURE_NEW_ENHANCE) {
                    set_global_mode(mps, MPS_DBLWDTH);
                    if (get_global_mode(mps, MPS_BITMODE)) {
                        unset_global_mode(mps, MPS_BITMODE);
                    }
                    mps->char_col_repeat = 2;
                } else {
                    warn_ctrl_code(14);
                }
                return;

            case 15: /* Enter Standard Character Mode (mps803: enhance off)
                        mps803:
                            - following characters are 6 pixel wide, 7 pixel high
                            - cancels bit image printing, switches to 1/16 inch line feed
                      */
                if (mps->model_ctrl_features & CTRL_FEATURE_NEW_ENHANCE) {
                    unset_global_mode(mps, MPS_DBLWDTH);
                } else {
                    warn_ctrl_code(15);
                }
                if (mps->model_ctrl_features & CTRL_FEATURE_BIT_IMAGE_PRINTING) {
                    if (get_global_mode(mps, MPS_BITMODE)) {
                        unset_global_mode(mps, MPS_BITMODE);
                    }
                }
                mps->char_col_repeat = 1;
                return;

            case 16: /* TAB setting the Print Head - chr$(16);"nHnL"
                        - moves printer head to absolute (char) position in the line */
                mps->tab = 2; /* 2 chars (digits) following, number of first char */
                return;

            /*
            * By sending the cursor up code [CHR$(145)] to your printer, following
            * characters will be printed in cursor up (graphic) mode until either
            * a carriage return or cursor down code [CHR$(17)] is detected.
            *
            * By sending the cursor down code [CHR$(17)] to your printer,
            * following characters will be printed in business mode until either
            * a carriage return or cursor up code [CHR$(145)] is detected.
            */
            case 17: /* (local) Lowercase / Business Mode (Enter Cursor Down Mode)
                         - functions until chr$(145) or CR
                      */
                unset_global_mode(mps, MPS_CRSRUP);
                return;

            case 145: /* (local) Uppercase / Graphics Mode (Enter Cursor Up Mode)
                          - functions until chr$(17) or CR
                       */
                set_global_mode(mps, MPS_CRSRUP);
                return;

            case 18: /* Reverse ON
                        - cancelled by CR
                      */
                set_global_mode(mps, MPS_REVERSE);
                return;

            case 146: /* Reverse OFF (18+128) */
                unset_global_mode(mps, MPS_REVERSE);
                return;

            case 26: /* Repeat Graphics Selected (Bit Image Repeat)
                        - repeat last chr$(8) c times. ?
                        - repeat graphic selected - chr$(26);chr$(repeat);chr$(image data) ?
                        */
                set_global_mode(mps, MPS_REPEAT);
                mps->repeatn = 1;
                return;

            case 27: /* specify Dot Address (must follow Print Head Tab Code)
                        chr$(27);chr$(16);chr$(nH)chr$(nL)
                        - set head position in dot units
                      */
                set_global_mode(mps, MPS_ESC); /* followed by 16, and number MSB, LSB*/
                return;

            case 2: /* FIXME: (MPS1000: Double density (960 DPL) bit image graphics) */
            case 12: /* FIXME: Form Feed */
            case 19: /* FIXME: Paging OFF (HOME) */
            case 29: /* FIXME: Skip Space */
            case 147: /* FIXME: Paging ON (CLR) */
            case 159: /* FIXME: NLQ OFF */
                warn_ctrl_code(c);
                return;
            case 160: /* FIXME: Prints blank alpha field in formatting print */
                warn_ctrl_code(c);
                /* continue, it will print a space */
                break;
        }

    }

    if (mps->model_ctrl_features & CTRL_FEATURE_BIT_IMAGE_PRINTING) {
        if (get_global_mode(mps, MPS_BITMODE)) {
            return;
        }
    }

   /*
    * When an odd number of CHR$(34) is detected in a line, the control
    * codes $00-$1F and $80-$9F will be made visible by printing a
    * reverse character for each of these controls. This will continue
    * until an even number of quotes [CHR$(34)] has been received or until
    * end of line.
    */
    if (c == 34) {
        mps->mode_global ^= MPS_QUOTED;
    }

    /*
        automatic printing:

        - automatic printing occurs under 3 conditions:

        a) when buffer fills up during input of data
        b) when printer "sees" that you have used more than 480 dots per line (chars
        and spaces count as 6 dots each)
        c) when a) and b) happen at the same time

        manual is a bit unclear about how much of the buffer will be printed
    */
    if ((mps->pos + (mps->char_col_repeat * mps->char_width)) > mps->page_width_dots) {
        /* flush buffer*/
        write_line(mps, prnr);
        clear_buffer(mps);
    }

    if (get_global_mode(mps, MPS_QUOTED)) {
        if (c <= 0x1f) {
            set_global_mode(mps, MPS_REVERSE);
            print_cbm_char(mps, (uint8_t)(c + 0x40));
            unset_global_mode(mps, MPS_REVERSE);
            return;
        }
        if ((c >= 0x80) && (c <= 0x9f)) {
            set_global_mode(mps, MPS_REVERSE);
            print_cbm_char(mps, (uint8_t)(c - 0x20));
            unset_global_mode(mps, MPS_REVERSE);
            return;
        }
    }

    /* any codes in the non printable range should produce no output at all.
     * FIXME: confirm this on a real printer */
    if ((c <= 0x1f) || ((c >= 0x80) && (c <= 0x9f)))  {
        return;
    }

    print_cbm_char(mps, c);
}

/* ------------------------------------------------------------------------- */
/* Interface to the upper layer.  */

static int drv_common_open(unsigned int prnr, unsigned int secondary)
{
    switch (secondary) {
        case  0: /* Print data exactly as received (mps801,mps803: Select graphic mode) */
            /* set_chargen_mode(&drv_mps803[prnr], secondary, MPS_GRAPHICS); */
            set_global_mode(&drv_mps803[prnr], MPS_CRSRUP);
            break;
        case  5: /* define user programmable character bitmap (character 254) */
            drv_mps803[prnr].custom_char_ptr = 0;
            memset(&drv_mps803[prnr].custom_char[0], 0, 8 * sizeof(int)); /* probably not what really happens */
            break;
        case  7: /* Select business mode */
            /* set_chargen_mode(&drv_mps803[prnr], secondary, MPS_BUSINESS); */
            unset_global_mode(&drv_mps803[prnr], MPS_CRSRUP);
            break;
        case  8: /* Select graphic mode */
            /* set_chargen_mode(&drv_mps803[prnr], secondary, MPS_GRAPHICS); */
            set_global_mode(&drv_mps803[prnr], MPS_CRSRUP);
            break;
        case  1: /* Print data in previously defined format */
        case  2: /* save format data */
        case  3: /* set lines per page */
        case  4: /* Format Error request */
        case  6: /* Setting spacing between lines */
        case  9: /* prevent error messages */
        case 10: /* Reset the printer */

        case 11: /* Set unidirectional printing. */
        case 12: /* Set bidirectional printing. */
        case 13: /* Set 15 cpi. (condense mode) */
        case 14: /* Set 10 cpi. (reset condense mode) */
        case 15: /* Enable correspondence (overstrike, pseudo letter quality) mode. */

        case 17: /* Print bit image graphics. */
        case 18: /* Print received bit image graphics again. */

        case 21: /* Disable correspondence (overstrike, pseudo letter quality) mode.
                    (To disable send on 21, then 14) */

        default:
            /* any other secondary address */
            log_warning(LOG_DEFAULT, "FIXME: opening secondary address %u not implemented yet.", secondary);
            break;
    }

    return 0;
}

static int drv_2022_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;

        DBG(("drv_2022_open(%u,DRIVER_FIRST_OPEN)", prnr));
        output_parameter.maxcol = 80 * 6;
        output_parameter.maxrow = 66 * 7;
        output_parameter.dpi_x = 72;
        output_parameter.dpi_y = 72;
        output_parameter.palette = palette;

        /* init_charset_2022(&drv_mps803[prnr], C2022_ROM_NAME, C2022_ROM_SIZE); */    /* 7x6 */
        if (sysfile_load(C2022_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, C2022_ROM_SIZE, C2022_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer ROM '%s'.", C2022_ROM_NAME);
            return -1;
        }
        convert_rom_char_192(&drv_mps803[prnr], 0);
#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif
        drv_mps803[prnr].char_height = 7;
        drv_mps803[prnr].char_width = 6;
        drv_mps803[prnr].page_width_dots = 80 * 6;
        drv_mps803[prnr].page_height_dots = 66 * 7;
        drv_mps803[prnr].model_sa_features = SA_FEATURE_PROG_CHAR_254;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_OLD_ENHANCE | CTRL_FEATURE_CR_WITHOUT_LF;
        drv_mps803[prnr].lookup_method = 2;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_2022_select(unsigned int prnr)
{
    DBG(("drv_2022_select(%u)", prnr));
    output_select_close(prnr);
    return drv_2022_open(prnr, DRIVER_FIRST_OPEN);
}

static int drv_4023_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        DBG(("drv_4023_open(%u,DRIVER_FIRST_OPEN)", prnr));

        output_parameter.maxcol = 80 * 8;
        output_parameter.maxrow = 66 * 8;
        output_parameter.dpi_x = 72;
        output_parameter.dpi_y = 72;
        output_parameter.palette = palette;

        /* init_charset_4023(&drv_mps803[prnr], C4023_ROM_NAME, C4023_ROM_SIZE); */    /* 8x8 */
        if (sysfile_load(C4023_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, C4023_ROM_SIZE, C4023_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer ROM '%s'.", C4023_ROM_NAME);
            return -1;
        }
        convert_rom_char_192(&drv_mps803[prnr], 1024);

#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif
        drv_mps803[prnr].char_height = 8;
        drv_mps803[prnr].char_width = 8;
        drv_mps803[prnr].page_width_dots = 80 * 8;
        drv_mps803[prnr].page_height_dots = 66 * 8;
        drv_mps803[prnr].model_sa_features = SA_FEATURE_PROG_CHAR_254;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_OLD_ENHANCE | CTRL_FEATURE_CR_WITHOUT_LF;
        drv_mps803[prnr].lookup_method = 1;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_4023_select(unsigned int prnr)
{
    DBG(("drv_4023_select(%u)", prnr));
    output_select_close(prnr);
    return drv_4023_open(prnr, DRIVER_FIRST_OPEN);
}

static int drv_8023_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        DBG(("drv_8023_open(%u,DRIVER_FIRST_OPEN)", prnr));

        output_parameter.maxcol = 136 * 6;
        output_parameter.maxrow = 66 * 7;
        output_parameter.dpi_x = 72;
        output_parameter.dpi_y = 72;
        output_parameter.palette = palette;

        /* init_charset_8023(&drv_mps803[prnr], C8023_ROM_NAME, C8023_ROM_SIZE); */    /* 8x8 */
        if (sysfile_load(C8023_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, C8023_ROM_SIZE, C8023_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer ROM '%s'.", C8023_ROM_NAME);
            return -1;
        }
        convert_rom_char_192(&drv_mps803[prnr], 0);
#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif

        drv_mps803[prnr].char_height = 7;
        drv_mps803[prnr].char_width = 6;
        drv_mps803[prnr].page_width_dots = 136 * 6;
        drv_mps803[prnr].page_height_dots = 66 * 7;
        drv_mps803[prnr].model_sa_features = SA_FEATURE_PROG_CHAR_254;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_OLD_ENHANCE | CTRL_FEATURE_CR_WITHOUT_LF;
        drv_mps803[prnr].lookup_method = 1;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_8023_select(unsigned int prnr)
{
    DBG(("drv_8023_select(%u)", prnr));
    output_select_close(prnr);
    return drv_8023_open(prnr, DRIVER_FIRST_OPEN);
}

static int drv_mps801_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        DBG(("drv_mps801_open(%u,DRIVER_FIRST_OPEN)", prnr));

        output_parameter.maxcol = 80 * 6;
        output_parameter.maxrow = 66 * 7;
        output_parameter.dpi_x = 60;
        output_parameter.dpi_y = 72;
        output_parameter.palette = palette;

        /* init_charset_mps801(&drv_mps803[prnr], MPS801_ROM_NAME, MPS801_ROM_SIZE); */    /* 8x8 */
        if (sysfile_load(MPS801_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, MPS801_ROM_SIZE, MPS801_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer ROM '%s'.", MPS801_ROM_NAME);
            return -1;
        }

        convert_rom_char_mps801(&drv_mps803[prnr], 0x800);
#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif

        drv_mps803[prnr].char_height = 7;
        drv_mps803[prnr].char_width = 6;
        drv_mps803[prnr].page_width_dots = 80 * 6;
        drv_mps803[prnr].page_height_dots = 66 * 7;
        drv_mps803[prnr].model_sa_features = 0;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_NEW_ENHANCE | CTRL_FEATURE_BIT_IMAGE_PRINTING;
        drv_mps803[prnr].lookup_method = 0;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_mps801_select(unsigned int prnr)
{
    DBG(("drv_mps801_select(%u)", prnr));
    output_select_close(prnr);
    return drv_mps801_open(prnr, DRIVER_FIRST_OPEN);
}

static int drv_mps802_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        DBG(("drv_mps802_open(%u,DRIVER_FIRST_OPEN)", prnr));

        output_parameter.maxcol = 80 * 8;
        output_parameter.maxrow = 66 * 8;
        output_parameter.dpi_x = 72;
        output_parameter.dpi_y = 72;
        output_parameter.palette = palette;

        /* init_charset_mps802(&drv_mps803[prnr], MPS802_ROM_NAME, MPS802_ROM_SIZE); */    /* 8x8 */
        if (sysfile_load(MPS802_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, MPS802_ROM_SIZE, MPS802_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer ROM '%s'.", MPS802_ROM_NAME);
            return -1;
        }
        convert_rom_char_192(&drv_mps803[prnr], 1024);
#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif
        drv_mps803[prnr].char_height = 8;
        drv_mps803[prnr].char_width = 8;
        drv_mps803[prnr].page_width_dots = 80 * 8;
        drv_mps803[prnr].page_height_dots = 66 * 8;
        drv_mps803[prnr].model_sa_features = SA_FEATURE_PROG_CHAR_254;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_CR_WITHOUT_LF;
        drv_mps803[prnr].lookup_method = 1;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_mps802_select(unsigned int prnr)
{
    DBG(("drv_mps802_select(%u)", prnr));
    output_select_close(prnr);
    return drv_mps802_open(prnr, DRIVER_FIRST_OPEN);
}

static int drv_mps803_open(unsigned int prnr, unsigned int secondary)
{
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;

        DBG(("drv_mps803_open(%u,DRIVER_FIRST_OPEN)", prnr));
        output_parameter.maxcol = MPS803_PAGE_WIDTH_DOTS;
        output_parameter.maxrow = MPS803_PAGE_HEIGHT_DOTS;
        output_parameter.dpi_x = 60;    /* mps803 has different horizontal & vertical dpi - see pg 49 of the manual part H. */
        output_parameter.dpi_y = 72;    /* NOTE - mixed dpi might not be liked by some image viewers */
        output_parameter.palette = palette;

        /* init_charset_mps803(&drv_mps803[prnr], MPS803_ROM_NAME, MPS803_ROM_SIZE); */    /* 7x6 */
        if (sysfile_load(MPS803_ROM_NAME, "PRINTER", drv_mps803[prnr].rom, MPS803_ROM_SIZE, MPS803_ROM_SIZE) < 0) {
            log_error(drv803_log, "Could not load printer charset '%s'.", MPS803_ROM_NAME);
            return -1;
        }

        convert_rom_char_160(&drv_mps803[prnr], 0xc3f);
#ifdef DEBUG_MPS803
        dump_printer_charset(&drv_mps803[prnr]);
#endif
        drv_mps803[prnr].char_height = 7;
        drv_mps803[prnr].char_width = MPS803_BYTES_PER_CHAR;
        drv_mps803[prnr].page_width_dots = MPS803_PAGE_WIDTH_DOTS;
        drv_mps803[prnr].page_height_dots = MPS803_PAGE_HEIGHT_DOTS;
        drv_mps803[prnr].model_sa_features = 0;
        drv_mps803[prnr].model_ctrl_features = CTRL_FEATURE_NEW_ENHANCE | CTRL_FEATURE_BIT_IMAGE_PRINTING;
        drv_mps803[prnr].lookup_method = 0;
        return output_select_open(prnr, &output_parameter);
    }
    return drv_common_open(prnr, secondary);
}

static int drv_mps803_select(unsigned int prnr)
{
    DBG(("drv_mps803_select(%u)", prnr));
    output_select_close(prnr);
    return drv_mps803_open(prnr, DRIVER_FIRST_OPEN);
}

/* ------------------------------------------------------------------------- */

static void drv_mps803_close(unsigned int prnr, unsigned int secondary)
{
    DBG(("drv_mps803_close(%u,%u)", prnr, secondary));
    output_select_close(prnr);
}

/*
 * We would like to have calls for LISTEN and UNLISTEN as well...
 * this may be important for emulating the proper cursor up/down
 * mode associated with SA=0 or 7.
 */

static int drv_mps803_putc(unsigned int prnr, unsigned int secondary, uint8_t b)
{
    /* DBG(("drv_mps803_putc(%u,%u:$%02x)", prnr, secondary, b)); */
    mps_engine_putc(&drv_mps803[prnr], prnr, secondary, b);
    return 0;
}

static int drv_mps803_getc(unsigned int prnr, unsigned int secondary, uint8_t *b)
{
    DBG(("drv_mps803_getc(%u,%u)", prnr, secondary));
    return output_select_getc(prnr, b);
}

static int drv_mps803_flush(unsigned int prnr, unsigned int secondary)
{
    /* DBG(("drv_mps803_flush(%u,%u)", prnr, secondary)); */
    return output_select_flush(prnr);
}

static int drv_mps803_formfeed(unsigned int prnr)
{
    DBG(("drv_mps803_formfeed(%u)", prnr));
    return output_select_formfeed(prnr);;
}


/** \brief  Register drivers
 *
 * Register printer drivers.
 *
 *  - Commodore MPS 801
 *  - Commodore MPS 802
 *  - Commodore MPS 803
 *  - Commodore 2022
 *  - Commodore 4034
 *  - Commodore 8034
 *
 * \return  0
 */
int drv_mps803_init_resources(void)
{
    driver_select_t driver_select;

    /* Commodore MPS 801 */
    driver_select = (driver_select_t){
        .drv_name     = "mps801",
        .ui_name      = "Commodore MPS 801",
        .drv_select   = drv_mps801_select,
        .drv_open     = drv_mps801_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = true,
        .ieee488      = false,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    /* Commodore MPS 802 */
    driver_select     = (driver_select_t){
        .drv_name     = "mps802",
        .ui_name      = "Commodore MPS 802",
        .drv_select   = drv_mps802_select,
        .drv_open     = drv_mps802_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = true,
        .ieee488      = false,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    /* Commodore MPS 803 */
    driver_select = (driver_select_t){
        .drv_name     = "mps803",
        .ui_name      = "Commodore MPS 803",
        .drv_select   = drv_mps803_select,
        .drv_open     = drv_mps803_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = true,
        .ieee488      = false,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    /* Commodore 2022 */
    driver_select = (driver_select_t){

        .drv_name     = "2022",
        .ui_name      = "Commodore 2022",
        .drv_select   = drv_2022_select,
        .drv_open     = drv_2022_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = false,
        .ieee488      = true,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    /* Commodore 4023 */
    driver_select = (driver_select_t){
        .drv_name     = "4023",
        .ui_name      = "Commodore 4023",
        .drv_select   = drv_4023_select,
        .drv_open     = drv_4023_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = false,
        .ieee488      = true,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    /* Commodore 8023 */
    driver_select = (driver_select_t){
        .drv_name     = "8023",
        .ui_name      = "Commodore 8023",
        .drv_select   = drv_8023_select,
        .drv_open     = drv_8023_open,

        .drv_close    = drv_mps803_close,
        .drv_putc     = drv_mps803_putc,
        .drv_getc     = drv_mps803_getc,
        .drv_flush    = drv_mps803_flush,
        .drv_formfeed = drv_mps803_formfeed,

        .printer      = true,
        .plotter      = false,
        .iec          = false,
        .ieee488      = true,
        .userport     = false,
        .text         = true,
        .graphics     = true
    };
    driver_select_register(&driver_select);

    return 0;
}

int drv_mps803_init(void)
{
    const char *color_names[2] = {"Black", "White"};

    drv803_log = log_open("MPS");

    /* FIXME: rename the palette somehow? */
    palette = palette_create(2, color_names);

    if (palette == NULL) {
        return -1;
    }

    if (palette_load("mps803.vpl", "PRINTER", palette) < 0) {
        log_error(drv803_log, "Cannot load palette file `%s'.",
                  "mps803.vpl");
        return -1;
    }

    return 0;
}

void drv_mps803_shutdown(void)
{
    DBG(("drv_mps803_shutdown"));
    palette_free(palette);
}
