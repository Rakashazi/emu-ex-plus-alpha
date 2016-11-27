/*
 * drv-1520.c - 1520 plotter driver.
 *
 * Written by
 *  Olaf Seibert <rhialto@falu.nl>
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
#include <assert.h>
#include <math.h>

#include "archdep.h"
#include "driver-select.h"
#include "drv-1520.h"
#include "lib.h"
#include "log.h"
#include "output-select.h"
#include "output.h"
#include "palette.h"
#include "types.h"

/*#define DEBUG1520       1*/
/*#define DEBUG1520_A   1*/

/*
 * Each line segment is 0,2 mm.
 * At 150 DPI that would be 11,8 pixels.
 */

#define MAX_Y_COORD             998
#define MIN_Y_COORD             -998
#define STEPS_PER_MM            5
#define MAX_COL                 480
#define MAX_ROW                 (MAX_Y_COORD - MIN_Y_COORD + 1)
#define VERTICAL_CLEARANCE      (10 * STEPS_PER_MM)     /* 10 mm */
#define PIXELS_PER_STEP         5

#define X_PIXELS                (PIXELS_PER_STEP * (MAX_COL+1))
#define Y_PIXELS                (PIXELS_PER_STEP * (MAX_ROW+1))

#define CR                      13

/*
 * Since the plotter's y coordinates follow the usual mathematic
 * convention (positive y axis is "up"), but programs like to number
 * rows from the top down, we'll invert the logical plotter coordinates
 * to obtain array indices (which start at 0 and increase as you go down
 * the page).
 */

enum command_stage {
    start,
    letter_seen,
    x_seen,
    y_seen
};

struct plot_s {
    int prnr;
    BYTE (*sheet)[Y_PIXELS][X_PIXELS];
    int colour;         /* sa = 2 */
    int colour_accu;
    int charsize;       /* sa = 3; segment multiplication */
    int charsize_accu;
    int rotation;       /* sa = 4 */
    int rotation_accu;
    int scribe;         /* sa = 5 */
    int scribe_accu;
    int scribe_state;
    int lowercase;      /* sa = 6 */
    int lowercase_accu;
    int quote_mode;
    enum command_stage command_stage;   /* sa = 1 */
    int command;
    int command_x;
    int command_y;
    int command_flags;
    int abs_origin_x, abs_origin_y;     /* relative to start of page */
    int rel_origin_x, rel_origin_y;     /* relative to abs_origin */
    int cur_x, cur_y;                   /* relative to abs_origin */
    int lowest_y;                       /* relative to start of page */
};
typedef struct plot_s plot_t;

static plot_t drv_1520[NUM_OUTPUT_SELECT];
static palette_t *palette = NULL;

/* Logging goes here.  */
static log_t drv1520_log = LOG_ERR;

#define PIXEL_INDEX_WHITE       0
#define PIXEL_INDEX_BLACK       1

static BYTE tochar[] = {
    OUTPUT_PIXEL_WHITE,         /* paper */
    OUTPUT_PIXEL_BLACK,         /* 1520 colour numbers: 0 - 3 */
    OUTPUT_PIXEL_BLUE, 
    OUTPUT_PIXEL_GREEN, 
    OUTPUT_PIXEL_RED, 
};

/* ------------------------------------------------------------------------- */
/* 1520 plotter engine. */

static void write_lines(plot_t *mps, int lines)
{
    int x, y;
    int prnr = mps->prnr;

#if DEBUG1520
    log_message(drv1520_log, "write_lines: %d lines", lines);
#endif

    lines *= PIXELS_PER_STEP;

    for (y = 0; y < lines; y++) {
        for (x = 0; x < X_PIXELS; x++) {
            output_select_putc(prnr, tochar[(*mps->sheet)[y][x]]);
        }
        output_select_putc(prnr, (BYTE)OUTPUT_NEWLINE);
    }
}

/*
 * The current location becomes the new hard origin.
 * Anything that is now too far away in the "up" direction
 * can be printed for sure and erased from our buffered sheet.
 */
static void reset_hard_origin(plot_t *mps)
{
#if DEBUG1520
    log_message(drv1520_log, "reset_hard_origin: abs_origin_y=%d lowest_y=%d cur_y=%d", mps->abs_origin_y, mps->lowest_y, mps->cur_y);
#endif
    mps->abs_origin_x += mps->cur_x;
    mps->abs_origin_y += mps->cur_y;
#if DEBUG1520
    log_message(drv1520_log, "                 : abs_origin_y=%d lowest_y=%d", mps->abs_origin_y, mps->lowest_y);
#endif

    mps->cur_x = mps->cur_y = 0;

    /* If the hard origin is reset, does the relative origin too reset?
     * Assume "yes" for now.
     */
    mps->rel_origin_x = mps->rel_origin_y = 0;

    if (mps->abs_origin_y < -MAX_Y_COORD) {
        int y_segments = -mps->abs_origin_y - MAX_Y_COORD;
        int y_pixels = y_segments * PIXELS_PER_STEP;
        int remaining_y_pixels = Y_PIXELS - y_pixels;
#if DEBUG1520
        log_message(drv1520_log, "reset_hard_origin: output and shift %d pixels (%d Y-coordinates)", y_pixels, y_segments);
#endif

        write_lines(mps, y_segments);
        memmove(&(*mps->sheet)[0][0],                   /* destination */
                &(*mps->sheet)[y_pixels][0],            /* source */
                remaining_y_pixels * X_PIXELS);

        memset(&(*mps->sheet)[remaining_y_pixels][0],
                PIXEL_INDEX_WHITE,
                y_pixels * X_PIXELS);

        mps->abs_origin_y += y_segments;
        mps->lowest_y += y_segments;
#if DEBUG1520
    log_message(drv1520_log, "                 : abs_origin_y=%d lowest_y=%d", mps->abs_origin_y, mps->lowest_y);
#endif
    }
}

static void eject(plot_t *mps)
{
#if DEBUG1520
    log_message(drv1520_log, "eject");
#endif
    write_lines(mps, -mps->lowest_y + 1);

    memset(&(*mps->sheet)[0][0], PIXEL_INDEX_WHITE, Y_PIXELS * X_PIXELS);

    mps->abs_origin_x = 0;
    mps->abs_origin_y = -VERTICAL_CLEARANCE;
    mps->cur_x = 0;
    mps->cur_y = 0;
    mps->lowest_y = mps->abs_origin_y;
}

static inline int vice_min(int a, int b)
{
    return a < b ? a : b;
}

/* FIXME: dead code? */
#if 0
static inline int vice_max(int a, int b)
{
    return a > b ? a : b;
}
#endif

static void mix(BYTE *old, int new)
{
    if (*old == PIXEL_INDEX_WHITE) {
        *old = new;
    } else if (*old == new) {
        /* no action needed */
    } else {
        /* multiple colours on top of each other make "black" */
        *old = PIXEL_INDEX_BLACK;
    }
}

static void plot(plot_t *mps, int x, int y)
{
#if DEBUG1520_A
    log_message(drv1520_log, "plot: (%4d,%4d) scribe=%d state=%d", x, y, mps->scribe, mps->scribe_state);
#endif
    if (mps->scribe) {
        if (mps->scribe_state < mps->scribe) {
            mix(&(*mps->sheet)[y][x], PIXEL_INDEX_BLACK + mps->colour);
        } 
        mps->scribe_state++;
        if (mps->scribe_state >= 2 * mps->scribe) {
            mps->scribe_state = 0;
        }
    } else {
        mix(&(*mps->sheet)[y][x], PIXEL_INDEX_BLACK + mps->colour);
    }
}



/*
 * Standard line drawing algorithm from Bresenham.
 * See http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm#Simplification
 * (retrieved Sat Nov 30 11:09:21 CET 2013).
 */
static void bresenham(plot_t *mps, int x0, int y0, int x1, int y1)
{
    int dx = abs(x0 - x1);
    int dy = abs(y0 - y1);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
#if DEBUG1520_B
    log_message(drv1520_log, "bresenham: (%4d,%4d)...(%4d,%4d)",  x0, y0, x1, y1);
#endif

    for (;;) {
        int e2;
        plot(mps, x0, y0);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0  += sx;
        }
        if (x0 == x1 && y0 == y1) {
            plot(mps, x0, y0);
            break;
        }
        if (e2 < dx) {
            err += dx;
            y0  += sy;
        }
    }
}

/*
 * Inspired by
 * http://www.zoo.co.uk/~murphy/thickline/index.html
 * in particular the parallel version, but using the structure of the
 * existing bresenham() function so that it functions in all octants.
 */
static void bresenham_par(plot_t *mps, int x0, int y0, int x1, int y1, int wd)
{
    int dy = abs(x0 - x1);      /* rotated by 90 degrees */
    int dx = abs(y0 - y1);

    int sy = (x0 <  x1) ? 1 : -1;
    int sx = (y0 >= y1) ? 1 : -1;
    int err = dx - dy;
    int xo = 0, yo = 0;
    int scribe = mps->scribe_state;
#if DEBUG1520_B
    log_message(drv1520_log, "bresenham_par: (%4d,%4d)...(%4d,%4d) dx=%d dy=%d sx=%d sy=%d",  x0, y0, x1, y1, dx, dy, sx, sy);
#endif

    /* Draw middle of the line */
    bresenham(mps, x0, y0, x1, y1);

    /* Draw parallel lines on both sides of the middle */
    for (wd--; wd > 0; wd -= 2) {
        int e2;
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            xo  += sx;
            /* mps->colour = vice_max(1, (mps->colour+1) % 4); *//* DEBUG */
            mps->scribe_state = scribe;
            bresenham(mps, x0 + xo, y0 + yo, x1 + xo, y1 + yo);
            if (wd > 1) {
                mps->scribe_state = scribe;
                bresenham(mps, x0 - xo, y0 - yo, x1 - xo, y1 - yo);
            }
        }
        if (e2 < dx) {
            err += dx;
            yo  += sy;
            /* mps->colour = vice_max(1, (mps->colour+1) % 4); *//* DEBUG */
            mps->scribe_state = scribe;
            bresenham(mps, x0 + xo, y0 + yo, x1 + xo, y1 + yo);
            if (wd > 1) {
                mps->scribe_state = scribe;
                bresenham(mps, x0 - xo, y0 - yo, x1 - xo, y1 - yo);
            }
        }
    }
}

#define Assert(val, expr) do {  \
        if (!(expr)) {          \
            log_error(drv1520_log, "%s %d: assertion %s failed: %d", __FILE__, __LINE__, #expr, val);\
            return;             \
        }                       \
    } while (0)

static void draw(plot_t *mps, int from_x, int from_y, int to_x, int to_y)
{
#if DEBUG1520_C
    log_error(drv1520_log, "draw:                                 (%4d,%4d)...(%4d,%4d)", from_x, from_y, to_x, to_y);
#endif
    /*
     * Convert to absolute plotter coordinates.
     * Add 1 here to make room for the line thickness.
     */
    from_x += mps->abs_origin_x + 1;
    from_y += mps->abs_origin_y + 1;
      to_x += mps->abs_origin_x + 1;
      to_y += mps->abs_origin_y + 1;

    mps->lowest_y = vice_min(mps->lowest_y, vice_min(from_y, to_y));

#if DEBUG1520_C
    log_message(drv1520_log, "draw: applied abs origin (%4d,%4d): (%4d,%4d)...(%4d,%4d) lowest_y=%d",  mps->abs_origin_x, mps->abs_origin_y, from_x, from_y, to_x, to_y, mps->lowest_y);
#endif
    /* Flip y-axis */
    from_y = -from_y;
      to_y = -  to_y;

#if DEBUG1520_C
    log_message(drv1520_log, "draw: flip y:                         (%4d,%4d)...(%4d,%4d)",  from_x, from_y, to_x, to_y);
#endif

    Assert(from_x, from_x > 0);
    Assert(  to_x,   to_x > 0);
    Assert(from_x, from_x <= MAX_COL);
    Assert(  to_x,   to_x <= MAX_COL);

    Assert(from_y, from_y > 0);
    Assert(  to_y,   to_y > 0);
    Assert(from_y, from_y <= MAX_ROW);
    Assert(  to_y,   to_y <= MAX_ROW);

    /* Convert to bitmap resolution and coordinates */

    from_x *= PIXELS_PER_STEP;
    from_y *= PIXELS_PER_STEP;
      to_x *= PIXELS_PER_STEP;
      to_y *= PIXELS_PER_STEP;

    /* Every line re-starts the dashed line state */
    mps->scribe_state = 0;

    /*bresenham(mps, from_x, from_y, to_x, to_y);*/
    bresenham_par(mps, from_x, from_y, to_x, to_y, PIXELS_PER_STEP);
}

/*
 * Reverse-engineered drawing commands for the characters.
 */
char *punct[32] = {
    /*   */ NULL,
    /* ! */ "66 d 8624 u 88 d 888886222224 u",
    /* " */ "988888 d 2 u 96 d 2",
    /* # */ "9 d 8888 u 1 d 6666 u 7 d 2222 u 9 d 4444",
    /* $ */ "96 d 6697447966 u 7 d 222222",
    /* % */ "8 d 9999 u 444 d 4268 u 332 d 6248",
    /* & */ "6666 d 777789321123699",               /* long line first */
    /* ' */ "888899 d 22",
    /* ( */ "6666 d 478888896",
    /* ) */ "66   d 698888874",
    /* * */ "8 d 9999 u 4444 d 3333 u 7788 d 2222", /* vertical down last */
    /* + */ "888 d 6666 u 11 d 8888",               /* right, then down */
    /* , */ "66 d 98426",
    /* - */ "888 d 66666",
    /* . */ "666 d 8624",
    /* / */ "d 9999",
    /* 0 */ "8 d 8888966322221447 9999",
    /* 1 */ "88888 d 9222222 u 4 d 66",             /* down */
    /* 2 */ "88888 d 9663211116666",
    /* 3 */ "88888 d 9663214 u 6 d321447",
    /* 4 */ "9966 d 44448999222222",
    /* 5 */ "8 d 3669887444886666",
    /* 6 */ "888 d 966322144788889663",
    /* 7 */ "d 9999884444",
    /* 8 */ "999 d 98744123 u 66 d 3214478966",
    /* 9 */ "8 d 36698888744123669",
    /* : */ "99 d 8624 u 88 d 8624",
    /* ; */ "96 d 98426 u 88 d 8426",
    /* < */ "999988 d 111333",
    /* = */ "88 d 6666 u 7744 d 6666",
    /* > */ "d 999777",
    /* ? */ "88888 d 96632142 u 2 d 2"
};
/* typically 4 units wide, and 6 units high. */
char *uppercase[32] = {
    /* @ */ "669 d 8884422266 98874412223666", /* anti-clockwise from inside */
    /* A */ "d 888899332222 u 888 d 4444",
    /* B */ "d 888888666321444 u 666 d 321444",
    /* C */ "99998 d 744122223669",
    /* D */ "d 888888666322221444",
    /* E */ "6666 d 4444 888888 6666 u 1114 d 666 u 1114",
    /* F */ "d 8888886666 u 1114 d 666",
    /* G */ "99998 d 74412222366688844",
    /* H */ "d 888888 u 222 d 6666 u 888 d 222222",
    /* I */ "6 d 66 u 4 d 888888 u 4 d 66",
    /* J */ "8 d 36988888",
    /* K */ "d 888888 u 6666 d 1111 u 9 d 333",
    /* L */ "888888 d 222222 6666",
    /* M */ "d 888888332 u 8 d 99222222",
    /* N */ "d 888888 u 2 d 3333 u 2 d 888888",
    /* O */ "8 d 8888966322221447",
    /* P */ "d 888888666321444",
    /* Q */ "8 d 8888966322221447 u 96 d 33",
    /* R */ "d 888888666321444 u 6 d 333",
    /* S */ "8 d 36698744789663",
    /* T */ "66 d 888888 u 44 d 6666",
    /* U */ "888888 d 22222366988888",
    /* V */ "888888 d 222233998888",
    /* W */ "888888 d 222222998 u 2 d 33888888",
    /* X */ "d 899998 u 4444 d 233332 ",
    /* Y */ "66 d 888778 u 6666 d 211", /* ?? */
    /* Z */ "888888 d 66662111126666",
    /* [ */ "66 d 44 888888 66",
/* pound */ "99999 d 41222266 u 887 d 44 u 1 d 1369 u 4 d 7",   /* ?? */
    /* ] */ "d 666 888888 44 ",
    /* ^ */ "66 d 88888 11 u 99 d 33",
    /* <-*/ "66999 d 44444 333 u 777 d 999",
};
/* Typically 3 units wide and 4 high (plus ascenders / descenders). */
char *lowercase[32] = {
    /* --*/ "888 d 666666",
    /* a */ "8888 d 66322147896323",
    /* b */ "888888 d 22222266988744",
    /* c */ "999 d 74122369",
    /* d */ "999888 d 22222244788966",
    /* e */ "88 d 666874122366",
    /* f */ "6 d 888886 u 112 d 66",/* ?? */
    /* g */ "996 d 1478963222147",
    /* h */ "d 888888 u 22 d 663222",
    /* i */ "66 d 888 u 8 d 8",
    /* j */ "d 369888 u 8 d 8",
    /* k */ "d 888888 u 336 d 111 u 9 d 33",
    /* l */ "888889 d 222222 6",
    /* m */ "d 8888 u 2 d 93222 u 888 d 93222",
    /* n */ "d 8888 u 2 d 963222",
    /* o */ "8 d 8896322147",           /* clockwise */
    /* p */ "8 d 66987442222",          /* anti-clockwise */
    /* q */ "6663 d 4888884412366",     /* anti-clockwise */
    /* r */ "8888 d 3222 u 888 d 963",  /* from serif down, then top bow */
    /* s */ "8 d 369747963",
    /* t */ "8888 d 66 u 78 d 2222226", /* down */
    /* u */ "8888 d 22226668888",       /* down right up */
    /* v */ "8888 d 22339988",
    /* w */ "8888 d 222398 u 2 d 39888",
    /* x */ "d 9999 u 4444 d 3333",
    /* y */ "8888 d 233 u 998 d 21111", /* top left to middle, top right to left bottom */
    /* z */ "8888 d 666611116666",      /* top to bottom */
    /* | */ "9998888 d 22222222",
    /* __*/ "2 d 66666666",
    /* ^_*/ "d 666666777111",/* too wide? */
    /* pi*/ "6 d 88866222 u 44488 d 9669 ",/* clockwise square, then hat */
    /* []*/ "d 8888886666622222244444",
};

int dir[20] = { /* delta-x, delta-y */
     0,  0,     /* 0 */
    -1, -1,     /* 1   7 8 9 */
     0, -1,     /* 2   4 5 6 */
     1, -1,     /* 3   1 2 3 */
    -1,  0,     /* 4 */
     0,  0,     /* 5 */
     1,  0,     /* 6 */
    -1,  1,     /* 7 */
     0,  1,     /* 8 */
     1,  1,     /* 9 */
};

#define LINEFEED       10
#define LW              4       /* letter width */
#define SW              6       /* letter pitch */
#define LH              6       /* letter height */

void draw_char(plot_t *mps, char *commands)
{
    int x = mps->cur_x;
    int y = mps->cur_y;
    int s = mps->charsize;
    int pen_down = 0;
    char command;

    /*
     * For normal and rotated letters, the tops of the
     * (upper case) letters line up. Since we draw down from the
     * starting point (when rotated), move "letter height" units up.
     */
    if (mps->rotation) {
        y += LH * s;
        x += s;
    }

    while ((command = *commands++) != '\0') {
        if (command == 'u') {
            pen_down = 0;
        } else if (command == 'd') {
            pen_down = 1;
        } else if (command >= '0' && command <= '9') {
            int num = 2 * (command - '0');
            int newx, newy;

            if (mps->rotation) {
                newx = x + s * dir[num + 1];
                newy = y - s * dir[num];
            } else {
                newx = x + s * dir[num];
                newy = y + s * dir[num + 1];
            }

            /* TODO: this is probably not how it was really handled: */
            newx = vice_min(MAX_COL, newx);
            if (pen_down) {
                draw(mps, x, y, newx, newy);
            }

            x = newx;
            y = newy;
        }
    }
}

static void print_char_text(plot_t *mps, BYTE c)
{
    int underline = 0;
    char **tab;

    if (c == CR) {
        mps->cur_x = 0;
        mps->cur_y -= mps->charsize * LINEFEED;
        reset_hard_origin(mps);
        mps->quote_mode = 0;
        return;
    }

    if (c == 0xFF) {    /* Handle pi */
        c = 0xDE;
    }

    switch (c & 0x60) {
        case 0x00:
            if (mps->quote_mode) {
                c += 0x40;
                underline++;
                tab = uppercase;
            } else {
                tab = NULL;
            }
            break;
        case 0x20:
            tab = punct;
            break;
        case 0x40:
            tab = uppercase;
            break;
        case 0x60:
        default:        /* pacify gcc */
            tab = NULL;
            break;
    }

    if (c == 0x22) {
        mps->quote_mode = !mps->quote_mode;
    }

    if (tab == uppercase) {
        /* shifted char XOR lowercase mode */
        if (!(c & 0x80) != !mps->lowercase) {
            tab = lowercase;
        }
    }

    if (tab && tab[c & 0x1F]) {
        draw_char(mps, tab[c & 0x1F]);
    }

    mps->cur_x += mps->charsize * SW;

    if (underline) {
        draw(mps, mps->cur_x - mps->charsize * SW, mps->cur_y - 1,
                  mps->cur_x                     , mps->cur_y - 1);
    }
}

#define INCOMPLETE      99999

/*
 * Parse a number until a CR is seen.
 * Expects *accu to start out at 0, and resets it to 0 when it returns
 * the final result.
 * Calls before that return INCOMPLETE.
 * White space is ignored.
 * Other characters reset the accumulator to 0.
 */
static int numparser(int *accu, BYTE c)
{
    /*
     * TODO: what does it do with other characters?
     * TODO: what happens if a command is not finished with a CR
     *       before the unlisten is sent? Does the unlisten cause
     *       the command to be executed, or is it the CR?
     *       And if it is the CR, is the state remembered when
     *       the printing to the secondary address is continued?
     */

    if (c >= '0' && c <= '9') {
        *accu *= 10;
        *accu += c - '0';
    } else if (c == CR) {
        int result = *accu;
        *accu = 0;
        return result;
    } else if (c == ' ' || c == 29 /* crsr right */) {
        /* Ignore */
    } else {
        *accu = 0;
    }

    return INCOMPLETE;
}

#define FNUM_START      0x01
#define FNUM_MINUS      0x02
#define FNUM_DOT        0x04
#define FNUM_E          0x08

/*
 * This parser considers a number finished when a space or other
 * non-numeric character is seen but ignores leading spaces.
 * It expects *accu to start out as 0 and leaves the result in there.
 *
 * It does a very approximate parsing of floating point numbers:
 * if it sees an E indicating an exponent, the result is 0.
 * The rationale is that if the exponent is negative, the number is less
 * than 1 and thus truncates to 0.
 * If the exponent is positive, it is likely to be at least 7, and
 * therefore the number overflows rather a lot.
 * (the 1520 indeed interprets 240E0 and 24E1 etc as 0!)
 */
static int fnumsparser(int *accu, int *state, BYTE c)
{
    /*
     * TODO: what does it do with other characters?
     * TODO: what if 2 numbers run together because the second one
     *       is negative: e.g.  10-10
     */

    if (*state & FNUM_START) {
        if (c == '-') {
            *accu = 0;
            *state |=  FNUM_MINUS;
            *state &= ~FNUM_START;
        } else if (c >= '0' && c <= '9') {
            *accu = (c - '0');
            *state &= ~FNUM_START;
        } else if (c == '.') {
            *accu = 0;
            *state |=  FNUM_DOT;        /* ignore further digits */
            *state &= ~FNUM_START;
        } else  if (c == ' ' || c == 29 /* crsr right */) {
            /* Ignore */
        } else {
            /* TODO: what to do here? */
        }
    } else {
        if (c >= '0' && c <= '9') {
            if (!(*state & (FNUM_DOT|FNUM_E))) {
                *accu *= 10;
                *accu += (c - '0');
                if (*accu > 998) {
                    *accu = 998;
                }
            }
        } else if (c == '.') {
            *state |= FNUM_DOT; /* ignore further digits */
        } else if (c == 'E') {
            *state |= FNUM_E;   /* ignore further digits */
            *accu = 0;          /* number too small or too large anyway */
        } else if (c == '-') {
            if (*state & FNUM_E) {
                /* ignore it */
            } else {
                goto done;
            }
        } else {
done:
            /* SPACE, CR, other junk -> Finished */
            if (*state & FNUM_MINUS) {
                *accu = - *accu;
                *state &= ~FNUM_MINUS;
            }
            *state = FNUM_START;
            return *accu;
        }
    }

    return INCOMPLETE;
}

static void print_char_plot(plot_t *mps, const BYTE c)
{
    int value;

    /*
     * TODO: what does it do with incorrect command letters?
     */
    switch (mps->command_stage) {
    case start:
        if (strchr("HIMDRJ", c)) {
            mps->command = c;
            mps->command_x = 0;
            mps->command_y = 0;
            mps->command_flags = FNUM_START;
            mps->command_stage = letter_seen;
#if DEBUG1520
            log_message(drv1520_log, "print_char_plot: command_stage := letter_seen");
#endif
        }
        break;
    case letter_seen:
        value = fnumsparser(&mps->command_x, &mps->command_flags, c);
        if (value != INCOMPLETE) {
            mps->command_stage = x_seen;
#if DEBUG1520
            log_message(drv1520_log, "print_char_plot: command_stage := x_seen");
#endif
        }
        break;
    case x_seen:
        value = fnumsparser(&mps->command_y, &mps->command_flags, c);
        if (value != INCOMPLETE) {
            mps->command_stage = y_seen;
#if DEBUG1520
            log_message(drv1520_log, "print_char_plot: command_stage := y_seen");
#endif
        }
        break;
    case y_seen:
        /* By now we really hope to see a CR - ignore anything else. */
        break;
    }

    if (c == CR) {
        int new_x, new_y;
#if DEBUG1520
            log_message(drv1520_log, "print_char_plot: executing command %c %d %d", mps->command, mps->command_x, mps->command_y);
#endif

        /* TODO: Q: what if not enough numbers are given?
         * A: It seems that 0 is used.
         * 
         * TODO: Q: what if the numbers are too large? >= 999
         * A1: x > 480: head tries to move there anyway; motor buzzes.
         */


        switch (mps->command) {
        case 'H':       /* move to absolute origin */
            mps->cur_x = 0;
            mps->cur_y = 0;
            break;
        case 'I':       /* set relative origin here */
            mps->rel_origin_x = mps->cur_x;
            mps->rel_origin_y = mps->cur_y;
            break;
        case 'M':       /* move to (x,y) relative to absolute origin */
            mps->cur_x = mps->command_x;
            mps->cur_y = mps->command_y;
            break;
        case 'R':       /* move to (x,y) relative to relative origin */
            mps->cur_x = mps->rel_origin_x + mps->command_x;
            mps->cur_y = mps->rel_origin_y + mps->command_y;
            break;
        case 'D':       /* draw to (x,y) relative to absolute origin */
            new_x = mps->command_x;
            new_y = mps->command_y;
            draw(mps, mps->cur_x, mps->cur_y, new_x, new_y);
            mps->cur_x = new_x;
            mps->cur_y = new_y;
            break;
        case 'J':       /* draw to (x,y) relative to relative origin */
            new_x = mps->rel_origin_x + mps->command_x;
            new_y = mps->rel_origin_y + mps->command_y;
            draw(mps, mps->cur_x, mps->cur_y, new_x, new_y);
            mps->cur_x = new_x;
            mps->cur_y = new_y;
            break;
        default:        /* some error - ignore */
            break;
        }
#if DEBUG1520
            log_message(drv1520_log, "print_char_plot: cur = (%4d, %4d) rel origin = (%4d, %4d)", mps->cur_x, mps->cur_y, mps->rel_origin_x, mps->rel_origin_y);
#endif

        mps->command_stage = start;
        mps->command = '?';
    }
}

static void print_char_select_colour(plot_t *mps, const BYTE c)
{
    int value;

    value = numparser(&mps->colour_accu, c);

    if (value != INCOMPLETE) {
        mps->colour = value % 4;
    }
}

static void print_char_select_character_size(plot_t *mps, const BYTE c)
{
    int value;

    value = numparser(&mps->charsize_accu, c);

    if (value != INCOMPLETE) {
        mps->charsize = 1 << (value % 4);
    }
}

static void print_char_select_character_rotation(plot_t *mps, const BYTE c)
{
    int value;

    value = numparser(&mps->rotation_accu, c);

    if (value != INCOMPLETE) {
        mps->rotation = value % 2;
    }
}

static void print_char_select_scribe(plot_t *mps, const BYTE c)
{
    int value;

    value = numparser(&mps->scribe_accu, c);

    if (value != INCOMPLETE) {
        mps->scribe = (value % 16) * PIXELS_PER_STEP;
#if DEBUG1520
        log_message(drv1520_log, "print_char_select_scribe: scribe = %d (%d)", mps->scribe, value);
#endif
    } else {
#if DEBUG1520
        log_message(drv1520_log, "print_char_select_scribe: scribe_accu = %d", mps->scribe_accu);
#endif
    }
}

static void print_char_select_lowercase(plot_t *mps, const BYTE c)
{
    int value;

    value = numparser(&mps->lowercase_accu, c);

    if (value != INCOMPLETE) {
        mps->lowercase = value % 2;
    }
}

static void draw_one_square(plot_t *mps, int colour)
{
    mps->colour = colour;

#define SZ (4 * STEPS_PER_MM)   /* 4 mm */

    draw(mps, mps->cur_x   , mps->cur_y   , mps->cur_x   , mps->cur_y+SZ);
    draw(mps, mps->cur_x   , mps->cur_y+SZ, mps->cur_x+SZ, mps->cur_y+SZ);
    draw(mps, mps->cur_x+SZ, mps->cur_y+SZ, mps->cur_x+SZ, mps->cur_y   );
    draw(mps, mps->cur_x+SZ, mps->cur_y   , mps->cur_x   , mps->cur_y   );

    mps->cur_x += SZ + STEPS_PER_MM;    /* 1mm in between squares */
}

static void draw_four_squares(plot_t *mps)
{
    int c;

    for (c = 1; c < 5; c++) {   /* blue, green, red, black */
        draw_one_square(mps, c % 4);
    }
    mps->cur_x = 0;
    mps->cur_y -= 4 * STEPS_PER_MM;
    reset_hard_origin(mps);
    mps->colour = 0;
}


static void power_on_reset(plot_t *mps)
{
    int prnr = mps->prnr;

    if (mps->sheet) {
        lib_free(mps->sheet);
    }

    memset(mps, 0, sizeof(*mps));

    mps->prnr = prnr;
    mps->charsize = 1 << 1;
    mps->sheet = lib_calloc(Y_PIXELS, X_PIXELS * sizeof(BYTE));
    mps->abs_origin_x = 0;
    mps->abs_origin_y = -VERTICAL_CLEARANCE;

    draw_four_squares(mps);
}

static void print_char_reset(plot_t *mps, const BYTE c)
{
    if (c == CR) {
        power_on_reset(mps);
    }
}

/* ------------------------------------------------------------------------- */
/* Interface to the upper layer.  */

static int drv_1520_open(unsigned int prnr, unsigned int secondary)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_open: sa=%d prnr=%d", secondary, prnr);
#endif

    /* Is this the first open? */
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        plot_t *mps = &drv_1520[prnr];

        output_parameter.maxcol = X_PIXELS;
        output_parameter.maxrow = Y_PIXELS;
        output_parameter.dpi_x = (PIXELS_PER_STEP * STEPS_PER_MM * 254) / 10;
        output_parameter.dpi_y = (PIXELS_PER_STEP * STEPS_PER_MM * 254) / 10;
        output_parameter.palette = palette;

        drv_1520[prnr].prnr = prnr;
        power_on_reset(mps);

        return output_select_open(prnr, &output_parameter);
    } else if (secondary > 7) {
        return -1;
    }

    return 0;
}

static void drv_1520_close(unsigned int prnr, unsigned int secondary)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_close: sa=%d prnr=%d", secondary, prnr);
#endif

    /* Is this the last close? */
    if (secondary == DRIVER_LAST_CLOSE) {
        plot_t *mps = &drv_1520[prnr];

#if DEBUG1520
        log_message(drv1520_log, "drv_1520_close: last close");
#endif
        eject(mps);

        if (mps->sheet) {
            lib_free(mps->sheet);
            mps->sheet = NULL;
            output_select_close(prnr);
        }
    }
}

static int drv_1520_putc(unsigned int prnr, unsigned int secondary, BYTE b)
{
    plot_t *mps = &drv_1520[prnr];

#if DEBUG1520
    log_message(drv1520_log, "drv_1520_putc: sa=%d b='%c' (%d) prnr=%d", secondary, b, b, prnr);
#endif

    switch (secondary) {
    case 0:
        print_char_text(mps, b);
        break;
    case 1:
        print_char_plot(mps, b);
        break;
    case 2:
        print_char_select_colour(mps, b);
        break;
    case 3:
        print_char_select_character_size(mps, b);
        break;
    case 4:
        print_char_select_character_rotation(mps, b);
        break;
    case 5:
        print_char_select_scribe(mps, b);
        break;
    case 6:
        print_char_select_lowercase(mps, b);
        break;
    case 7:
        print_char_reset(mps, b);
        break;
    default:
        return -1;
    }

    return 0;
}

static int drv_1520_getc(unsigned int prnr, unsigned int secondary, BYTE *b)
{
    return output_select_getc(prnr, b);
}

static int drv_1520_flush(unsigned int prnr, unsigned int secondary)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_flush");
#endif
    return output_select_flush(prnr);
}

static int drv_1520_formfeed(unsigned int prnr)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_formfeed");
#endif
    plot_t *mps = &drv_1520[prnr];

    if (mps->prnr == (int)prnr && mps->sheet != NULL) {
        eject(mps);
    }
    return 0;
}

int drv_1520_init_resources(void)
{
    driver_select_t driver_select;
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_init_resources");
#endif

    driver_select.drv_name = "1520";
    driver_select.drv_open = drv_1520_open;
    driver_select.drv_close = drv_1520_close;
    driver_select.drv_putc = drv_1520_putc;
    driver_select.drv_getc = drv_1520_getc;
    driver_select.drv_flush = drv_1520_flush;
    driver_select.drv_formfeed = drv_1520_formfeed;

    driver_select_register(&driver_select);

    return 0;
}

int drv_1520_init(void)
{
    static const char *color_names[5] =
    {
        "Black", "White", "Blue", "Green", "Red"
    };

    drv1520_log = log_open("plot1520");

#if DEBUG1520
    log_message(drv1520_log, "drv_1520_init");
#endif

    palette = palette_create(5, color_names);

    if (palette == NULL) {
        return -1;
    }

    if (palette_load("1520" FSDEV_EXT_SEP_STR "vpl", palette) < 0) {
        log_error(drv1520_log, "Cannot load palette file `%s'.",
                  "1520" FSDEV_EXT_SEP_STR "vpl");
        return -1;
    }

    return 0;
}

void drv_1520_shutdown(void)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_shutdown");
#endif
    palette_free(palette);
}
