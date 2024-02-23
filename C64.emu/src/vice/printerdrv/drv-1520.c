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

/*#define DEBUG1520       1 */
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
#define SHIFT_CR                (CR + 128)
#define LF                      10

/*
 * Since the plotter's y coordinates follow the usual mathematic
 * convention (positive y axis is "up"), but programs like to number
 * rows from the top down, we'll invert the logical plotter coordinates
 * to obtain array indices (which start at 0 and increase as you go down
 * the page).
 *
 * In places, references are made to a ROM disassembly. It can be found at
 * http://www.zimmers.net/anonftp/pub/cbm/firmware/printers/1520/1520-03.asm.html
 */

enum command_stage {
    start,
    letter_seen,
    x_seen,
    y_seen,
    skip_rest
};

struct plot_s {
    int prnr;
    uint8_t (*sheet)[Y_PIXELS][X_PIXELS];
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
    int abs_origin_x, abs_origin_y;     /* steps relative to start of page */
    int rel_origin_x, rel_origin_y;     /* steps relative to abs_origin */
    int cur_x, cur_y;                   /* steps relative to abs_origin */
    int lowest_y;                       /* steps relative to start of page */
};
typedef struct plot_s plot_t;

static plot_t drv_1520[NUM_OUTPUT_SELECT];
static palette_t *palette = NULL;

/* Logging goes here.  */
static log_t drv1520_log = LOG_ERR;

#define PIXEL_INDEX_WHITE       0
#define PIXEL_INDEX_BLACK       1

static const uint8_t tochar[] = {
    OUTPUT_PIXEL_WHITE,         /* paper */
    OUTPUT_PIXEL_BLACK,         /* 1520 colour numbers: 0 - 3 */
    OUTPUT_PIXEL_BLUE,
    OUTPUT_PIXEL_GREEN,
    OUTPUT_PIXEL_RED,
};

static void reset_scribe_state(plot_t *mps);

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
        output_select_putc(prnr, (uint8_t)OUTPUT_NEWLINE);
    }
}

/*
 * The current location becomes the new absolute origin.
 * Anything that is now too far away in the "up" direction
 * can be printed for sure and erased from our buffered sheet.
 */
static void reset_abs_origin(plot_t *mps)
{
#if DEBUG1520
    log_message(drv1520_log, "reset_abs_origin: abs_origin_y=%d lowest_y=%d cur_y=%d", mps->abs_origin_y, mps->lowest_y, mps->cur_y);
#endif
    mps->abs_origin_x += mps->cur_x;
    mps->abs_origin_y += mps->cur_y;
#if DEBUG1520
    log_message(drv1520_log, "                 : abs_origin_y=%d lowest_y=%d", mps->abs_origin_y, mps->lowest_y);
#endif

    mps->cur_x = mps->cur_y = 0;
    reset_scribe_state(mps);

    /* If the absolute origin is reset, does the relative origin too reset?
     * Assume "yes" for now.
     */
    mps->rel_origin_x = mps->rel_origin_y = 0;

    if (mps->abs_origin_y < -MAX_Y_COORD) {
        int y_steps = -mps->abs_origin_y - MAX_Y_COORD;
        int y_pixels = y_steps * PIXELS_PER_STEP;
        int remaining_y_pixels = Y_PIXELS - y_pixels;
#if DEBUG1520
        log_message(drv1520_log, "reset_abs_origin: output and shift %d pixels (%d Y-coordinates)", y_pixels, y_steps);
#endif

        write_lines(mps, y_steps);
        memmove(&(*mps->sheet)[0][0],                   /* destination */
                &(*mps->sheet)[y_pixels][0],            /* source */
                remaining_y_pixels * X_PIXELS);

        memset(&(*mps->sheet)[remaining_y_pixels][0],
                PIXEL_INDEX_WHITE,
                y_pixels * X_PIXELS);

        mps->abs_origin_y += y_steps;
        mps->lowest_y += y_steps;
#if DEBUG1520
        log_message(drv1520_log, "                 : abs_origin_y=%d lowest_y=%d", mps->abs_origin_y, mps->lowest_y);
#endif
    }
}

/*
 * Set the relative origin to the place where we are now.
 */
static void set_rel_origin(plot_t *mps)
{
    mps->rel_origin_x = mps->cur_x;
    mps->rel_origin_y = mps->cur_y;
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

/*
 * Originally: every line draw re-starts the dashed line state.
 *
 * Now: the rom disassembly suggests that the scribe state gets reset on
 * every move or draw (pen up or down) or when setting the absolute
 * origin. Of course you see the effect only when the pen is down.
 * Therefore we can leave out the calls where there is only movement.
 * See 0e13  sta dash_counter
 *
 * The difference is what happens with lines drawn for letters.
 */
static void reset_scribe_state(plot_t *mps)
{
    mps->scribe_state = 0;
}

static inline int vice_min(int a, int b)
{
    return a < b ? a : b;
}

/* Used in DEBUG code only */
#if 0
static inline int vice_max(int a, int b)
{
    return a > b ? a : b;
}
#endif

static void mix(uint8_t *old, int new)
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
    if (x >= X_PIXELS || x < 0 || y > Y_PIXELS || y < 0) {
#if DEBUG1520
        log_message(drv1520_log, "plot: (%4d,%4d) out of bounds", x, y);
#endif
        return;
    }

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
            /* mps->colour = vice_max(1, (mps->colour+1) % 4); */ /* DEBUG */
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
            /* mps->colour = vice_max(1, (mps->colour+1) % 4); */ /* DEBUG */
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

    /*bresenham(mps, from_x, from_y, to_x, to_y);*/
    bresenham_par(mps, from_x, from_y, to_x, to_y, PIXELS_PER_STEP);
}

/*
 * Reverse-engineered drawing commands for the characters.
 */
static const char * const punct[32] = {
    /*   */ NULL,
    /* ! */ "96 d 8624 u 88 d 8888622224",
    /* " */ "9888888 d 2 u 96 d 2",
    /* # */ "98 d 8888 u 66 d 2222 u 9 d 4444 u 88 d 6666",
    /* $ */ "98 d 6697447966 u 7 d 222222",
    /* % */ "88 d 9999 u 444 d 4268 u 332 d 6248",
    /* & */ "9666 d 777789321123699",                /* long line first */
    /* ' */ "88899 d 88",
    /* ( */ "966 d 47888896",
    /* ) */ "96  d 69888874",
    /* * */ "88 d 9999 u 4444 d 3333 u 7788 d 2222", /* vertical down last */
    /* + */ "99 d 8888 u 11 d 6666",
    /* , */ "66 d 98426",
    /* - */ "8888 d 6666",
    /* . */ "96 d 8624",
    /* / */ "8 d 99999",
    /* 0 */ "88 d 9999 74412222 36698888",
    /* 1 */ "988888 d 9222222 u 4 d 66",             /* down */
    /* 2 */ "888888 d 9663211116666",
    /* 3 */ "888888 d 9663214 u 6 d 321447",
    /* 4 */ "9996 d 44448999222222",
    /* 5 */ "88 d 3669887444886666",
    /* 6 */ "8888 d 966322144788889663",
    /* 7 */ "8 d 9999884444",
    /* 8 */ "8889 d 789663213214478966",
    /* 9 */ "88 d 36698888744123669",
    /* : */ "89 d 8624 u 888 d 8624",
    /* ; */ "9 d 98426 u 888 d 8426",
    /* < */ "9666 d 777999",
    /* = */ "988 d 666 u 774 d 666",
    /* > */ "8 d 999777",
    /* ? */ "888888 d 96632142 u 2 d 2"
};
/* typically 4 units wide, and 7 units high (baseline 1). */
static const char * const uppercase[32] = {
    /* @ */ "999 d 884422266 98874412223666", /* anti-clockwise from inside */
    /* A */ "8 d 888899332222 u 888 d 4444",
    /* B */ "8 d 888888666321444 u 666 d 321444",
    /* C */ "9966 d 144788889663",
    /* D */ "8 d 888888666322221444",
    /* E */ "9666 d 4444 888888 6666 u 122 d 444",
    /* F */ "8 d 8888886666 u 1114 d 666",
    /* G */ "999988 d 74412222366688844",
    /* H */ "8 d 888888 u 6666 d 222222 u 7774 d 6666",
    /* I */ "9 d 66 u 4 d 888888 u 4 d 66",
    /* J */ "88 d 36988888",
    /* K */ "8 d 888888 u 6666 d 1111 u 9 d 333",
    /* L */ "8888888 d 222222 6666",
    /* M */ "8 d 88888833 28 99222222",
    /* N */ "8 d 888888 u 2 d 3333 u 88888 d 222222",
    /* O */ "88 d 8888966322221447",
    /* P */ "8 d 888888666321444",
    /* Q */ "998 d 33 u 4 d 4478888966322221",
    /* R */ "8 d 888888666321444 u 6 d 333",
    /* S */ "88 d 36698744789663",
    /* T */ "96 d 888888 u 44 d 6666",
    /* U */ "8888888 d 22222366988888",
    /* V */ "8888888 d 222233998888",
    /* W */ "8888888 d 22222299 u 8 d 233888888",
    /* X */ "8 d 899998 u 4444 d 233332 ",
    /* Y */ "69 d 888998 u 4444 d 233",
    /* Z */ "8888888 d 66662111126666",
    /* [ */ "69 d 44 888888 66",
/* pound */ "999998 d 4122214793366 u 777 d 66",
    /* ] */ "9 d 66 888888 44 ",
    /* ^ */ "69 d 88888 11 u 99 d 33",
    /* <-*/ "99 d 7799 u 11 d 66666",
};
/* Typically 3 units wide and 5 high (plus ascenders / descenders). */
static const char * const lowercase[32] = {
    /* --*/ "8888 d 6666666",
    /* a */ "996 d 1478963 u 774 d 663223",
    /* b */ "8888888 d 22222266988744",
    /* c */ "996 d 14788963",
    /* d */ "99988 d 44122366888888",
    /* e */ "888 d 666874122366",
    /* f */ "96 d 8888886 u 112 d 66",
    /* g */ "8 d 3698887412369",
    /* h */ "8 d 888888 u 22 d 663222",
    /* i */ "69 d 888 u 8 d 8",
    /* j */ "8 d 369888 u 8 d 8",
    /* k */ "669 d 77 u 7888 d 222222 u 8 d 999",
    /* l */ "8888889 d 222222 6",
    /* m */ "8 d 8888 u 2 d 93222 u 888 d 93222",
    /* n */ "8 d 8888 2 963222",
    /* o */ "88 d 8896322147",           /* clockwise */
    /* p */ "88 d 669874422222",         /* anti-clockwise */
    /* q */ "6666 d 4888884412366",      /* anti-clockwise */
    /* r */ "88888 d 3222 u 888 d 96",   /* from serif down, then top bow */
    /* s */ "88 d 369747963",
    /* t */ "88888 d 66 u 78 d 2222226", /* down */
    /* u */ "88888 d 22226668888",       /* down right up */
    /* v */ "88888 d 22339988",
    /* w */ "88888 d 22239 82 39888",
    /* x */ "8 d 9999 u 4444 d 3333",
    /* y */ "88888 d 233 u 998 d 21111", /* top left to middle, top right to left bottom */
    /* z */ "88888 d 666611116666",      /* top to bottom */
    /* | */ "666 d 8888888",
    /* __*/ "d 6666666",
    /* ^_*/ "8 d 999333 444444",
    /* pi*/ "888 d 966222 u 44 d 888669",
    /* []*/ "8 d 8888886666622222244444",
};

static const int dir[20] = { /* delta-x, delta-y */
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

#define LINE_HEIGHT    10       /* line height */
#define LW              4       /* letter width */
#define SW              6       /* letter pitch */
#define LH              7       /* letter height */

static void draw_char(plot_t *mps, const char *commands)
{
    int x = mps->cur_x;
    int y = mps->cur_y;
    int s = mps->charsize;
    int pen_down = 0;
    char command;
    char prev_command = '?';

    /* Set relative origin; see disasm 0d71: char_found  jsr set_relorigin */
    set_rel_origin(mps);

    /*
     * For normal and rotated letters, the tops of the
     * (upper case) letters line up. Since we draw down from the
     * starting point (when rotated), move "letter height" units up.
     */
    if (mps->rotation) {
        y += LH * s;
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
                /*
                 * The ROM code draws longer lines in one go, but we do it in
                 * unit square segments. Reset the scribe state only on a
                 * really new line.
                 */
                if (prev_command != command) {
                    reset_scribe_state(mps);
                }

                draw(mps, x, y, newx, newy);
            }

            x = newx;
            y = newy;
            prev_command = command;
        }
    }
}

/*
 * Move the pen all the way back to the left.
 */
static void move_left_side(plot_t *mps)
{
    mps->cur_x = 0;
}

/*
 * Move the paper upward by one line.
 * This sets the absolute origin to the new position.
 */
static void line_feed(plot_t *mps)
{
    mps->cur_y -= mps->charsize * LINE_HEIGHT;
    reset_abs_origin(mps);
}

/*
 * Move to the next line without resetting quote mode.
 */
static void carriage_return(plot_t *mps)
{
    move_left_side(mps);
    line_feed(mps);
}

/*
 * Move to the next line and reset quote mode.
 */
static void print_char_cr(plot_t *mps)
{
    carriage_return(mps);
    mps->quote_mode = 0;
}

/*
 * On the computer shift-CR would reset quote mode, like CR, but
 * the 1520 apparently doesn't do it like that.
 * See disassembly 0c7e do_shcr: jsr move_left_side; beq nextchar
 */
static void print_char_shcr(plot_t *mps)
{
    move_left_side(mps);
}

static void print_char_text(plot_t *mps, uint8_t c)
{
    int underline = 0;
    const char * const *tab;

    if (c == CR || c == LF) {
        print_char_cr(mps);
        return;
    } else if (c == SHIFT_CR) {
        print_char_shcr(mps);
        return;
    }

    /* Wrap long lines, without resetting quote mode */
    if (mps->cur_x >= MAX_COL) {
        carriage_return(mps);
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

    if (underline) {
        /*
         * TODO: may also set absolute origin;
         * see disassembly 0c67: jsr set_absorigin; ldx #font.LOW_LINE
         */
        draw_char(mps, "d 6666666");
    }

    mps->cur_x += mps->charsize * SW;

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
static int numparser(int *accu, uint8_t c)
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
static int fnumsparser(int *accu, int *state, uint8_t c)
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
            /*
             * TODO: if too many digits, ignore the remainder.
             * See disassembly 0cc9 sta count, 0d0a beq digits_finished,
             * digits_finished 0d25
             */
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

static void print_char_plot(plot_t *mps, const uint8_t c)
{
    int value;

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
        } else {
            mps->command = '?';
            mps->command_stage = skip_rest;
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
    case skip_rest:
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
            set_rel_origin(mps);
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
            reset_scribe_state(mps);
            draw(mps, mps->cur_x, mps->cur_y, new_x, new_y);
            mps->cur_x = new_x;
            mps->cur_y = new_y;
            break;
        case 'J':       /* draw to (x,y) relative to relative origin */
            new_x = mps->rel_origin_x + mps->command_x;
            new_y = mps->rel_origin_y + mps->command_y;
            reset_scribe_state(mps);
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

static void print_char_select_colour(plot_t *mps, const uint8_t c)
{
    int value;

    value = numparser(&mps->colour_accu, c);

    if (value != INCOMPLETE) {
        mps->colour = value % 4;
    }
}

static void print_char_select_character_size(plot_t *mps, const uint8_t c)
{
    int value;

    value = numparser(&mps->charsize_accu, c);

    if (value != INCOMPLETE) {
        mps->charsize = 1 << (value % 4);
    }
}

static void print_char_select_character_rotation(plot_t *mps, const uint8_t c)
{
    int value;

    value = numparser(&mps->rotation_accu, c);

    if (value != INCOMPLETE) {
        mps->rotation = value % 2;
    }
}

static void print_char_select_scribe(plot_t *mps, const uint8_t c)
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

static void print_char_select_lowercase(plot_t *mps, const uint8_t c)
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
    reset_abs_origin(mps);
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
    mps->sheet = lib_calloc(Y_PIXELS, X_PIXELS * sizeof(uint8_t));
    mps->abs_origin_x = 0;
    mps->abs_origin_y = -VERTICAL_CLEARANCE;

    draw_four_squares(mps);
}

static void print_char_reset(plot_t *mps, const uint8_t c)
{
    if (c == CR) {
        eject(mps);     /* don't lose plotted data */
        power_on_reset(mps);
    }
}

/* ------------------------------------------------------------------------- */
/* Interface to the upper layer.  */

static int drv_1520_open(unsigned int prnr, unsigned int secondary)
{
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_open: sa=%u prnr=%u", secondary, prnr);
#endif

    /* Is this the first open? */
    if (secondary == DRIVER_FIRST_OPEN) {
        output_parameter_t output_parameter;
        plot_t *mps = &drv_1520[prnr];

        output_parameter.maxcol = X_PIXELS+1;
        output_parameter.maxrow = Y_PIXELS+1;
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
    log_message(drv1520_log, "drv_1520_close: sa=%u prnr=%u", secondary, prnr);
#endif

    /* Is this the last close? */
    if (secondary == DRIVER_LAST_CLOSE) {
        plot_t *mps = &drv_1520[prnr];

#if DEBUG1520
        log_message(drv1520_log, "drv_1520_close: last close");
#endif
        if (palette == NULL) {
            log_message(drv1520_log, "PALETTE ALREADY DEALLOCATED!!\n");
        }
        eject(mps);

        if (mps->sheet) {
            lib_free(mps->sheet);
            mps->sheet = NULL;
            output_select_close(prnr);
        }
    }
}

static int drv_1520_putc(unsigned int prnr, unsigned int secondary, uint8_t b)
{
    plot_t *mps = &drv_1520[prnr];

#if DEBUG1520
    log_message(drv1520_log, "drv_1520_putc: sa=%u b='%c' (%u) prnr=%u", secondary, b, b, prnr);
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

static int drv_1520_getc(unsigned int prnr, unsigned int secondary, uint8_t *b)
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
    output_select_formfeed(prnr);

    return 0;
}

int drv_1520_init_resources(void)
{
    driver_select_t driver;
#if DEBUG1520
    log_message(drv1520_log, "drv_1520_init_resources");
#endif

    driver = (driver_select_t){
        .drv_name     = "1520",
        .ui_name      = "Commodore 1520",

        .drv_open     = drv_1520_open,
        .drv_close    = drv_1520_close,
        .drv_putc     = drv_1520_putc,
        .drv_getc     = drv_1520_getc,
        .drv_flush    = drv_1520_flush,
        .drv_formfeed = drv_1520_formfeed,
        .drv_select   = NULL,

        .printer      = false,
        .plotter      = true,
        .iec          = true,
        .ieee488      = false,
        .userport     = false,
        .text         = false,
        .graphics     = true
    };
    driver_select_register(&driver);

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

    if (palette_load("1520.vpl", "PRINTER", palette) < 0) {
        log_error(drv1520_log, "Cannot load palette file `%s'.",
                  "1520.vpl");
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
    palette = NULL;
}
