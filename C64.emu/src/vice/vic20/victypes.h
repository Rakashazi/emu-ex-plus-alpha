/*
 * victypes.h
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

#ifndef VICE_VICTYPES_H
#define VICE_VICTYPES_H

#include "vice.h"

#include "raster.h"
#include "types.h"

#define VIC_PAL_SCREEN_WIDTH            284     /* 71 cycles * 4 pixels */
#define VIC_NTSC_SCREEN_WIDTH           260     /* 65 cycles * 4 pixels */

#define VIC_PAL_MAX_TEXT_COLS           32
#define VIC_NTSC_MAX_TEXT_COLS          31

#define VIC_MAX_TEXT_COLS               VIC_PAL_MAX_TEXT_COLS

#define VIC_PAL_DISPLAY_WIDTH           224 /* FIXME: REMOVE */
#define VIC_NTSC_DISPLAY_WIDTH          200 /* FIXME: REMOVE */

#define VIC_NUM_COLORS 16

/* This is the only machine that needs those defines.  (MSDOS?, OS2?) */
#define RASTER_PIXEL(c) (vic.pixel_table.sing[(c)])

/* FIXME: MSDOS does not need double pixel.
`ifdef' them out once all video chips actually honour this.  */
#define RASTER_PIXEL2(c) (vic.pixel_table.doub[(c)])

/* On MS-DOS, do not duplicate pixels.  Otherwise, we would always need at
   least 466 horizontal pixels to contain the whole screen.  */
/* But this is no problem as 320*200 does not fit anyhow.  */
#if !defined(__OS2__) && !defined(DINGUX_SDL) && !defined(DINGOO_NATIVE) && !defined(ANDROID_COMPILE)
#define VIC_DUPLICATES_PIXELS
#endif

#ifdef VIC_DUPLICATES_PIXELS
typedef WORD VIC_PIXEL;
#define VIC_PIXEL(n)    RASTER_PIXEL2(n)
#define VIC_PIXEL_WIDTH 2
#define VIC_PIXEL_WIDTH_SHIFT 1
#else
typedef BYTE VIC_PIXEL;
#define VIC_PIXEL(n)    RASTER_PIXEL(n)
#define VIC_PIXEL_WIDTH 1
#define VIC_PIXEL_WIDTH_SHIFT 0
#endif

/* Cycle # within the current line.  */
#define VIC_RASTER_CYCLE(clk) ((unsigned int)((clk) % vic.cycles_per_line))

/* `clk' value for the beginning of the current line.  */
#define VIC_LINE_START_CLK(clk)  (((clk) / vic.cycles_per_line) \
                                  * vic.cycles_per_line)

/* Current vertical position of the raster.  Unlike `rasterline', which is
   only accurate if a pending `A_RASTERDRAW' event has been served, this is
   guarranteed to be always correct.  It is a bit slow, though.  */
#define VIC_RASTER_Y(clk)     ((unsigned int)((clk) / vic.cycles_per_line)   \
                               % vic.screen_height)

#define VIC_RASTER_X(cycle)      (((int)(cycle) - 7) * 4 * VIC_PIXEL_WIDTH)

/* char affected by a change in this cycle rounded to whole chars */
#define VIC_RASTER_CHAR(cycle)   ((int)((cycle) - vic.raster.display_xstart / (VIC_PIXEL_WIDTH * 4) - 6) / 2)

/* char affected by a change in this cycle */
#define VIC_RASTER_CHAR_INT(cycle)   ((int)((cycle) - vic.raster.display_xstart / (VIC_PIXEL_WIDTH * 4) - 7) / 2)
/* half of the char affected by a change in this cycle */
#define VIC_RASTER_CHAR_FRAC(cycle)   ((int)((cycle) - vic.raster.display_xstart / (VIC_PIXEL_WIDTH * 4) - 7) % 2)

/* Video mode definitions. */

enum vic_video_mode_s {
    VIC_STANDARD_MODE,
    VIC_NUM_VMODES
};
typedef enum vic_video_mode_s vic_video_mode_t;

#define VIC_IDLE_MODE VIC_STANDARD_MODE

struct snapshot_s;
struct screenshot_s;
struct palette_s;
struct canvas_refresh_s;

struct vic_light_pen_s {
    int state;
    int triggered;
    int x, y, x_extra_bits;
    CLOCK trigger_cycle;
};
typedef struct vic_light_pen_s vic_light_pen_t;

enum vic_fetch_state_s {
    /* fetch has not started yet */
    VIC_FETCH_IDLE,
    /* fetch starting */
    VIC_FETCH_START,
    /* fetch from screen/color memomy */
    VIC_FETCH_MATRIX,
    /* fetch from chargen */
    VIC_FETCH_CHARGEN,
    /* fetch done on current line */
    VIC_FETCH_DONE
};
typedef enum vic_fetch_state_s vic_fetch_state_t;

enum vic_area_state_s {
    /* v-flipflop has not opened yet (upper border) */
    VIC_AREA_IDLE,
    /* v-flipflop has just been opened, first line not reached yet */
    VIC_AREA_PENDING,
    /* normal display area */
    VIC_AREA_DISPLAY,
    /* v-flipflop has closed (lower border) */
    VIC_AREA_DONE
};
typedef enum vic_area_state_s vic_area_state_t;

struct video_chip_cap_s;

struct vic_s {
    int initialized;

    signed int log;

    raster_t raster;

    struct palette_s *palette;

    BYTE regs[0x10];

    /* Cycle # within the current line.  */
    unsigned int raster_cycle;

    /* Current line.  */
    unsigned int raster_line;

    int auxiliary_color;
    int mc_border_color;
    int reverse;
    int old_auxiliary_color;
    int old_mc_border_color;
    int old_reverse;
    int half_char_flag;

    unsigned int char_height;   /* changes immediately for memory fetch */
    unsigned int row_increase_line; /* may change next line for row count */
    unsigned int text_cols;     /* = 22 */
    unsigned int text_lines;    /* = 23 */
    unsigned int pending_text_cols;
    unsigned int line_was_blank;

    unsigned int memptr;

    /* offset for screen memory pointer */
    unsigned int memptr_inc;

    /* counting the text lines in the current frame */
    unsigned int row_counter;

    /* area in the frame */
    vic_area_state_t area;

    /* fetch state */
    vic_fetch_state_t fetch_state;

    /* Screen memory buffer (1 char) */
    BYTE vbuf;

    /* Offset to the cbuf/gbuf buffers */
    unsigned int buf_offset;

    /* Color memory buffer */
    BYTE cbuf[VIC_MAX_TEXT_COLS];

    /* Graphics buffer (chargen/bitmap) */
    BYTE gbuf[VIC_MAX_TEXT_COLS];

    unsigned int cycles_per_line;
    unsigned int screen_height;
    unsigned int first_displayed_line;
    unsigned int last_displayed_line;
    unsigned int screen_width;
    unsigned int display_width;
    unsigned int cycle_offset;
    unsigned int max_text_cols;
    int screen_leftborderwidth;
    int screen_rightborderwidth;

    vic_light_pen_t light_pen;

    /* Video chip capabilities.  */
    struct video_chip_cap_s *video_chip_cap;

    struct {
        BYTE sing[0x100];
        WORD doub[0x100];
    } pixel_table;
};
typedef struct vic_s vic_t;

extern vic_t vic;

#endif
