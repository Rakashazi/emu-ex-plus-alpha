/*
 * tedtypes.h - A cycle-exact event-driven TED emulation.
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

#ifndef VICE_TEDTYPES_H
#define VICE_TEDTYPES_H

#include "raster.h"
#include "types.h"

/* Screen constants.  */
#define TED_SCREEN_XPIX                 320
#define TED_SCREEN_YPIX                 200
#define TED_SCREEN_TEXTCOLS             40
#define TED_SCREEN_TEXTLINES            25

/*
#define TED_40COL_START_PIXEL           0x20
#define TED_40COL_STOP_PIXEL            0x160
#define TED_38COL_START_PIXEL           0x28
#define TED_38COL_STOP_PIXEL            0x158
*/

#define TED_40COL_START_PIXEL ted.screen_leftborderwidth
#define TED_40COL_STOP_PIXEL  (ted.screen_leftborderwidth + TED_SCREEN_XPIX)
#define TED_38COL_START_PIXEL (ted.screen_leftborderwidth + 7)
#define TED_38COL_STOP_PIXEL  (ted.screen_leftborderwidth + 311)

/* FIXME don't need */
#define TED_PAL_OFFSET                  48
#define TED_NTSC_OFFSET                 0 /* FIXME */

/* values in TED raster counter */
/* 0x004 in TED raster counter */
#define TED_PAL_25ROW_START_LINE        4
/* 0x0CB in TED raster counter */
#define TED_PAL_25ROW_STOP_LINE         0xcb
/* 0x008 in TED raster counter */
#define TED_PAL_24ROW_START_LINE        8
/* 0x0C7 in TED raster counter */
#define TED_PAL_24ROW_STOP_LINE         0xc7

/* FIXME calculate NTSC values */
/*
#define TED_NTSC_25ROW_START_LINE       (0x33 - TED_NTSC_OFFSET)
#define TED_NTSC_25ROW_STOP_LINE        (0xfb - TED_NTSC_OFFSET)
#define TED_NTSC_24ROW_START_LINE       (0x37 - TED_NTSC_OFFSET)
#define TED_NTSC_24ROW_STOP_LINE        (0xf7 - TED_NTSC_OFFSET)
*/
#define TED_NTSC_25ROW_START_LINE       4
#define TED_NTSC_25ROW_STOP_LINE        0xcb
#define TED_NTSC_24ROW_START_LINE       8
#define TED_NTSC_24ROW_STOP_LINE        0xc7

/* TED raster counter values */
#define TED_PAL_VSYNC_LINE              257
#define TED_NTSC_VSYNC_LINE             229

/* FIXME add negated colors as well */
#define TED_NUM_COLORS                  128


/* Available video modes.  The number is given by TED registers.  */
enum ted_video_mode_s {
    TED_NORMAL_TEXT_MODE,
    TED_MULTICOLOR_TEXT_MODE,
    TED_HIRES_BITMAP_MODE,
    TED_MULTICOLOR_BITMAP_MODE,
    TED_EXTENDED_TEXT_MODE,
    TED_ILLEGAL_TEXT_MODE,
    TED_ILLEGAL_BITMAP_MODE_1,
    TED_ILLEGAL_BITMAP_MODE_2,
    TED_IDLE_MODE,           /* Special mode for idle state.  */
    TED_NUM_VMODES
};
typedef enum ted_video_mode_s ted_video_mode_t;

#define TED_IS_ILLEGAL_MODE(x)       ((x) >= TED_ILLEGAL_TEXT_MODE && (x) != TED_IDLE_MODE)
#define TED_IS_BITMAP_MODE(x)        ((x) & 0x02)

/* Note: we measure cycles from 0 to 113, not from 1 to 114.  */

/* Cycle # at which the TED takes the bus in a bad line (BA goes low).  */
#define TED_FETCH_CYCLE             4

/* Delay for the raster line interrupt.  This is not due to the TED, since
   it triggers the IRQ line at the beginning of the line, but to the 7501
   that needs at least 2 cycles to detect it.  */
#define TED_RASTER_IRQ_DELAY        2 /* FIXME!!! */

/* Current char being drawn by the raster.  < 0 or >= TED_SCREEN_TEXTCOLS
   if outside the visible range.  */
#define TED_RASTER_CHAR(cycle)      (((int)(cycle) - 15) / 2 )

/* Current horizontal position (in pixels) of the raster.  < 0 or >=
   SCREEN_WIDTH if outside the visible range.  */
/* #define TED_RASTER_X(cycle)         (((int)(cycle) - 7) * 4) */
#define TED_RASTER_X(cycle)         ((((int)(cycle) - 15) * 4) + ted.screen_leftborderwidth)

/* Current vertical position of the raster.  Unlike `rasterline', which is
   only accurate if a pending drawing event has been served, this is
   guaranteed to be always correct. */
#define TED_RASTER_Y(clk)           ((unsigned int)((ted.ted_raster_counter \
                                                     + (((clk) - ted.last_emulate_line_clk) \
                                                        >= 114 ? (ted.ted_raster_counter == ted.screen_height - 1 \
                                                                  ? 1 - ted.screen_height : 1) : 0)) & 0x1ff))

/* Cycle # within the current line.  */
#define TED_RASTER_CYCLE(clk)       ((unsigned int)((clk) - ted.last_emulate_line_clk - (((clk) - ted.last_emulate_line_clk) >= 114 ? 114 : 0)))

/* `clk' value for the beginning of the current line.  */
#define TED_LINE_START_CLK(clk)     ((unsigned int)(ted.last_emulate_line_clk + (((clk) - ted.last_emulate_line_clk) >= 114 ? 114 : 0)))

/* # of the previous and next raster line.  Handles wrap over.  */
/* FIXME not always true, previous line can be 511 */
#define TED_PREVIOUS_LINE(line)  (((line) > 0) ? (line) - 1 : ted.screen_height - 1)
/* FIXME not always true, line counter can be in range [screen_height, 511] */
#define TED_NEXT_LINE(line)      (((line) + 1) % ted.screen_height)

/* FIXME not used can be dropped */
#define TED_LINE_RTOU(line) ((line + ted.screen_height - ted.offset) % ted.screen_height)
#define TED_LINE_UTOR(line) ((line + ted.screen_height + ted.offset) % ted.screen_height)

/* Bad line range.  */
/* TED raster_counter values */
#define TED_PAL_FIRST_DMA_LINE      0x0
#define TED_PAL_LAST_DMA_LINE       0xcb

/* FIXME: verify ntsc values */
/*
#define TED_NTSC_FIRST_DMA_LINE     (0x30 - TED_NTSC_OFFSET)
#define TED_NTSC_LAST_DMA_LINE      0xf7
*/
#define TED_NTSC_FIRST_DMA_LINE     0x0   /* FIXME */
#define TED_NTSC_LAST_DMA_LINE      0xcb  /* FIXME */

/* TED structures.  This is meant to be used by TED modules
   *exclusively*!  */

/*
enum ted_fetch_idx_s {
    TED_FETCH_MATRIX,
    TED_FETCH_COLOR,
};
typedef enum ted_fetch_idx_s ted_fetch_idx_t;
*/

/* FIXME Idle location is always $ffff in TED or the data is coming from CPU cycles in certain cases */
enum ted_idle_data_location_s {
    IDLE_NONE,
    IDLE_3FFF,
    IDLE_39FF
};
typedef enum ted_idle_data_location_s ted_idle_data_location_t;

struct alarm_s;

struct ted_s {
    /* Flag: Are we initialized?  */
    int initialized;            /* = 0; */

    /* TED raster.  */
    raster_t raster;

    /* TED registers.  */
    BYTE regs[64];

    /* Interrupt register.  */
    int irq_status;             /* = 0; */

    /* Line for raster compare IRQ.  */
    unsigned int raster_irq_line;

    /* Video memory pointers.  */
    BYTE *screen_ptr;
    BYTE *chargen_ptr;
    BYTE *bitmap_ptr;
    BYTE *color_ptr;

    /* Screen memory buffers (chars and color).  */
    BYTE vbuf[TED_SCREEN_TEXTCOLS];
    BYTE cbuf[TED_SCREEN_TEXTCOLS];
    BYTE cbuf_tmp[TED_SCREEN_TEXTCOLS];

    /* If this flag is set, bad lines (DMA's) can happen.  */
    int allow_bad_lines;

    /* Extended background colors (1, 2 and 3).  */
    int ext_background_color[3];

    /* Flag: is reverse mode enabled or not (bit 7 of $ff07) */
    int reverse_mode;

    /* Flag: are we in idle state? */
    int idle_state;

    /* Flag: should we force display (i.e. non-idle) state for the following
       line? */
    int force_display_state;

    /* Which display line is drawn? */
    unsigned int tv_current_line;
    unsigned int ted_raster_counter;

    /* This flag is set if a memory fetch has already happened on the current
       line.  FIXME: Value of 2?...  */
    int memory_fetch_done;

    /* Internal memory pointer (VCBASE).  */
    int memptr;
    int memptr_col;

    /* Internal memory counter (VC).  */
    int mem_counter;
    /* For bitmap fetch */
    int chr_pos_reload;
    int chr_pos_count;
    int chr_pos_inc_enable;

    /* Value to add to `mem_counter' after the graphics has been painted.  */
    int mem_counter_inc;

    /* Flag: is the current line a `bad' line? */
    int bad_line;

    /* Is the cursor visible?  */
    int cursor_visible;

    /* Cursor interval counter.  */
    int cursor_phase;

    /* Cursor position.  */
    int crsrpos;

    /* Flag: Check for raster.ycounter reset already done on this line?
       (cycle 13) */
    int ycounter_reset_checked;

    /* Flag: Does the currently selected video mode force the overscan
       background color to be black?  (This happens with the hires bitmap and
       illegal modes.)  */
    int force_black_overscan_background_color;

    /* Data to display in idle state.  */
    int idle_data;

    /* Where do we currently fetch idle stata from?  If `IDLE_NONE', we are
       not in idle state and thus do not need to update `idle_data'.  */
    ted_idle_data_location_t idle_data_location;

    /* TED keybaord read value.  */
    BYTE kbdval;

    /* All the TED logging goes here.  */
    signed int log;

    /* TED alarms.  */
    struct alarm_s *raster_fetch_alarm;
    struct alarm_s *raster_draw_alarm;
    struct alarm_s *raster_irq_alarm;
#if 0
    /* What do we do when the `A_RASTERFETCH' event happens?  */
    ted_fetch_idx_t fetch_idx;
#endif
    /* Clock cycle for the next "raster fetch" alarm.  */
    CLOCK fetch_clk;

    /* Clock cycle for the next "raster draw" alarm.  */
    CLOCK draw_clk;

    /* Clock value for raster compare IRQ.  */
    CLOCK raster_irq_clk;

    /* FIXME: Bad name.  FIXME: Has to be initialized.  */
    CLOCK last_emulate_line_clk;

    /* Geometry and timing parameters of the selected TED emulation.  */
    unsigned int screen_height;
    int first_displayed_line;
    int last_displayed_line;

    unsigned int row_25_start_line;
    unsigned int row_25_stop_line;
    unsigned int row_24_start_line;
    unsigned int row_24_stop_line;

    int screen_leftborderwidth;
    int screen_rightborderwidth;

    int cycles_per_line;
    int draw_cycle;

    unsigned int first_dma_line;
    unsigned int last_dma_line;

    unsigned int vsync_line;

    /* Number of lines the whole screen is shifted up.  */
    int offset;

    /* TED clock mode.  */
    unsigned int fastmode;

    int character_fetch_on;

    /* Last value read from TED (used for RMW access).  */
    BYTE last_read;

    /* Video chip capabilities.  */
    struct video_chip_cap_s *video_chip_cap;

    unsigned int int_num;
};
typedef struct ted_s ted_t;

extern ted_t ted;

/* Private function calls, used by the other TED modules.  */
extern void ted_update_memory_ptrs(unsigned int cycle);
extern void ted_update_video_mode(unsigned int cycle);
extern void ted_raster_draw_alarm_handler(CLOCK offset, void *data);
/* extern void ted_resize(void); */
extern void ted_delay_clk(void);
extern void ted_delay_oldclk(CLOCK num);

/* Debugging options.  */

/* #define TED_VMODE_DEBUG */
/* #define TED_RASTER_DEBUG */
/* #define TED_REGISTERS_DEBUG */

#ifdef TED_VMODE_DEBUG
#define TED_DEBUG_VMODE(x) log_debug x
#else
#define TED_DEBUG_VMODE(x)
#endif

#ifdef TED_RASTER_DEBUG
#define TED_DEBUG_RASTER(x) log_debug x
#else
#define TED_DEBUG_RASTER(x)
#endif

#ifdef TED_REGISTERS_DEBUG
#define TED_DEBUG_REGISTER(x) log_debug x
#else
#define TED_DEBUG_REGISTER(x)
#endif

#endif
