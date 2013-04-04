/*
 * viewport.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_VIEWPORT_H
#define VICE_VIEWPORT_H

#include "types.h"

/* A simple convenience type for defining rectangular areas.  */
struct rectangle_s {
    unsigned int width;
    unsigned int height;
};
typedef struct rectangle_s rectangle_t;

/* A simple convenience type for defining screen positions.  */
struct position_s {
    unsigned int x;
    unsigned int y;
};
typedef struct position_s position_t;

struct viewport_s {
    /* Title for the viewport.  FIXME: Duplicated info from the canvas?  */
    char *title;

    /* Offset of the screen on the window.  */
    unsigned int x_offset, y_offset;

    /* First and last lines shown in the output window.  */
    unsigned int first_line, last_line;

    /* First pixel in one line of the frame buffer to be shown on the output
       window.  */
    unsigned int first_x;

    /* Only display canvas if this flag is set.  */
    int update_canvas;

    /* type of emulated monitor, used to select renderer */
    int crt_type;
};
typedef struct viewport_s viewport_t;

struct geometry_s {
    /* Total size of the screen, including borders and unused areas.
       (SCREEN_WIDTH, SCREEN_HEIGHT)  */
    rectangle_t screen_size;

    /* Size of the graphics area (i.e. excluding borders and unused areas.
       (SCREEN_XPIX, SCREEN_YPIX)  */
    rectangle_t gfx_size;

    /* Size of the text area.  (SCREEN_TEXTCOLS)  */
    rectangle_t text_size;

    /* Position of the graphics area.  (SCREEN_BORDERWIDTH,
       SCREEN_BORDERHEIGHT) */
    position_t gfx_position;

    /* If nonzero, `gfx_position' is expected to be moved around when poking
       to the chip registers.  */
    int gfx_area_moves;

    /* FIXME: Bad names.  */
    unsigned int first_displayed_line, last_displayed_line;

    unsigned int extra_offscreen_border_left;
    unsigned int extra_offscreen_border_right;
    /* true pixel aspect ratio of the current video mode */
    float pixel_aspect_ratio;
    /* width of each char in pixels (usually 8, but VDC on X128 can change this) */
    unsigned int char_pixel_width;
};
typedef struct geometry_s geometry_t;

#endif
