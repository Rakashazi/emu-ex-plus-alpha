/*
 * drv-nl10.c - NL10 printer driver.
 *
 * Written by
 *  David Hansel <david@hansels.net>
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "driver-select.h"
#include "drv-nl10.h"
#include "log.h"
#include "output-select.h"
#include "output.h"
#include "palette.h"
#include "sysfile.h"
#include "types.h"
#include "lib.h"

/* MAX_COL must be a multiple of 32 */
/* 2432 x 3172 */
#define BORDERX 16
#define BORDERY 2
#define MAX_COL (80 * 30 + 2 * BORDERX)
#define MAX_ROW (66 * 48 + 2 * BORDERY)
#define BUF_ROW (4 * 4 * 9 + 1)
#define CHARSET_SIZE       200
#define NL10_ROM_SIZE      0x8000

#define NL10_NLQ           0x00001
#define NL10_ELITE         0x00002
#define NL10_CONDENSED     0x00004
#define NL10_EXPANDED      0x00008
#define NL10_EXPANDED_LINE 0x00010
#define NL10_UNDERLINE     0x00020
#define NL10_SUPERSCRIPT   0x00040
#define NL10_SUBSCRIPT     0x00080
#define NL10_ITALIC        0x00100
#define NL10_BOLD          0x00200
#define NL10_EMPHASIZE     0x00400
#define NL10_PROP          0x00800
#define NL10_USERAM        0x01000
#define NL10_ASCII         0x02000
#define NL10_CBMTEXT       0x04000
#define NL10_REVERSE       0x08000
#define NL10_QUOTED        0x10000
#define NL10_ZERO_CROSSED  0x20000

#define NL10_GFX_OFF       0x00
#define NL10_GFX_SINGLE    0x01
#define NL10_GFX_DOUBLE    0x02
#define NL10_GFX_QUAD      0x03
#define NL10_GFX_CRT       0x04
#define NL10_GFX_PLOT      0x05
#define NL10_GFX_CRT2      0x06
#define NL10_GFX_REVERSE   0x40
#define NL10_GFX_7PIN      0x80

#define NL10_ESCBUF_SIZE   60

typedef struct nl10_s {
    BYTE esc[NL10_ESCBUF_SIZE], esc_ctr;
    BYTE line[BUF_ROW][MAX_COL];
    BYTE htabs[41], vtabs[41], macro[16];
    BYTE mapping[256];
    BYTE *char_ram, *char_ram_nlq;
    BYTE expand, expand_half;

    int marg_l, marg_r, marg_t, marg_b;
    int mapping_intl_id;
    int pos_x, pos_y, pos_y_pix;
    int col_nr, line_nr;
    int isopen, mode, gfx_mode, gfx_count;
    int linespace; /* in 1/216 inch */
} nl10_t;


static palette_t *palette = NULL;

/* Logging goes here.  */
static log_t drvnl10_log = LOG_ERR;

#ifdef USE_EMBEDDED
#include "printernl10cbm.h"
#else
static BYTE drv_nl10_rom[NL10_ROM_SIZE];
#endif

static BYTE *drv_nl10_charset = drv_nl10_rom;
static BYTE drv_nl10_charset_nlq[CHARSET_SIZE * 47];
static BYTE drv_nl10_charset_nlq_italic[CHARSET_SIZE * 47];

STATIC_PROTOTYPE const BYTE drv_nl10_charset_mapping_intl[3][8][14];
STATIC_PROTOTYPE const BYTE drv_nl10_charset_mapping[3][256];

static int drv_nl10_init_charset(void);
static int handle_control_sequence(nl10_t *nl10, unsigned int prnr, const BYTE c);
static int handle_esc_control_sequence(nl10_t *nl10, unsigned int prnr, const BYTE c);

static nl10_t drv_nl10[NUM_OUTPUT_SELECT];


/* ------------------------------------------------------------------------- */
/* NL-10 printer engine. */

static inline void set_mode(nl10_t *nl10, unsigned int m)
{
    nl10->mode |= m;
}

static inline void del_mode(nl10_t *nl10, unsigned int m)
{
    nl10->mode &= ~m;
}

static inline int is_mode(nl10_t *nl10, unsigned int m)
{
    return nl10->mode & m;
}


static BYTE *get_char_data(nl10_t *nl10, BYTE c)
{
    BYTE *data;

    if (nl10->mapping[c] == 0xff) {
        data = NULL;
    } else if (is_mode(nl10, NL10_NLQ)) {
        if (is_mode(nl10, NL10_USERAM) && c >= 32 && c <= 127) {
            data = nl10->char_ram_nlq + (c - 32) * 47;
        } else if (is_mode(nl10, NL10_ITALIC)) {
            data = drv_nl10_charset_nlq_italic + nl10->mapping[c] * 47;
        } else {
            data = drv_nl10_charset_nlq + nl10->mapping[c] * 47;
        }
    } else {
        if (is_mode(nl10, NL10_USERAM) && c >= 32 && c <= 127) {
            data = nl10->char_ram + (c - 32) * 12;
        } else {
            data = drv_nl10_charset + nl10->mapping[c] * 12;
        }
    }

    return data;
}


static double get_char_width(nl10_t *nl10, BYTE c, int no_prop)
{
    BYTE *data = get_char_data(nl10, c);
    double w;

    if (data == NULL) {
        return 0;
    } else if (is_mode(nl10, NL10_NLQ)) {
        w = 30;
    } else if (is_mode(nl10, NL10_ELITE)) {
        w = is_mode(nl10, NL10_CONDENSED) ? 15 : 25;
    } else {
        w = is_mode(nl10, NL10_CONDENSED) ? 17.5 : 30;
    }

    if (!no_prop && is_mode(nl10, NL10_PROP) && !is_mode(nl10, NL10_NLQ)) {
        w = (w / 11.0) * ((data[0] & 15) - ((data[0] >> 4) & 7)) + 1;
    }

    return w * (is_mode(nl10, NL10_EXPANDED | NL10_EXPANDED_LINE) ? 2 : 1) * nl10->expand;
}


static void init_mapping(nl10_t *nl10, int intl)
{
    int mapping;
    if (is_mode(nl10, NL10_ASCII)) {
        mapping = 0;
    } else if (is_mode(nl10, NL10_CBMTEXT)) {
        mapping = 2;
    } else {
        mapping = 1;
    }

    nl10->mapping_intl_id = intl;

    memcpy(nl10->mapping, drv_nl10_charset_mapping[mapping], 256);
    nl10->mapping[0x23] = drv_nl10_charset_mapping_intl[mapping][intl][0];
    nl10->mapping[0x24] = drv_nl10_charset_mapping_intl[mapping][intl][1];
    nl10->mapping[0x40] = drv_nl10_charset_mapping_intl[mapping][intl][2];
    nl10->mapping[0x5b] = drv_nl10_charset_mapping_intl[mapping][intl][3];
    nl10->mapping[0x5c] = drv_nl10_charset_mapping_intl[mapping][intl][4];
    nl10->mapping[0x5d] = drv_nl10_charset_mapping_intl[mapping][intl][5];
    nl10->mapping[0x7b] = drv_nl10_charset_mapping_intl[mapping][intl][6];
    nl10->mapping[0x7c] = drv_nl10_charset_mapping_intl[mapping][intl][7];
    nl10->mapping[0x7d] = drv_nl10_charset_mapping_intl[mapping][intl][8];
    nl10->mapping[0x7e] = drv_nl10_charset_mapping_intl[mapping][intl][9];
    nl10->mapping[0xdb] = drv_nl10_charset_mapping_intl[mapping][intl][10];
    nl10->mapping[0xdc] = drv_nl10_charset_mapping_intl[mapping][intl][11];
    nl10->mapping[0xdd] = drv_nl10_charset_mapping_intl[mapping][intl][12];
    nl10->mapping[0xde] = drv_nl10_charset_mapping_intl[mapping][intl][13];

    if (is_mode(nl10, NL10_ZERO_CROSSED)) {
        nl10->mapping[0x30] = 0x1f;
    }
}


static void reset(nl10_t *nl10)
{
    int i;
    memset(nl10->line, 0, MAX_COL * BUF_ROW);

    nl10->line_nr = 1;
    nl10->linespace = 12 * 3;
    nl10->mode = 0;
    nl10->gfx_mode = 0;
    nl10->col_nr = 0;
    nl10->expand = 1;
    nl10->marg_l = BORDERX;
    nl10->marg_r = MAX_COL - BORDERX;
    nl10->marg_t = 0;
    nl10->marg_b = 0;
    nl10->pos_x = nl10->marg_l;
    /* init_mapping(nl10, 0); */

    for (i = 0; i < 40; i++)
    {
        nl10->htabs[i] = 8 * (i + 1);
        nl10->vtabs[i] = 0;
    }

    nl10->htabs[40] = 0;
    nl10->vtabs[40] = 0;
}


static void reset_hard(nl10_t *nl10)
{
    reset(nl10);
    memset(nl10->char_ram, 0, 12 * 96);
    memset(nl10->char_ram_nlq, 0, 47 * 96);
}


static int store_char(BYTE *dest, const BYTE *src)
{
    BYTE c, r;
    int ret = 0, s = (src[0] >> 4) & 7, e = src[0] & 15;

    if (s < 0 || s > 7) {
        log_warning(drvnl10_log, "Illegal prop-start value: %u\n", s);
    } else if (e < 4 || e > 11) {
        log_warning(drvnl10_log, "Illegal prop-end value: %u\n", e);
    } else if ((e - s) < 4) {
        log_warning(drvnl10_log, "Illegal character width: (s=%u, e=%u)\n", s, e);
    } else {
        ret = 1;
    }

    dest[0] = ret ? src[0] : ((src[0] & 0x80) | 10);
    for (c = 0; c < 11; c++)
    {
        dest[c + 1] = src[c + 1];
        if (c != 0) {
            for (r = 0; r < 8; r++) {
                if ((dest[c] & (1 << r)) && (dest[c + 1] & (1 << r))) {
                    log_warning(drvnl10_log, "Illegal dot col=%u, row=%u\n", c + 1, r + 1);
                    dest[c + 1] = dest[c + 1] & ~(1 << r);
                    ret = 0;
                }
            }
        }
    }

    return ret;
}


static int store_char_nlq(BYTE *dest, const BYTE *src)
{
    BYTE c, r;
    int ret = 1;

    dest[0] = src[0];
    for (c = 0; c < 46; c++)
    {
        dest[c + 1] = src[c + 1];
        if (c != 0 && c != 23) {
            for (r = 0; r < 8; r++) {
                if ((dest[c] & (1 << r)) && (dest[c + 1] & (1 << r))) {
                    log_warning(drvnl10_log, "Illegal dot col=%u, row=%u\n", c + 1, r + 1);
                    dest[c + 1] = dest[c + 1] & ~(1 << r);
                    ret = 0;
                }
            }
        }
    }

    return ret;
}


static inline int inc_y(nl10_t *nl10)
{
    switch ((nl10->pos_y++) % 3) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 1;
    }

    return 0;
}


static void linefeed(nl10_t *nl10, unsigned int prnr)
{
    int c, i, j;

    for (i = 0; i < nl10->linespace; i++) {
        for (j = inc_y(nl10); j > 0; j--) {
            while (nl10->pos_y_pix < BORDERY) {
                output_select_putc(prnr, (BYTE)(OUTPUT_NEWLINE));
                nl10->pos_y_pix++;
            }

            /* output topmost row */
            for (c = 0; c < MAX_COL; c++) {
                output_select_putc(prnr, (BYTE)(nl10->line[0][c] ? OUTPUT_PIXEL_BLACK : OUTPUT_PIXEL_WHITE));
            }
            output_select_putc(prnr, (BYTE)(OUTPUT_NEWLINE));

            /* move everything else one row up */
            memmove(nl10->line[0], nl10->line[1], (BUF_ROW - 1) * MAX_COL * sizeof(BYTE));

            /* clear bottom row */
            memset(nl10->line[BUF_ROW - 1], 0, MAX_COL * sizeof(BYTE));

            /* increase pixel row count */
            nl10->pos_y_pix++;

            /* check end-of-page */
            if (nl10->pos_y_pix >= MAX_ROW - BORDERY) {
                while (nl10->pos_y_pix++ < MAX_ROW) {
                    output_select_putc(prnr, (BYTE)(OUTPUT_NEWLINE));
                }
                nl10->line_nr = 0;
                nl10->pos_y = 0;
                nl10->pos_y_pix = 0;
            }
        }
    }

    nl10->line_nr++;
}


static void output_buf(nl10_t *nl10, unsigned int prnr)
{
    int r, c;

    /* output buffer */
    for (r = 0; r < BUF_ROW; r++) {
        for (c = 0; c < MAX_COL; c++) {
            output_select_putc(prnr, (BYTE)(drv_nl10[prnr].line[r][c] ? OUTPUT_PIXEL_BLACK : OUTPUT_PIXEL_WHITE));
        }
        output_select_putc(prnr, (BYTE)(OUTPUT_NEWLINE));
    }

    /* clear buffer */
    memset(nl10->line, 0, BUF_ROW * MAX_COL * sizeof(BYTE));

    nl10->pos_y += (BUF_ROW / 4 * 3);
    nl10->pos_y_pix += BUF_ROW;
}


static void formfeed(nl10_t *nl10, unsigned int prnr)
{
    int r;
    output_buf(nl10, prnr);
    for (r = nl10->pos_y_pix; r < MAX_ROW; r++) {
        output_select_putc(prnr, (BYTE)(OUTPUT_NEWLINE));
    }
    nl10->line_nr = 1;
    nl10->pos_y = 0;
    nl10->pos_y_pix = 0;
}


inline static void draw_point2(nl10_t *nl10, int x, int y)
{
/*
   **
   #*
   **
*/

    nl10->line[y][x] = 1;
    nl10->line[y][x + 1] = 1;
    nl10->line[y + 1][x] = 1;
    nl10->line[y - 1][x] = 1;
    nl10->line[y + 1][x + 1] = 1;
    nl10->line[y - 1][x + 1] = 1;
}

inline static void draw_point3(nl10_t *nl10, int x, int y)
{
/*
    *
   *#*
    *
*/

    nl10->line[y][x] = 1;
    nl10->line[y][x - 1] = 1;
    nl10->line[y][x + 1] = 1;
    nl10->line[y - 1][x] = 1;
    nl10->line[y + 1][x] = 1;
}

static void draw_char_nlq(nl10_t *nl10, const BYTE c)
{
/*
     NLQ (80 char per line, 30 pixels per char):
      0         1         2
      012345678901234567890123456789
      ** * ** * ** * ** * ** * ** *
      #**#*#**#*#**#*#**#*#**#*#**#*
      ** * ** * ** * ** * ** * ** *

       ** * ** * ** * ** * ** * **
       #**#*#**#*#**#*#**#*#**#*#*
       ** * ** * ** * ** * ** * **
*/

    int i, j, k, n, rs, re, underline, expanded;
    BYTE *cdata = get_char_data(nl10, c);

    if (cdata) {
        int xs = nl10->pos_x;
        BYTE desc = (cdata[0] & 128) ? 0 : 1;

        underline = is_mode(nl10, NL10_UNDERLINE) ? 1 : 0;
        expanded = (is_mode(nl10, NL10_EXPANDED | NL10_EXPANDED_LINE) ? 2 : 1) * nl10->expand;

        if (nl10->expand_half == 0) {
            rs = 0; re = 16;
        } else if (nl10->expand_half == 1) {
            rs = 0; re = 8;
        } else if (nl10->expand_half == 2) {
            rs = 8; re = 16;
        } else {
            rs = 0; re = 16;
        }

        for (i = 0; i < 23; i++) {
            for (k = 0; k < expanded; k++)
            {
                WORD data;
                data = ((cdata[i + 1] & 0x01 ? 0x0002 : 0) + (cdata[i + 24] & 0x01 ? 0x0001 : 0) +
                        (cdata[i + 1] & 0x02 ? 0x0008 : 0) + (cdata[i + 24] & 0x02 ? 0x0004 : 0) +
                        (cdata[i + 1] & 0x04 ? 0x0020 : 0) + (cdata[i + 24] & 0x04 ? 0x0010 : 0) +
                        (cdata[i + 1] & 0x08 ? 0x0080 : 0) + (cdata[i + 24] & 0x08 ? 0x0040 : 0) +
                        (cdata[i + 1] & 0x10 ? 0x0200 : 0) + (cdata[i + 24] & 0x10 ? 0x0100 : 0) +
                        (cdata[i + 1] & 0x20 ? 0x0800 : 0) + (cdata[i + 24] & 0x20 ? 0x0400 : 0) +
                        (cdata[i + 1] & 0x40 ? 0x2000 : 0) + (cdata[i + 24] & 0x40 ? 0x1000 : 0) +
                        (cdata[i + 1] & 0x80 ? 0x8000 : 0) + (cdata[i + 24] & 0x80 ? 0x4000 : 0));

                for (j = rs; j < re; j++) {
                    for (n = 0; n < nl10->expand; n++)
                    {
                        if (underline && (j + desc) == 16 && n == 0) {
                        } else if (data & (1 << (15 - j))) {
                            if (i & 2 || expanded > 1) {
                                draw_point3(nl10, nl10->pos_x - expanded / 2 + k, ((j + desc) * nl10->expand + n) * 2 + 1);
                            } else {
                                draw_point2(nl10, nl10->pos_x - expanded / 2 + k, ((j + desc) * nl10->expand + n) * 2 + 1);
                            }
                        }
                    }
                }

                nl10->pos_x += ((i * expanded + k) % 4) == 1 ? 2 : 1;
            }
        }

        nl10->pos_x += expanded;

        if (underline) {
            for (i = xs; i < nl10->pos_x; i++) {
                if ((i & 3) == 1) {
                    draw_point2(nl10, i, (8 * nl10->expand) * 4 + 1);
                }
            }
        }
    }
}

static void draw_char_draft(nl10_t *nl10, const BYTE c)
{
/*
     Pica (80 char per line, 30 pixels per char):
      0         1         2
      012345678901234567890123456789
       *    *    *    *    *    *
      *#*  *#*  *#*  *#*  *#*  *#*
       *    *    *    *    *    *

         **   **   **   **   **
         #*   #*   #*   #*   #*
         **   **   **   **   **

     Elite (96 char per line, 25 pixels per char):
      0         1         2
      0123456789012345678901234
       *   *   *   *   *   *
      *#* *#* *#* *#* *#* *#*
       *   *   *   *   *   *

         *   *   *   *   *
        *#* *#* *#* *#* *#*
         *   *   *   *   *

     Condensed (136 char per line, 17 3/5 pixels per char):
      0         1
      01234567890123456
      ** ** ** ** ** **
      #* #* #* #* #* #*
      ** ** ** ** ** **

        *  *  *  *  *
       *#**#**#**#**#*
        *  *  *  *  *
*/

    int i, j, k, l, m, n, cs, ce, rs, re, expanded, condensed, pinspace, pinoffset;
    int bold, emphasize, underline, elite;
    BYTE desc, *cdata = get_char_data(nl10, c);

    if (cdata) {
        int xs = nl10->pos_x;

        elite = is_mode(nl10, NL10_ELITE) ? 1 : 0;
        expanded = (is_mode(nl10, NL10_EXPANDED | NL10_EXPANDED_LINE) ? 2 : 1) * nl10->expand;
        underline = is_mode(nl10, NL10_UNDERLINE) ? 1 : 0;
        bold = is_mode(nl10, NL10_BOLD) ? 1 : 0;
        emphasize = is_mode(nl10, NL10_EMPHASIZE) ? 1 : 0;
        condensed = is_mode(nl10, NL10_CONDENSED) && (!emphasize) && (!bold) ? 1 : 0;
        pinoffset = is_mode(nl10, NL10_SUBSCRIPT) ? 4 * 4 : 0;
        pinspace = (is_mode(nl10, NL10_SUPERSCRIPT) || is_mode(nl10, NL10_SUBSCRIPT)) ? 2 : 4;

        if (is_mode(nl10, NL10_PROP)) {
            cs = (cdata[0] >> 4) & 7; ce = (cdata[0] & 15) - 1;
        } else {
            cs = 0; ce = 10;
        }

        if (nl10->expand_half == 0) {
            rs = 0; re = 8;
        } else if (nl10->expand_half == 1) {
            rs = 0; re = 4;
        } else if (nl10->expand_half == 2) {
            rs = 4; re = 8;
        } else {
            rs = 0; re = 8;
        }

        desc = (cdata[0] & 128) ? 0 : 1;
        for (i = cs; i <= ce; i++)
        {
            BYTE data = cdata[i + 1];
            for (j = rs; j < re; j++) {
                if (data & (1 << (7 - j))) {
                    for (l = 0; l <= emphasize; l++) {
                        for (m = 0; m <= bold; m++) {
                            for (n = 0; n < nl10->expand; n++) {
                                for (k = 0; k < expanded; k++)
                                {
                                    if (underline && (j + desc) == 8 && n == 0) {
                                    } else if (condensed) {
                                        if ((expanded == 1) && ((i + l) & 1)) {
                                            draw_point2(nl10, nl10->pos_x + 1 * l - 1, ((j + desc) * nl10->expand + n) * pinspace + pinoffset + 1 + 2 * m);
                                        } else {
                                            draw_point3(nl10, nl10->pos_x + 2 * l + 3 * k, ((j + desc) * nl10->expand + n) * pinspace + pinoffset + 1 + 2 * m);
                                        }
                                    } else if (elite) {
                                        draw_point3(nl10, nl10->pos_x + l + 2 * l + 4 * k, ((j + desc) * nl10->expand + n) * pinspace + pinoffset + 1 + 2 * m);
                                    } else {
                                        if ((expanded == 1) && ((i + l) & 1)) {
                                            draw_point2(nl10, nl10->pos_x + 2 * l, ((j + desc) * nl10->expand + n) * pinspace + pinoffset + 1 + 2 * m);
                                        } else {
                                            draw_point3(nl10, nl10->pos_x + 3 * l + 5 * k, ((j + desc) * nl10->expand + n) * pinspace + pinoffset + 1 + 2 * m);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (condensed) {
                nl10->pos_x += (expanded > 1) ? (3 * (expanded / 2)) : ((i & 1) ? 1 : 2);
            } else if (elite) {
                nl10->pos_x += 2 * expanded;
            } else {
                nl10->pos_x += (expanded > 1) ? (5 * (expanded / 2)) : ((i & 1) ? 3 : 2);
            }
        }

        if (condensed) {
            nl10->pos_x += (((nl10->col_nr % 5) & 1) ? 0 : expanded) + expanded / 2;
        } else if (elite) {
            nl10->pos_x += expanded * 3;
        } else {
            nl10->pos_x += expanded * 3 - expanded / 2;
        }

        if (underline) {
            for (i = xs; i < nl10->pos_x; i++) {
                if ((i & 3) == 1) {
                    draw_point2(nl10, i, (8 * nl10->expand) * pinspace + pinoffset + 1);
                }
            }
        }
    }
}


static void draw_char_draft_reverse(nl10_t *nl10, const BYTE c)
{
    int i, j, k, expanded;
    BYTE *cdata = get_char_data(nl10, c);

    if (cdata) {
        expanded = (is_mode(nl10, NL10_EXPANDED | NL10_EXPANDED_LINE) ? 2 : 1) * nl10->expand;

        for (i = 0; i <= 11; i++) {
            for (k = 0; k < expanded; k++) {
                for (j = 0; j < 7; j++) {
                    BYTE bit = 1 << (7 - j);

                    if ((i < 11 && (cdata[i + 1] & bit)) || (i > 0 && (cdata[i] & bit))) {
                    } else {
                        if (i & 1) {
                            draw_point2(nl10, nl10->pos_x, (j * 4) + 1);
                        } else {
                            draw_point3(nl10, nl10->pos_x, (j * 4) + 1);
                        }

                        if (i == 7) {
                            draw_point2(nl10, nl10->pos_x, (j * 4) + 1);
                        }
                    }
                }

                nl10->pos_x += ((i * expanded + k) & 1) ? 3 : 2;
            }
        }
    }
}


static void draw_char(nl10_t *nl10, const BYTE c)
{
    /*printf("draw_char %i %i\n", c, cc);*/

    if (is_mode(nl10, NL10_NLQ)) {
        if (is_mode(nl10, NL10_SUBSCRIPT | NL10_SUPERSCRIPT)) {
            int tmp = nl10->mode;
            nl10->mode = tmp & (NL10_SUBSCRIPT | NL10_SUPERSCRIPT | NL10_UNDERLINE | NL10_EXPANDED | NL10_EXPANDED_LINE);
            draw_char_draft(nl10, c);
            nl10->mode = tmp;
        } else {
            draw_char_nlq(nl10, c);
        }
    } else if (is_mode(nl10, NL10_REVERSE)) {
        draw_char_draft_reverse(nl10, c);
    } else {
        draw_char_draft(nl10, c);
    }
}


static void draw_graphics(nl10_t *nl10, BYTE c)
{
    int j;

    if (nl10->gfx_mode & NL10_GFX_7PIN) {
        switch (nl10->gfx_mode & ~(NL10_GFX_7PIN | NL10_GFX_REVERSE)) {
            case NL10_GFX_SINGLE:
                {
                    /* 480 dots per line */

                    for (j = 0; j < 7; j++) {
                        if (((c & (1 << j)) != 0) ^ ((nl10->gfx_mode & NL10_GFX_REVERSE) != 0)) {
                            draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                        }
                    }

                    nl10->pos_x += 5;
                    break;
                }

            case NL10_GFX_DOUBLE:
                {
                    /* 960 dots per line */

                    /* 01234 */
                    /* #**#* */

                    for (j = 0; j < 7; j++) {
                        if (c & (1 << j)) {
                            if (nl10->gfx_count & 1) {
                                draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            } else {
                                draw_point2(nl10, nl10->pos_x, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count & 1) ? 2 : 3;
                    nl10->gfx_count++;
                    break;
                }
        }
    } else {
        switch (nl10->gfx_mode) {
            case NL10_GFX_SINGLE:
                {
                    /* 480 dots per line */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                        }
                    }

                    nl10->pos_x += 5;
                    break;
                }

            case NL10_GFX_PLOT:
                {
                    /* 576 dots per line */

                    /* 0123456789012345678901234 */
                    /* *#** *#* *#* *#* *#* *#*  */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            if (!(nl10->gfx_count % 6)) {
                                draw_point3(nl10, nl10->pos_x + 1, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count % 6) ? 4 : 5;
                    break;
                }

            case NL10_GFX_CRT:
                {
                    /* 640 dots per line */

                    /* 012345678901234 */
                    /* #* *#* *#* *#*  */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            if ((nl10->gfx_count % 4) == 3) {
                                draw_point2(nl10, nl10->pos_x, j * 4 + 1);
                            } else {
                                draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count % 4) ? 4 : 3;
                    break;
                }

            case NL10_GFX_CRT2:
                {
                    /* 720 dots per line */

                    /* 0123456789 */
                    /* #* *#* #*  */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            if ((nl10->gfx_count % 3) == 2) {
                                draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            } else {
                                draw_point2(nl10, nl10->pos_x, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count % 3) ? 3 : 4;
                    break;
                }

            case NL10_GFX_DOUBLE:
                {
                    /* 960 dots per line */

                    /* 01234 */
                    /* #**#* */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            if (nl10->gfx_count & 1) {
                                draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            } else {
                                draw_point2(nl10, nl10->pos_x, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count & 1) ? 2 : 3;
                    break;
                }

            case NL10_GFX_QUAD:
                {
                    /* 1920 dots per line */

                    /* 01234  */
                    /* #*     */
                    /*  *#*   */
                    /*   *#*  */
                    /*    *#* */

                    for (j = 0; j < 8; j++) {
                        if (c & (1 << (7 - j))) {
                            if ((nl10->gfx_count % 4) == 0) {
                                draw_point2(nl10, nl10->pos_x, j * 4 + 1);
                            } else {
                                draw_point3(nl10, nl10->pos_x, j * 4 + 1);
                            }
                        }
                    }

                    nl10->pos_x += (nl10->gfx_count % 4) ? 1 : 2;
                    break;
                }
        }
    }
}


static void print_char(nl10_t *nl10, unsigned int prnr, const BYTE c)
{
    /* handle dot-graphics pringing */
    if (nl10->gfx_mode != NL10_GFX_OFF) {
        if (nl10->gfx_mode & NL10_GFX_7PIN) {
            /* 7-pin (CBM) mode */
            if ((nl10->esc_ctr == 0) && (c & 0x80)) {
                draw_graphics(nl10, c);
                return;
            }
        } else {
            /* 8-pin (epson) mode */
            draw_graphics(nl10, c);
            nl10->gfx_count--;
            if (nl10->gfx_count == 0) {
                nl10->gfx_mode = NL10_GFX_OFF;
            }
            return;
        }
    }

    /* handle CBM quoted-mode (print description strings for control characters,
       e.g. print "(rvs)" for character 0x12) */
    if (is_mode(nl10, NL10_QUOTED)) {
        WORD i = 0;

        /* find pointer to description string in ROM */
        if ((c >= 0x01) && (c < 0x20) && (c != 0x0d)) {
            i = ((drv_nl10_rom[0x428e + (c - 0x01) * 2] & 0x7f) << 8) + drv_nl10_rom[0x428f + (c - 0x01) * 2];
        } else if ((c >= 0x80) && (c < 0xa0)) {
            i = ((drv_nl10_rom[0x42cc + (c - 0x80) * 2] & 0x7f) << 8) + drv_nl10_rom[0x42cd + (c - 0x80) * 2];
        }

        if (i) {
            /* if found, read and print description string from ROM
               (terminated by 0xff) */
            while ((i < NL10_ROM_SIZE) && (drv_nl10_rom[i] != 0xff)) {
                print_char(nl10, prnr, drv_nl10_rom[i++]);
            }
            return;
        }
    }

    /* ensure that top margin is honored */
    while (nl10->line_nr <= nl10->marg_t) {
        linefeed(nl10, prnr);
    }

    /* ensure that left margin is honored */
    if (nl10->pos_x < nl10->marg_l) {
        nl10->pos_x = nl10->marg_l;
    }

    /* ensure that right margin is honored */
    if ((nl10->pos_x + get_char_width(nl10, c, 0)) > nl10->marg_r) {
        linefeed(nl10, prnr);
        nl10->pos_x = nl10->marg_l;
        nl10->col_nr = 0;
    }

    /* ensure that bottom margin is honored */
    if (nl10->marg_b > 0 && nl10->line_nr > ((MAX_ROW - 2 * BORDERY) / (nl10->linespace * 4 / 3) - nl10->marg_b)) {
        formfeed(nl10, prnr);
    }

    /* check if character is part of a control sequence and, if so, process it there.
       Otherwise draw the character. */
    if (!handle_control_sequence(nl10, prnr, c)) {
        if (c == '"') {
            if (is_mode(nl10, NL10_QUOTED)) {
                del_mode(nl10, NL10_QUOTED);
            } else {
                set_mode(nl10, NL10_QUOTED);
            }
        }

        draw_char(nl10, c);
        nl10->col_nr++;
    }

    /*printf("modes: esc=%i mode=%i gfx_mode=%i gfx_ctr=%i ls=%i px=%i\n", nl10->esc_ctr, nl10->mode, nl10->gfx_mode, nl10->gfx_count, nl10->linespace, nl10->pos_x);*/
}


static int handle_control_sequence(nl10_t *nl10, unsigned int prnr, const BYTE c)
{
    if (nl10->esc_ctr >= NL10_ESCBUF_SIZE) {
        /* We should never get here.  If we do then there is a bug
           in the ESC handling routine */
        log_warning(drvnl10_log, "ESC counter overflow");
        nl10->esc_ctr = 0;
    }

    nl10->esc[nl10->esc_ctr] = c;
    switch (nl10->esc[0]) {
        case 0:
            break;

        case 7:
            /* beep (NOT IMPLEMENTED) */
            break;

        case 8:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    /* ASCII: step back */
                    nl10->pos_x -= (int) get_char_width(nl10, ' ', 1);
                } else {
                    /* CBM: set single density graphics and line spacing 7/72" */
                    nl10->gfx_mode = NL10_GFX_SINGLE | NL10_GFX_7PIN;
                    nl10->linespace = 3 * 7;
                }
                break;
            }

        case 9:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    /* ASCII: horizontal tab */
                    int i;
                    double w = get_char_width(nl10, ' ', 1);
                    for (i = 0; nl10->htabs[i] > 0; i++)
                    {
                        int p = nl10->marg_l + (int) (w * nl10->htabs[i]);
                        if ((nl10->pos_x < p) && (p < nl10->marg_r)) {
                            nl10->pos_x = p;
                            break;
                        }
                    }
                } else {
                    /* CBM: set double density graphics and line spacing 7/72" */
                    nl10->gfx_mode = NL10_GFX_DOUBLE | NL10_GFX_7PIN;
                    nl10->linespace = 3 * 7;
                }

                break;
            }

        case 10:
            /* linefeed */
            linefeed(nl10, prnr);
            break;

        case 11:
            {
                /* advance to next vertical tab position */
                int i = 0;
                while ((nl10->line_nr >= nl10->vtabs[i]) && (i == 0 || (nl10->vtabs[i] > nl10->vtabs[i - 1]))) {
                    i++;
                }

                if ((nl10->vtabs[i] <= nl10->vtabs[i - 1])) {
                    /* we're past the last tab. go to top of next page */
                    formfeed(nl10, prnr);

                    /* find the first tab greater than the top margin */
                    i = 0;
                    while ((nl10->marg_t >= nl10->vtabs[i]) && (i == 0 || (nl10->vtabs[i] > nl10->vtabs[i - 1]))) {
                        i++;
                    }

                    if (nl10->vtabs[i] <= nl10->vtabs[i - 1]) {
                        /* past the last tab again => there is no valid tab */
                        i = -1;
                    }
                }

                if (i >= 0) {
                    while (nl10->line_nr < nl10->vtabs[i]) {
                        linefeed(nl10, prnr);
                    }
                }
                break;
            }

        case 12:
            /* formfeed */
            formfeed(nl10, prnr);
            break;

        case 13:
            /* carriage return */
            linefeed(nl10, prnr);
            del_mode(nl10, NL10_QUOTED | NL10_EXPANDED_LINE);
            nl10->pos_x = nl10->marg_l;
            nl10->col_nr = 0;
            break;

        case 14:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    /* ASCII: turn on expanded print for current line */
                    set_mode(nl10, NL10_EXPANDED_LINE);
                } else {
                    /* CBM: turn on expanded print (and turn off graphics printing) */
                    set_mode(nl10, NL10_EXPANDED);

                    if (nl10->gfx_mode & NL10_GFX_7PIN) {
                        nl10->gfx_mode = NL10_GFX_OFF;
                        nl10->linespace = 12 * 3;
                    }
                }

                break;
            }

        case 15:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    /* ASCII: turn on condensed print */
                    set_mode(nl10, NL10_CONDENSED);
                } else {
                    /* CBM: turn off expanded print (and turn off graphics printing) */
                    del_mode(nl10, NL10_EXPANDED);

                    if (nl10->gfx_mode & NL10_GFX_7PIN) {
                        nl10->gfx_mode = NL10_GFX_OFF;
                        nl10->linespace = 12 * 3;
                    }
                }

                break;
            }

        case 16:
            {
                /* skip to horizontal print position */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    int i = 0;
                    if ((nl10->esc[1] >= '0') && (nl10->esc[1] <= '9')) {
                        i += 10 * (nl10->esc[1] - '0');
                    }
                    if ((nl10->esc[2] >= '0') && (nl10->esc[2] <= '9')) {
                        i += 1 * (nl10->esc[2] - '0');
                    }
                    if (i > 79) {
                        i = 79;
                    }
                    nl10->pos_x = BORDERX + 30 * i;
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 17:
            set_mode(nl10, NL10_CBMTEXT);
            init_mapping(nl10, nl10->mapping_intl_id);
            break;

        case 18:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    /* ASCII: Set 'elite' print mode */
                    del_mode(nl10, NL10_ELITE);
                } else {
                    /* CBM: Enable reverse print */
                    set_mode(nl10, NL10_REVERSE);
                }
                break;
            }

        case 19:
            {
                if (!is_mode(nl10, NL10_ASCII)) {
                    /* clear top/bottom margins (only CMD mode) */
                    nl10->marg_t = 0;
                    nl10->marg_b = 0;
                }
                break;
            }

        case 20:
            {
                if (is_mode(nl10, NL10_ASCII)) {
                    set_mode(nl10, NL10_EXPANDED | NL10_EXPANDED_LINE);
                }
                break;
            }

        case 26:
            {
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    int i;
                    if ((nl10->gfx_mode & NL10_GFX_7PIN) && (nl10->esc[2] & 0x80)) {
                        for (i = 0; i < nl10->esc[1]; i++) {
                            draw_graphics(nl10, nl10->esc[2]);
                        }
                    }
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 27:
            {
                /* ESC sequence */
                if (nl10->esc_ctr < 1) {
                    nl10->esc_ctr++;
                } else {
                    return handle_esc_control_sequence(nl10, prnr, c);
                }
                break;
            }

        case 145:
            /* enable CBM text character mode */
            del_mode(nl10, NL10_CBMTEXT);
            init_mapping(nl10, nl10->mapping_intl_id);
            break;

        case 146:
            /* disable reverse printing */
            del_mode(nl10, NL10_REVERSE);
            break;

        case 147:
            {
                if (!is_mode(nl10, NL10_ASCII)) {
                    nl10->marg_b = 6;
                }
                break;
            }

        default:
            return 0;
    }

    return 1;
}


static int handle_esc_control_sequence(nl10_t *nl10, unsigned int prnr, const BYTE c)
{
    switch (nl10->esc[1]) {
        case 10:
            {
                /* reverse paper one line (NOT IMPLEMENTED) */
                log_warning(drvnl10_log, "Command 'reverse paper one line' (%i %i) not implemented.",
                            nl10->esc[0], nl10->esc[1]);
                nl10->esc_ctr = 0;
                break;
            }

        case 12:
            {
                /* reverse paper to top of page (NOT IMPLEMENTED) */
                log_warning(drvnl10_log, "Command 'reverse paper to top of page' (%i %i) not implemented.",
                            nl10->esc[0], nl10->esc[1]);
                nl10->esc_ctr = 0;
                break;
            }

        case 15:
            /* enable expanded print */
            set_mode(nl10, NL10_EXPANDED);
            break;

        case 16:
            {
                /* skip to horizontal point position */
                if (nl10->esc_ctr < 3) {
                    nl10->esc_ctr++;
                } else {
                    int i = 256 * nl10->esc[2] + nl10->esc[3];
                    if (i > 479) {
                        i = 479;
                    }
                    nl10->pos_x = BORDERX + 5 * i;
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 18:
            {
                /* CBM: enable single-density reverse graphics printing */
                if (!is_mode(nl10, NL10_ASCII)) {
                    nl10->gfx_mode = NL10_GFX_SINGLE | NL10_GFX_REVERSE | NL10_GFX_7PIN;
                }
                nl10->esc_ctr = 0;
                break;
            }

        case 25:
            {
                /* auto-feed mode control (NOT IMPLEMENTED) */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    log_warning(drvnl10_log, "Command 'auto-feed mode control' (%i %i %i) not implemented.",
                                nl10->esc[0], nl10->esc[1], nl10->esc[2]);
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 33:
            {
                /* master command for print style changes */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    del_mode(nl10, NL10_ELITE | NL10_CONDENSED | NL10_EXPANDED | NL10_UNDERLINE);
                    if (nl10->esc[2] & 1) {
                        set_mode(nl10, NL10_ELITE);
                    }
                    if (nl10->esc[2] & 2) {
                        set_mode(nl10, NL10_PROP);
                    }
                    if (nl10->esc[2] & 4) {
                        set_mode(nl10, NL10_CONDENSED);
                    }
                    if (nl10->esc[2] & 8) {
                        set_mode(nl10, NL10_EMPHASIZE);
                    }
                    if (nl10->esc[2] & 16) {
                        set_mode(nl10, NL10_BOLD);
                    }
                    if (nl10->esc[2] & 32) {
                        set_mode(nl10, NL10_EXPANDED);
                    }
                    if (nl10->esc[2] & 128) {
                        set_mode(nl10, NL10_UNDERLINE);
                    }
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 37:
            {
                /* enable/disable use of character RAM */
                if (nl10->esc_ctr < 3) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '1' || nl10->esc[2] == 1) && nl10->esc[3] == 0) {
                        set_mode(nl10, NL10_USERAM);
                    } else if ((nl10->esc[2] == '0' || nl10->esc[2] == 0) && nl10->esc[3] == 0) {
                        del_mode(nl10, NL10_USERAM);
                    }

                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 38:
            {
                /* download new character data to RAM */
                if (nl10->esc_ctr < 4 ||
                    nl10->esc_ctr < 4 + (is_mode(nl10, NL10_NLQ) ? 47 : 12)) {
                    nl10->esc_ctr++;
                } else {
                    int i;

                    i = nl10->esc[3];
                    if ((i >= 32) && (i <= 127)) {
                        if (is_mode(nl10, NL10_NLQ)) {
                            store_char_nlq(nl10->char_ram_nlq + (i - 32) * 47, nl10->esc + 5);
                        } else {
                            store_char(nl10->char_ram + (i - 32) * 12, nl10->esc + 5);
                        }
                    }

                    nl10->esc[3]++;
                    nl10->esc_ctr = (nl10->esc[3] <= nl10->esc[4]) ? 5 : 0;
                }

                break;
            }

        case 42:
            {
                /* enter graphics mode */
                if (nl10->esc_ctr < 4) {
                    nl10->esc_ctr++;
                } else {
                    nl10->gfx_mode = NL10_GFX_OFF;
                    switch (nl10->esc[2]) {
                        case 0:
                            nl10->gfx_mode = NL10_GFX_SINGLE;
                            break;
                        case 1:
                        case 2:
                            nl10->gfx_mode = NL10_GFX_DOUBLE;
                            break;
                        case 3:
                            nl10->gfx_mode = NL10_GFX_QUAD;
                            break;
                        case 4:
                            nl10->gfx_mode = NL10_GFX_CRT; break;
                        case 5:
                            nl10->gfx_mode = NL10_GFX_PLOT;
                            break;
                        case 6:
                            nl10->gfx_mode = NL10_GFX_CRT2;
                            break;
                    }

                    nl10->gfx_count = nl10->esc[3] + 256 * nl10->esc[4];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 43:
            {
                /* macro commands */
                if (nl10->esc_ctr < 3) {
                    nl10->esc_ctr++;
                } else if (nl10->esc[2] == 1) {
                    /* execute macro */
                    BYTE i;
                    for (i = 0; i < 16; i++) {
                        if (nl10->macro[i] == 30) {
                            break;
                        } else {
                            print_char(nl10, prnr, nl10->macro[i]);
                        }
                    }
                    nl10->esc_ctr = 0;
                } else if (nl10->esc_ctr < 2 + 16 && nl10->esc[nl10->esc_ctr] != 30) {
                    nl10->esc_ctr++;
                } else {
                    /* define macro */
                    BYTE i;
                    for (i = 0; i < 16; i++) {
                        nl10->macro[i] = nl10->esc[2 + i];
                    }
                    nl10->esc_ctr = 0;
                }

                break;
            }

        case 45:
            {
                /* turn underline on/off */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_UNDERLINE);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_UNDERLINE);
                    }

                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 48:
            /* line spacing 1/8" */
            nl10->linespace = 3 * 9;
            nl10->esc_ctr = 0;
            break;

        case 49:
            /* line spacing 7/72" */
            nl10->linespace = 3 * 7;
            nl10->esc_ctr = 0;
            break;

        case 50:
            /* line spacing 1/6" */
            nl10->linespace = 3 * 12;
            nl10->esc_ctr = 0;
            break;

        case 51:
            {
                /* set line spacing to n/216" */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->linespace = nl10->esc[2];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 52:
            /* Selects italic characters */
            set_mode(nl10, NL10_ITALIC);
            nl10->esc_ctr = 0;
            break;

        case 53:
            /* Cancels italic characters */
            del_mode(nl10, NL10_ITALIC);
            nl10->esc_ctr = 0;
            break;

        case 58:
            {
                /* copy character set to RAM */
                if (nl10->esc_ctr < 4) {
                    nl10->esc_ctr++;
                } else {
                    if (nl10->esc[2] == 0 && nl10->esc[3] == 0 && nl10->esc[4] == 0) {
                        int c;
                        for (c = 0; c < 96; c++) {
                            memcpy(nl10->char_ram + c * 12, drv_nl10_charset + nl10->mapping[c + 32] * 12, 12);
                            memcpy(nl10->char_ram_nlq + c * 47, drv_nl10_charset_nlq + nl10->mapping[c + 32] * 47, 47);
                        }
                    }
                    nl10->esc_ctr = 0;
                }

                break;
            }

        case 64:
            /* (Soft) Reset printer */
            reset(nl10);
            nl10->esc_ctr = 0;
            break;

        case 65:
            {
                /* line spacing n/72" */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->linespace = 3 * nl10->esc[2];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 66:
            {
                /* set vertical tabs */
                if (nl10->esc_ctr < 3 || (nl10->esc_ctr < 42 && (nl10->esc[nl10->esc_ctr] > nl10->esc[nl10->esc_ctr - 1]))) {
                    nl10->esc_ctr++;
                } else {
                    int i;
                    for (i = 2; i < nl10->esc_ctr; i++) {
                        nl10->vtabs[i - 2] = nl10->esc[i];
                    }
                    nl10->vtabs[i - 2] = 0;
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 67:
            {
                /* set page length */
                if (nl10->esc_ctr < 2 || ((nl10->esc[2] == 0) && nl10->esc_ctr < 3)) {
                    nl10->esc_ctr++;
                } else {
                    if (nl10->esc[2] == 0) {
                        /* set page length to n inches (NOT IMPLEMENTED) */
                        log_warning(drvnl10_log, "Command 'set page length to n inches' (%i %i %i %i) not implemented.",
                                    nl10->esc[0], nl10->esc[1], nl10->esc[2], nl10->esc[3]);
                        nl10->esc_ctr = 0;
                    } else {
                        /* set page length to n lines (NOT IMPLEMENTED) */
                        log_warning(drvnl10_log, "Command 'set page length to n lines' (%i %i %i) not implemented.",
                                    nl10->esc[0], nl10->esc[1], nl10->esc[2]);
                        nl10->esc_ctr = 0;
                    }
                }
                break;
            }

        case 68:
            {
                /* set horizontal tabs */
                if (nl10->esc_ctr < 3 || (nl10->esc_ctr < 42 && (nl10->esc[nl10->esc_ctr] > nl10->esc[nl10->esc_ctr - 1]))) {
                    nl10->esc_ctr++;
                } else {
                    int i;
                    for (i = 2; i < nl10->esc_ctr; i++) {
                        nl10->htabs[i - 2] = nl10->esc[i];
                    }
                    nl10->htabs[i - 2] = 0;
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 69:
            /* Selects emphasized printing */
            set_mode(nl10, NL10_EMPHASIZE);
            nl10->esc_ctr = 0;
            break;

        case 70:
            /* Cancels emphasized printing */
            del_mode(nl10, NL10_EMPHASIZE);
            nl10->esc_ctr = 0;
            break;

        case 71:
            /* Selects boldface printing */
            set_mode(nl10, NL10_BOLD);
            nl10->esc_ctr = 0;
            break;

        case 72:
            /* Cancels boldface printing */
            del_mode(nl10, NL10_BOLD);
            nl10->esc_ctr = 0;
            break;

        case 74:
            {
                /* one-time linefeed of n/216" */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    int tmp = nl10->linespace;
                    nl10->linespace = nl10->esc[2];
                    linefeed(nl10, prnr);
                    nl10->linespace = tmp;
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 75: /* Prints normal density graphics */
        case 76: /* Prints double density graphics */
        case 89: /* Prints double density graphics (double speed) */
        case 90: /* Prints quadruple density graphics */
            {
                if (nl10->esc_ctr < 3) {
                    nl10->esc_ctr++;
                } else {
                    nl10->gfx_mode = NL10_GFX_OFF;
                    switch (nl10->esc[1]) {
                        case 'K':
                            nl10->gfx_mode = NL10_GFX_SINGLE;
                            break;
                        case 'L':
                        case 'Y':
                            nl10->gfx_mode = NL10_GFX_DOUBLE;
                            break;
                        case 'Z':
                            nl10->gfx_mode = NL10_GFX_QUAD;
                            break;
                    }

                    nl10->gfx_count = nl10->esc[2] + 256 * nl10->esc[3];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 77:
            /* Sets print pitch to elite */
            set_mode(nl10, NL10_ELITE);
            nl10->esc_ctr = 0;
            break;

        case 78:
            {
                /* set bottom margin to n lines */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->marg_b = nl10->esc[2];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 79:
            /* clear top/bottom margins */
            nl10->marg_t = 0;
            nl10->marg_b = 0;
            nl10->esc_ctr = 0;
            break;

        case 80:
            /* Set print pitch to pica */
            del_mode(nl10, NL10_ELITE);
            nl10->esc_ctr = 0;
            break;

        case 81:
            {
                /* Set right margin */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->marg_r = BORDERX + (int) (get_char_width(nl10, ' ', 1) * nl10->esc[2]);
                    if (nl10->marg_r > MAX_COL - BORDERX) {
                        nl10->marg_r = MAX_COL - BORDERX;
                    }
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 82:
            {
                /* Select an international character set */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    init_mapping(nl10, nl10->esc[2]);
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 83:
            {
                /* Select superscript or subscript */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_SUBSCRIPT);
                        set_mode(nl10, NL10_SUPERSCRIPT);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_SUBSCRIPT);
                        del_mode(nl10, NL10_SUPERSCRIPT);
                    }

                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 84:
            {
                /* Cancel superscript and subscript */
                del_mode(nl10, NL10_SUPERSCRIPT | NL10_SUBSCRIPT);
                nl10->esc_ctr = 0;
                break;
            }

        case 87:
            {
                /* Select/cancel expanded print */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_EXPANDED);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_EXPANDED);
                    }

                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 93:
            {
                /* 0=CBM mode, 1=ASCII mode */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_ASCII);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_ASCII);
                    }

                    init_mapping(nl10, nl10->mapping_intl_id);
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 97:
            {
                /* set horizontal alignment (NOT IMPLEMENTED) */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    log_warning(drvnl10_log, "Command 'set horizontal alignment' (%i %i %i) not implemented.",
                                nl10->esc[0], nl10->esc[1], nl10->esc[2]);
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 104:
            {
                /* Select double/quadruple sized printing */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    switch (nl10->esc[2]) {
                        case 0:
                            nl10->expand = 1;
                            nl10->expand_half = 0;
                            break;
                        case 1:
                            nl10->expand = 2;
                            nl10->expand_half = 0;
                            break;
                        case 2:
                            nl10->expand = 4;
                            nl10->expand_half = 0;
                            break;
                        case 3:
                            nl10->expand = 2;
                            nl10->expand_half = 1;
                            break;
                        case 4:
                            nl10->expand = 4;
                            nl10->expand_half = 1;
                            break;
                        case 5:
                            nl10->expand = 2;
                            nl10->expand_half = 2;
                            break;
                        case 6:
                            nl10->expand = 4;
                            nl10->expand_half = 2;
                            break;
                    }
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 108:
            {
                /* Set left margin */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->marg_l = BORDERX + (int) (get_char_width(nl10, ' ', 1) * nl10->esc[2]);
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 112:
            {
                /* Select/cancel proportional print */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_PROP);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_PROP);
                    }

                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 114:
            {
                /* set top margin to n lines */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    nl10->marg_t = nl10->esc[2];
                    nl10->esc_ctr = 0;
                }
                break;
            }

        case 120:
            {
                /* Select/cancel NLQ mode */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_NLQ);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_NLQ);
                    }

                    nl10->esc_ctr = 0;
                }

                break;
            }

        case 126:
            {
                /* print 0 with/without slash */
                if (nl10->esc_ctr < 2) {
                    nl10->esc_ctr++;
                } else {
                    if ((nl10->esc[2] == '0') || (nl10->esc[2] == 0)) {
                        del_mode(nl10, NL10_ZERO_CROSSED);
                    } else if ((nl10->esc[2] == '1') || (nl10->esc[2] == 1)) {
                        set_mode(nl10, NL10_ZERO_CROSSED);
                    }

                    init_mapping(nl10, nl10->mapping_intl_id);
                    nl10->esc_ctr = 0;
                }

                break;
            }

        default:
            {
                log_warning(drvnl10_log, "Unsupported escape-sequence: %i %i",
                            nl10->esc[0], nl10->esc[1]);
                nl10->esc_ctr = 0;
                return 1;
            }
    }

    return 1;
}


/* ------------------------------------------------------------------------- */
/* Interface to the upper layer.  */

static int drv_nl10_open(unsigned int prnr, unsigned int secondary)
{
    nl10_t *nl10 = &(drv_nl10[prnr]);

    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;

        output_parameter.maxcol = MAX_COL;
        output_parameter.maxrow = MAX_ROW;
        output_parameter.dpi_x = 300;
        output_parameter.dpi_y = 300;
        output_parameter.palette = palette;

        drv_nl10[prnr].pos_y = 0;
        drv_nl10[prnr].pos_y_pix = 0;
        drv_nl10[prnr].isopen = 1;

        return output_select_open(prnr, &output_parameter);
    }

    if (secondary == 7) {
        set_mode(nl10, NL10_CBMTEXT);
    } else {
        del_mode(nl10, NL10_CBMTEXT);
    }

    init_mapping(nl10, drv_nl10[prnr].mapping_intl_id);

    return 0;
}

static void drv_nl10_close(unsigned int prnr, unsigned int secondary)
{
    /* cannot call output_select_close() here since it would eject the
       current page, which is not what "close"ing a channel to a real
       printer does */
    /*
    if (secondary == DRIVER_LAST_CLOSE) {
        output_select_close(prnr);
    }
    */
}

static int drv_nl10_putc(unsigned int prnr, unsigned int secondary, BYTE b)
{
    print_char(&drv_nl10[prnr], prnr, b);
    return 0;
}

static int drv_nl10_getc(unsigned int prnr, unsigned int secondary, BYTE *b)
{
    return 0x80;
}

static int drv_nl10_flush(unsigned int prnr, unsigned int secondary)
{
    return 0;
}

static int drv_nl10_formfeed(unsigned int prnr)
{
    nl10_t *nl10 = &(drv_nl10[prnr]);
    if (nl10->isopen) {
        formfeed(nl10, prnr);
    }
    return 0;
}

int drv_nl10_init_resources(void)
{
    driver_select_t driver_select;

    driver_select.drv_name = "nl10";
    driver_select.drv_open = drv_nl10_open;
    driver_select.drv_close = drv_nl10_close;
    driver_select.drv_putc = drv_nl10_putc;
    driver_select.drv_getc = drv_nl10_getc;
    driver_select.drv_flush = drv_nl10_flush;
    driver_select.drv_formfeed = drv_nl10_formfeed;

    driver_select_register(&driver_select);

    return 0;
}

int drv_nl10_init(void)
{
    int i;
    static const char *color_names[2] =
    {
        "Black", "White"
    };

    drvnl10_log = log_open("NL10");

    for (i = 0; i < NUM_OUTPUT_SELECT; i++) {
        drv_nl10[i].char_ram = lib_malloc(96 * 12);
        drv_nl10[i].char_ram_nlq = lib_malloc(96 * 47);
        reset_hard(&(drv_nl10[i]));
        drv_nl10[i].isopen = 0;
    }

    if (drv_nl10_init_charset() < 0) {
        return -1;
    }

    palette = palette_create(2, color_names);

    if (palette == NULL) {
        return -1;
    }

    if (palette_load("nl10" FSDEV_EXT_SEP_STR "vpl", palette) < 0) {
        log_error(drvnl10_log, "Cannot load palette file `%s'.",
                  "nl10" FSDEV_EXT_SEP_STR "vpl");
        return -1;
    }

    log_message(drvnl10_log, "Printer driver initialized.");

    return 0;
}

void drv_nl10_shutdown(void)
{
    int i;
    palette_free(palette);

    for (i = 0; i < NUM_OUTPUT_SELECT; i++) {
        if (drv_nl10[i].isopen) {
            output_select_close(i);
        }

        lib_free(drv_nl10[i].char_ram);
        lib_free(drv_nl10[i].char_ram_nlq);
    }
}


void drv_nl10_reset(void)
{
    int i;
    for (i = 0; i < NUM_OUTPUT_SELECT; i++) {
        reset_hard(&(drv_nl10[i]));
    }
}


/* ------------------------------------------------------------------------- */
/* character data and mappings  */


static const BYTE drv_nl10_charset_mapping_intl[3][8][14] =
{   /* ASCII */
    {
        /*    #     $     @     [     \     ]     {     |     }     ~    */
        /*   35,   36,   64,   91,   92,   93,  123,  124,  125,  126,  219,  220,  221,  222 */
        /* 0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7e, 0xdb, 0xdc, 0xdd, 0xde */

        {  0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7e, 0x81, 0x82, 0x83, 0x84}, /* CBM standard */
        {  0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7e, 0x7b, 0x7c, 0x7d, 0x7e}, /* USA */
        {  0x23, 0x24, 0x10, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x11, 0x1a, 0x1b, 0x1c, 0x11}, /* Germany */
        {  0x23, 0x24, 0x40, 0x12, 0x14, 0x0d, 0x13, 0x15, 0x0e, 0x7e, 0x13, 0x15, 0x0e, 0x7e}, /* Denmark */
        {  0x23, 0x24, 0x00, 0x05, 0x0f, 0x10, 0x1e, 0x02, 0x01, 0x16, 0x1e, 0x02, 0x01, 0x16}, /* France */
        {  0x23, 0x0b, 0x1d, 0x17, 0x18, 0x0d, 0x1a, 0x1b, 0x0e, 0x1c, 0x1a, 0x1b, 0x0e, 0x1c}, /* Sweden */
        {  0x23, 0x24, 0x40, 0x05, 0x5c, 0x1e, 0x00, 0x03, 0x01, 0x04, 0x00, 0x03, 0x01, 0x04}, /* Italy */
        {  0x0c, 0x24, 0x40, 0x07, 0x09, 0x08, 0x16, 0x0a, 0x7d, 0x7e, 0x16, 0x0a, 0x7d, 0x7e}, /* Spain */
    },

    /* CBM graphics */
    {
        {  0x23, 0x24, 0x40, 0x5b, 0x06, 0x5d, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* CBM standard */
        {  0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* USA */
        {  0x23, 0x24, 0x10, 0x17, 0x18, 0x19, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* Germany */
        {  0x23, 0x24, 0x40, 0x12, 0x14, 0x0d, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* Denmark */
        {  0x23, 0x24, 0x00, 0x05, 0x0f, 0x10, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* France */
        {  0x23, 0x0b, 0x1d, 0x17, 0x18, 0x0d, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* Sweden */
        {  0x23, 0x24, 0x40, 0x05, 0x5c, 0x1e, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* Italy */
        {  0x0c, 0x24, 0x40, 0x07, 0x09, 0x08, 0xa1, 0xa2, 0xa3, 0xa4, 0xa1, 0xa2, 0xa3, 0xa4}, /* Spain */
    },

    /* CBM text */
    {
        {  0x23, 0x24, 0x40, 0x5b, 0x06, 0x5d, 0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84}, /* CBM standard */
        {  0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7e, 0x7b, 0x7c, 0x7d, 0x7e}, /* USA */
        {  0x23, 0x24, 0x10, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x11, 0x1a, 0x1b, 0x1c, 0x11}, /* Germany */
        {  0x23, 0x24, 0x40, 0x12, 0x14, 0x0d, 0x13, 0x15, 0x0e, 0x7e, 0x13, 0x15, 0x0e, 0x7e}, /* Denmark */
        {  0x23, 0x24, 0x00, 0x05, 0x0f, 0x10, 0x1e, 0x02, 0x01, 0x16, 0x1e, 0x02, 0x01, 0x16}, /* France */
        {  0x23, 0x0b, 0x1d, 0x17, 0x18, 0x0d, 0x1a, 0x1b, 0x0e, 0x1c, 0x1a, 0x1b, 0x0e, 0x1c}, /* Sweden */
        {  0x23, 0x24, 0x40, 0x05, 0x5c, 0x1e, 0x00, 0x03, 0x01, 0x04, 0x00, 0x03, 0x01, 0x04}, /* Italy */
        {  0x0c, 0x24, 0x40, 0x07, 0x09, 0x08, 0x16, 0x0a, 0x7d, 0x7e, 0x16, 0x0a, 0x7d, 0x7e}, /* Spain */
    }
};


static const BYTE drv_nl10_charset_mapping[3][256] =
{   /* ASCII */
    {
        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* digits and punctuation */
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

        /* uppercase characters */
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,

        /* lowercase characters */
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x20,

        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* cbm graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xc7, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,

        /* uppercase characters */
        0x86, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x81, 0x82, 0x83, 0x84, 0x85,

        /* cbm graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xc7, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0x84
    },
    /* CBM graphic */
    {
        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* digits and punctuation */
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

        /* uppercase characters */
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x06, 0x5d, 0x7f, 0x80,

        /* shift-graphic symbols */
        0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
        0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,

        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* cbm-graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,

        /* shift-graphic symbols */
        0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95,
        0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,

        /* cbm-graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xa4
    },
    /* CBM text */
    {
        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* digits and punctuation */
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

        /* lowercase characters */
        0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x06, 0x5d, 0x7f, 0x80,

        /* uppercase characters */
        0x86, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x81, 0x82, 0x83, 0x84, 0x85,

        /* unprintable */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

        /* cbm-graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xc7, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5,

        /* uppercase characters */
        0x86, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x81, 0x82, 0x83, 0x84, 0x85,

        /* cbm-graphic symbols */
        0x20, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xc7, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0x84
    }
};

/* -------------------------------------------------------------------------- */

static int drv_nl10_init_charset(void)
{
    char *name = "nl10-cbm";
    int i, j;

    memset(drv_nl10_charset_nlq, 0, CHARSET_SIZE * 47);
    memset(drv_nl10_charset_nlq_italic, 0, CHARSET_SIZE * 47);

    /* load nl-10 rom file */
    if (sysfile_load(name, drv_nl10_rom, NL10_ROM_SIZE, NL10_ROM_SIZE) < 0) {
        memset(drv_nl10_rom, 0, NL10_ROM_SIZE);
        log_error(drvnl10_log, "Could not load NL-10 ROM file '%s'.", name);
        return -1;
    }

    /* check version string */
    if (memcmp(drv_nl10_rom + 0x3c7c, "STAR NL-10C VER 1.1\xff", 20) != 0) {
        log_warning(drvnl10_log, "Invalid NL-10 ROM file.");
    }

    /* init NLQ characters */
    for (i = 0; i < 129; i++) {
        /* roman */
        memcpy(drv_nl10_charset_nlq + (i * 47) + 0, drv_nl10_rom + 0x0960 + i * 24, 24);
        memcpy(drv_nl10_charset_nlq + (i * 47) + 24, drv_nl10_rom + 0x2191 + i * 24, 23);

        /* italic */
        memcpy(drv_nl10_charset_nlq_italic + (i * 47) + 0, drv_nl10_rom + 0x1578 + i * 24, 24);
        memcpy(drv_nl10_charset_nlq_italic + (i * 47) + 24, drv_nl10_rom + 0x2da9 + i * 24, 23);
    }

    /* construct nlq cbm-graphic characters from draft cbm-graphic characters */
    for (i = 129; i < CHARSET_SIZE; i++) {
        drv_nl10_charset_nlq[i * 47] = drv_nl10_charset[i * 12] & 128 ? 255 : 0;
        drv_nl10_charset_nlq_italic[i * 47] = drv_nl10_charset[i * 12] & 128 ? 255 : 0;

        for (j = 0; j < 6; j++) {
            BYTE b = drv_nl10_charset[i * 12 + j * 2 + 1];
            drv_nl10_charset_nlq[i * 47 + j * 4 + 1] = b;
            drv_nl10_charset_nlq[i * 47 + j * 4 + 3] = b;
            drv_nl10_charset_nlq[i * 47 + j * 4 + 24] = b;
            drv_nl10_charset_nlq[i * 47 + j * 4 + 26] = b;

            drv_nl10_charset_nlq_italic[i * 47 + j * 4 + 1] = b;
            drv_nl10_charset_nlq_italic[i * 47 + j * 4 + 3] = b;
            drv_nl10_charset_nlq_italic[i * 47 + j * 4 + 24] = b;
            drv_nl10_charset_nlq_italic[i * 47 + j * 4 + 26] = b;
        }
    }

    return 0;
}
