/*
 * crtc.c - A line-based CRTC emulation (under construction).
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * 16/24bpp support added by
 *  Steven Tieu <stieu@physics.ubc.ca>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

/* #define DEBUG_CRTC */

#ifdef DEBUG_CRTC
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"
#include "clkguard.h"
#include "crtc-cmdline-options.h"
#include "crtc-color.h"
#include "crtc-draw.h"
#include "crtc-resources.h"
#include "crtc.h"
#include "crtctypes.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "raster-canvas.h"
#include "raster-changes.h"
#include "raster-line.h"
#include "raster-modes.h"
#include "resources.h"
#include "screenshot.h"
#include "types.h"
#include "vsync.h"
#include "video.h"
#include "viewport.h"


#define crtc_min(a, b)   (((a) < (b)) ? (a) : (b))


static void crtc_raster_draw_alarm_handler(CLOCK offset, void *data);


/*--------------------------------------------------------------------*/
/* CRTC variables */

/* the first variable is the initialized flag. We don't want that be
   uninitialized... */
crtc_t crtc = {
    0,              /* initialized */

    340,            /* screen_width */
    270,            /* screen_heigth */

    0,              /* hw_cursor */
    1,              /* hw_cols */
    0,              /* hw_blank */
    0x3ff,          /* vaddr_mask */
    0x2000,         /* vaddr_charswitch */
    512,            /* vaddr_charoffset */
    0x1000          /* vaddr_revswitch */
};

/* crtc-struct access functions */
#define CRTC_SCREEN_ADDR() \
    ((crtc.regs[13] | (crtc.regs[12] << 8)) & 0x3fff)

#define CRTC_SCREEN_TEXTCOLS() \
    (crtc.regs[1] * crtc.hw_cols)
#define CRTC_SCREEN_TEXTLINES() \
    (crtc.regs[6] & 0x7f)


#define CRTC_SCREEN_YPIX() \
    (CRTC_SCREEN_TEXTLINES() * (crtc_min(16, crtc.regs[9] + 1)))


#define CRTC_FIRST_DISPLAYED_LINE \
    CRTC_SCREEN_BORDERHEIGHT
#define CRTC_LAST_DISPLAYED_LINE \
    (crtc.screen_height - CRTC_SCREEN_BORDERHEIGHT - 1)

#define CRTC_CYCLES_PER_LINE() \
    crtc.regs[0]


/*--------------------------------------------------------------------*/
/* size/mode handling */
/*
 * So far we changed the window size according to the values poked
 * to the CRTC. Now we keep the window size fixed and try to position
 * the character array in there.
 *
 * The external hardware allows for a number of options.
 * Those are set with
 *
 *    crtc_set_hw_options(int hwflag, int vmask, int vchar, int vcoffset,
 *                                                      int vrevmask);
 *      hwflag & 1 -> hardware cursor available
 *      hwflag & 2 -> each CRTC character accounts for two chars on the
 *                    screen (hw_cols)
 *      vmask      -> the valid bits for the CRTC screen address (screen
 *                    buffer wraparound)
 *      vmchar     -> bit in CRTC screen address to switch to alternate
 *                    (second) charset
 *      vcoffset   -> how many chars the alternate charset of away
 *                    (CBM default: 512, because the charsets a 256 chars
 *                    for graphics/lowercase are switched elsewhere)
 *      vrevmask   -> bit in CRTC screen address to invert screen
 *
 * The screen that is attached to the CRTC can have different capabilities.
 * This function sets the expected size of the pixel area that is to
 * be used. Poking to the CRTC registers might change that, but the
 * video code positions the area in the window given here.
 *
 *    crtc_set_screen_options(int num_cols, int rasterlines);
 *      num_cols   -> 40 or 80
 *      rasterlines-> number of (text data) rasterlines (25*8, 25*14)
 *
 *
 * The CRTC memory and charset can be changed by the CPU. Those
 * functions tell the CRTC about it. The charset is always
 * organized as 16 raster bytes per char, one char after the other.
 *
 *    crtc_set_chargen_addr(BYTE *chargen, int cmask);
 *      chargen    -> pointer to base of charset
 *      clen       -> length of charset in chars (must be power of 2)
 *    crtc_set_chargen_offset(int offset);
 *      offset     -> offset of current charset in chargen, measured in chars
 *    crtc_set_screen_addr(BYTE *screen);
 *      screen    -> pointer to base of screen character array
 *
 * The above functions set the appropriate fields in the CRTC struct
 * and then call crtc_update_*().
 * The update functions check whether the CRTC is already initialized
 * and only then perform the appropriate action.
 * The update functions are also called in crtc_init() to
 * finish any resize/mode settings being made from the resources.
 *
 * Internal CRTC screen pointer handling:
 *
 * We assume that the screen address is increased every rasterline.
 * Only at frame reset (rasterline 0) the value is reloaded and
 * changes to the screen base register have effect.
 * This effects the selection of the chargen as well as the
 * screen pointer itself, also the mode selection.
 */
/*--------------------------------------------------------------------*/

/* reset the screen pointer at the beginning of the screen */
static inline void crtc_reset_screen_ptr(void)
{
    if (!crtc.initialized) {
        return;
    }

    crtc.screen_rel = ((CRTC_SCREEN_ADDR() & crtc.vaddr_mask)
                       * crtc.hw_cols);

    crtc.chargen_rel = (((CRTC_SCREEN_ADDR() & crtc.vaddr_charswitch)
                         ? crtc.vaddr_charoffset : 0)
                        | crtc.chargen_offset)
                       & crtc.chargen_mask;

    if ((crtc.vaddr_revswitch & crtc.vaddr_mask)
        || ((crtc.vaddr_revswitch < 0)
            && !(CRTC_SCREEN_ADDR() & (-crtc.vaddr_revswitch)))
        || ((crtc.vaddr_revswitch > 0)
            && (CRTC_SCREEN_ADDR() & crtc.vaddr_revswitch))) {
        /* standard mode */
        if (crtc.raster.video_mode != CRTC_STANDARD_MODE) {
            raster_changes_foreground_add_int(&crtc.raster, 0,
                                              &crtc.raster.video_mode,
                                              CRTC_STANDARD_MODE);
        }
    } else {
        /* reverse mode */
        if (crtc.raster.video_mode != CRTC_REVERSE_MODE) {
            raster_changes_foreground_add_int(&crtc.raster, 0,
                                              &crtc.raster.video_mode,
                                              CRTC_REVERSE_MODE);
        }
    }
}

/* update the chargen pointer when external switch has changed */
static inline void crtc_update_chargen_rel(void)
{
    if (!crtc.initialized) {
        return;
    }

    crtc.chargen_rel = ((crtc.chargen_rel & crtc.vaddr_charoffset)
                        | crtc.chargen_offset)
                       & crtc.chargen_mask;
}

/* update disp_char after writing to register 1 */
static inline void crtc_update_disp_char(void)
{
    if (!crtc.initialized) {
        return;
    }

    crtc.rl_visible = crtc.regs[1];
/*
    crtc.disp_chars = (crtc.rl_visible * (crtc.hw_double_cols ? 1 : 0));
*/
}

/* return pixel aspect ratio for current video mode */
/* FIXME: calculate proper values.
   look at http://www.codebase64.org/doku.php?id=base:pixel_aspect_ratio&s[]=aspect
   for an example calculation.
   The Fat-40 models have a different aspect ratio than the older,
   CRTC-less models, since they display their 40 characters in the same
   space as 80 characters, and the screen is wider than before.
*/
static float crtc_get_pixel_aspect(void)
{
/*
    int video;
    resources_get_int("MachineVideoStandard", &video);
    switch (video) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            return 0.936f;
        default:
            return 0.75f;
    }
*/
    return 1.0f; /* assume 1:1 for CRTC; corrected by 1x2 render mode */
}

/* return type of monitor used for current video mode */
static int crtc_get_crt_type(void)
{
    return 2; /* RGB */
}

/* update screen window */
void crtc_update_window(void)
{
    if (!crtc.initialized) {
        return;
    }

    crtc.raster.display_ystart = CRTC_SCREEN_BORDERHEIGHT;
    crtc.raster.display_ystop = crtc.screen_height
                                - 2 * CRTC_SCREEN_BORDERHEIGHT;
    crtc.raster.display_xstart = CRTC_SCREEN_BORDERWIDTH;
    crtc.raster.display_xstop = crtc.screen_width
                                - 2 * CRTC_SCREEN_BORDERWIDTH;

    crtc_update_renderer();

    raster_set_geometry(&crtc.raster,
                        crtc.screen_width, crtc.screen_height - 2 * CRTC_SCREEN_BORDERHEIGHT,
                        crtc.screen_width, crtc.screen_height,
                        crtc.screen_width - 2 * CRTC_SCREEN_BORDERWIDTH,
                        crtc.screen_height - 2 * CRTC_SCREEN_BORDERHEIGHT,
                        CRTC_SCREEN_TEXTCOLS(), CRTC_SCREEN_TEXTLINES(),
                        CRTC_SCREEN_BORDERWIDTH, CRTC_SCREEN_BORDERHEIGHT,
                        0,
                        CRTC_FIRST_DISPLAYED_LINE,
                        CRTC_LAST_DISPLAYED_LINE,
                        0, 0);

    crtc.raster.geometry->pixel_aspect_ratio = crtc_get_pixel_aspect();
    crtc.raster.viewport->crt_type = crtc_get_crt_type();
}

/*--------------------------------------------------------------------*/

void crtc_set_screen_addr(BYTE *screen)
{
    crtc.screen_base = screen;
}

void crtc_set_chargen_offset(int offset)
{
/* printf("crtc_set_chargen_offset(%d)\n",offset); */
    crtc.chargen_offset = offset << 4; /* times the number of bytes/char */

    crtc_update_chargen_rel();
}

void crtc_set_chargen_addr(BYTE *chargen, int cmask)
{
    crtc.chargen_base = chargen;
    crtc.chargen_mask = (cmask << 4) - 1;

    crtc_update_chargen_rel();
}

void crtc_set_screen_options(int num_cols, int rasterlines)
{
    crtc.screen_width = (num_cols + CRTC_EXTRA_COLS) * 8 + 2 * CRTC_SCREEN_BORDERWIDTH;
    crtc.screen_height = rasterlines + CRTC_EXTRA_RASTERLINES + 2 * CRTC_SCREEN_BORDERHEIGHT;

    DBG(("crtc_set_screen_options: cols=%d, rl=%d -> w=%d, h=%d",
         num_cols, rasterlines, crtc.screen_width, crtc.screen_height));

    crtc_update_window();
    resources_touch("CrtcDoubleSize");

    if (crtc.raster.canvas != NULL) {
        video_viewport_resize(crtc.raster.canvas, 1);
    }
}

void crtc_set_hw_options(int hwflag, int vmask, int vchar, int vcoffset,
                         int vrevmask)
{
    crtc.hw_cursor = hwflag & 1;
    crtc.hw_cols = (hwflag & 2) ? 2 : 1;
    crtc.vaddr_mask = vmask;
    crtc.vaddr_charswitch = vchar;
    crtc.vaddr_charoffset = vcoffset << 4; /* times the number of bytes/char */
    crtc.vaddr_revswitch = vrevmask;

    crtc_update_chargen_rel();
    crtc_update_disp_char();
}

void crtc_set_retrace_callback(machine_crtc_retrace_signal_t callback)
{
    crtc.retrace_callback = callback;
}

void crtc_set_retrace_type(int type)
{
    crtc.retrace_type = type;
}

void crtc_set_hires_draw_callback(crtc_hires_draw_t callback)
{
    crtc.hires_draw_callback = callback;
}

/*--------------------------------------------------------------------*/

static void clk_overflow_callback(CLOCK sub, void *data)
{
    crtc.frame_start -= sub;

    crtc.rl_start -= sub;
}

/*--------------------------------------------------------------------*/

raster_t *crtc_init(void)
{
    raster_t *raster;

    DBG(("crtc_init"));

    crtc.log = log_open("CRTC");

    crtc.raster_draw_alarm = alarm_new(maincpu_alarm_context, "CrtcRasterDraw",
                                       crtc_raster_draw_alarm_handler, NULL);

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    raster = &crtc.raster;

    raster->sprite_status = NULL;
    raster_line_changes_init(raster);

    if (raster_init(raster, CRTC_NUM_VMODES) < 0) {
        return NULL;
    }

    raster_modes_set_idle_mode(raster->modes, CRTC_IDLE_MODE);
    resources_touch("CrtcVideoCache");

    if (crtc_color_update_palette(raster->canvas) < 0) {
        log_error(crtc.log, "Cannot load palette.");
        return NULL;
    }

    if (!crtc.regs[0]) {
        crtc.regs[0] = 49;
    }
    if (!crtc.regs[1]) {
        crtc.regs[1] = 40;
    }
    if (!crtc.regs[2]) {
        crtc.regs[2] = 45;
    }
    if (!crtc.regs[4]) {
        crtc.regs[4] = 30;
    }
    if (!crtc.regs[6]) {
        crtc.regs[6] = 25;
    }
    if (!crtc.regs[9]) {
        crtc.regs[9] = 7;
    }

    /* FIXME */
    crtc.screen_xoffset = 0;
    crtc.screen_yoffset = CRTC_SCREEN_BORDERHEIGHT;
    crtc.retrace_callback = NULL;
    crtc.hires_draw_callback = NULL;

#if 0
    log_debug("scr_width=%d, scr_height=%d",
              crtc.screen_width, crtc.screen_height);
    log_debug("tcols=%d, tlines=%d, bwidth=%d, bheight=%d",
              CRTC_SCREEN_TEXTCOLS(), CRTC_SCREEN_TEXTLINES(),
              CRTC_SCREEN_BORDERWIDTH, CRTC_SCREEN_BORDERHEIGHT);
    log_debug("displayed lines: first=%d, last=%d",
              CRTC_FIRST_DISPLAYED_LINE, CRTC_LAST_DISPLAYED_LINE);
#endif

    crtc.initialized = 1;

    crtc_update_window();

    raster_set_title(raster, machine_name);

    if (raster_realize(raster) < 0) {
        return NULL;
    }

    crtc_update_chargen_rel();
    crtc_update_disp_char();
    crtc_reset_screen_ptr();

    crtc_draw_init();

    crtc_reset();
/*
    raster->display_ystart = CRTC_SCREEN_BORDERHEIGHT;
    raster->display_ystop = crtc.screen_height - 2 * CRTC_SCREEN_BORDERHEIGHT;
    raster->display_xstart = CRTC_SCREEN_BORDERWIDTH;
    raster->display_xstop = crtc.screen_width - 2 * CRTC_SCREEN_BORDERWIDTH;
*/
    resources_touch("CrtcDoubleSize");

    return &crtc.raster;
}

struct video_canvas_s *crtc_get_canvas(void)
{
    return crtc.raster.canvas;
}

/* Reset the CRTC chip.  */
void crtc_reset(void)
{
    raster_reset(&crtc.raster);

    alarm_set(crtc.raster_draw_alarm, CRTC_CYCLES_PER_LINE());

    crtc.rl_visible = crtc.regs[1];
    crtc.rl_sync = crtc.regs[2];
    crtc.rl_len = crtc.regs[0];
    crtc.prev_rl_visible = crtc.rl_visible;
    crtc.prev_rl_sync = crtc.rl_sync;
    crtc.prev_rl_len = crtc.rl_len;

    crtc.rl_start = maincpu_clk;
    crtc.frame_start = maincpu_clk;

    crtc_reset_screen_ptr();

    crtc.raster.ycounter = 0;
    crtc.current_charline = 0;
    crtc.current_line = 0;
    /* expected number of rasterlines for next frame */
    crtc.framelines = (crtc.regs[4] + 1) * (crtc.regs[9] + 1)
                      + crtc.regs[5];
}

/* Redraw the current raster line.  This happens at the last
   cycle of each line.  */
static void crtc_raster_draw_alarm_handler(CLOCK offset, void *data)
{
    int new_sync_diff;
    int new_venable;
    int new_vsync;
    int screen_width = (int)crtc.screen_width;

    new_venable = crtc.venable;
    new_vsync = crtc.vsync;

    /******************************************************************
     * handle one rasterline
     */

    /* first the time between two sync pulses */
    new_sync_diff = (crtc.prev_rl_len + 1 - crtc.prev_rl_sync)
                    + crtc.rl_sync;

    /* compute the horizontal position */
    /* the original PET displays have quite a variety of sync timings
       (or I haven't found the scheme yet). Therefore we cannot simply
       center the part between the syncs. We assume the sync in the
       first rasterline of the screen to be the default for the next
       frame. */

    /* FIXME: crtc.regs[3] & 15 == 0 -> 16 */
    if (crtc.raster.current_line == 0) {
        crtc.screen_xoffset = ((screen_width
                                - (crtc.sync_diff * 8 * crtc.hw_cols)) / 2)
                              + ((crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                                  - ((crtc.regs[3] & 0x0f) / 2))
                                 * 8 * crtc.hw_cols);

        /* FIXME: The 320 is the pixel width of a window with 40 cols.
           make that a define - or measure the visible line length?
           but how to do that reliably? */
        crtc.xoffset = CRTC_SCREEN_BORDERWIDTH + (CRTC_EXTRA_COLS * 4)
                       /* ((screen_width - crtc.rl_visible * 8 * crtc.hw_cols)
                       / 2) */
                       - crtc.screen_xoffset
                       + ((screen_width
                           - (crtc.sync_diff * 8 * crtc.hw_cols)) / 2)
                       + ((crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                           - ((crtc.regs[3] & 0x0f) / 2)) * 8 * crtc.hw_cols);
    }

    /* emulate the line */
    if (crtc.raster.current_line >=
        crtc.screen_height - 2 * CRTC_SCREEN_BORDERHEIGHT) {
        /* FIXFRAME: crtc.raster.current_line ++; */
    } else {
        raster_line_emulate(&crtc.raster);
    }

    /* now add jitter if this is out of phase (sync_diff changes) */
    crtc.hjitter -= (new_sync_diff - crtc.sync_diff) * 4 * crtc.hw_cols;
    if (crtc.hjitter > 16) {
        crtc.hjitter = 16;
    }
    if (crtc.hjitter < -16) {
        crtc.hjitter = -16;
    }
    /* exponential/sine decay */
    crtc.hjitter = (int)((double)(crtc.hjitter) * -0.5);
/*
    if (crtc.hjitter) {
        printf("rl=%d, jitter=%d, sync_diff=%d, old diff=%d, \n",
               crtc.raster.current_line, crtc.hjitter,
               new_sync_diff, crtc.sync_diff);
    }
*/
    crtc.sync_diff = new_sync_diff;

/*
    if (crtc.raster.current_line == 10) {
        printf("centering=%d, sync2start=%d -> xoff=%d, jitter=%d\n",
                ((screen_width - (sync_diff * crtc.hw_cols * 8)) / 2),
                (crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                - ((crtc.regs[3] & 0x0f) / 2)),
                ((screen_width - (sync_diff * crtc.hw_cols * 8)) / 2)
                + ((crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                - ((crtc.regs[3] & 0x0f) / 2)) * crtc.hw_cols * 8),
                crtc.hjitter);
    }
*/

    crtc.prev_rl_visible = crtc.rl_visible;
    crtc.prev_rl_sync = crtc.rl_sync;
    crtc.prev_rl_len = crtc.rl_len;
    crtc.prev_screen_rel = crtc.screen_rel;

    crtc.rl_visible = crtc.regs[1];
    crtc.rl_sync = crtc.regs[2];
    crtc.rl_len = crtc.regs[0];

    crtc.rl_start = maincpu_clk - offset;

    /******************************************************************
     * handle the rasterline numbering
     */

    crtc.current_line++;
    /* FIXFRAME; crtc.framelines --;

    if (crtc.framelines == crtc.screen_yoffset) {
*/
    if ((crtc.framelines - crtc.current_line) == crtc.screen_yoffset) {
        crtc.raster.current_line = 0;
        raster_canvas_handle_end_of_frame(&crtc.raster);
        raster_skip_frame(&crtc.raster,
                          vsync_do_vsync(crtc.raster.canvas,
                                         crtc.raster.skip_frame));
    }

    {
        /* FIXME: charheight */
        if (crtc.current_charline >= crtc.regs[4] + 1) {
            if ((crtc.raster.ycounter + 1) >= crtc.regs[5]) {
                long cycles;

                /* Do vsync stuff.  */
                /* printf("new screen at clk=%d\n",crtc.rl_start); */
                crtc_reset_screen_ptr();
                crtc.raster.ycounter = 0;
                crtc.current_charline = 0;
                new_venable = 1;

                /* expected number of rasterlines for next frame */
                crtc.framelines = crtc.current_line;
                crtc.current_line = 0;

                /* hardware cursor handling */
                if (crtc.crsrmode & 2) {
                    crtc.crsrcnt--;
                    if (!crtc.crsrcnt) {
                        crtc.crsrcnt = (crtc.crsrmode & 1) ? 16 : 32;
                        crtc.crsrstate ^= 1;
                    }
                }

                /* cycles per frame, for speed adjustments */
                cycles = crtc.rl_start - crtc.frame_start;
                if (crtc.frame_start && (cycles != crtc.cycles_per_frame)) {
                    machine_set_cycles_per_frame(cycles);
                    crtc.cycles_per_frame = cycles;
                }
                crtc.frame_start = crtc.rl_start;
            } else {
                crtc.raster.ycounter++;
            }
        } else {
            if (crtc.raster.ycounter != crtc.regs[9]) {
                crtc.raster.ycounter++;
                crtc.raster.ycounter &= 0x1f;
            } else {
                crtc.raster.ycounter = 0;
                crtc.current_charline++;
                crtc.current_charline &= 0x7f;

                if (crtc.henable) {
                    crtc.screen_rel += crtc.rl_visible * crtc.hw_cols;
                }
                if (crtc.current_charline == crtc.regs[6]) {
                    new_venable = 0;
                }
                if (crtc.current_charline == crtc.regs[7]) {
                    /* printf("hsync starts at clk=%d\n",crtc.rl_start); */
                    new_vsync = (crtc.regs[3] >> 4) & 0x0f;
                    if (!new_vsync) {
                        new_vsync = 16;
                    }
                    new_vsync++;  /* compensate for the first decrease below */
                }
            }
            if (crtc.raster.ycounter == (unsigned int)(crtc.regs[10] & 0x1f)) {
                crtc.cursor_lines = 1;
            } else
            if (crtc.raster.ycounter
                == (unsigned int)((crtc.regs[11] + 1) & 0x1f)) {
                crtc.cursor_lines = 0;
            }

            crtc.henable = 1;
        }
        if (new_vsync) {
            new_vsync--;
        }
    }

    /******************************************************************
     * signal retrace to CPU
     */

    if (crtc.retrace_callback) {
        if (crtc.retrace_type & 1) {
            if (crtc.vsync && !new_vsync) {
                crtc.retrace_callback(0);
            } else
            if (new_vsync && !crtc.vsync) {
                crtc.retrace_callback(1);
            }
        } else {
            if (crtc.venable && !new_venable) {
                crtc.retrace_callback(1);
            } else
            if (new_venable && !crtc.venable) {
                crtc.retrace_callback(0);
            }
        }
    }
/*
    if (crtc.venable && !new_venable)
        printf("disable ven, cl=%d, yc=%d, rl=%d\n",
                crtc.current_charline, crtc.raster.ycounter,
                crtc.raster.current_line);
    if (new_venable && !crtc.venable)
        printf("enable ven, cl=%d, yc=%d, rl=%d\n",
                crtc.current_charline, crtc.raster.ycounter,
                crtc.raster.current_line);
*/
/*
    if (crtc.vsync && !new_vsync)
        printf("disable vsync, cl=%d, yc=%d, rl=%d\n",
                crtc.current_charline, crtc.raster.ycounter,
                crtc.raster.current_line);
    if (new_vsync && !crtc.vsync)
        printf("enable vsync, cl=%d, yc=%d, rl=%d\n",
                crtc.current_charline, crtc.raster.ycounter,
                crtc.raster.current_line);
*/
    if (crtc.venable && !new_venable) {
        /* visible area ends here - try to compute vertical centering */
        /* FIXME: count actual number of rasterlines */
        int visible_height = crtc.current_line;
        /* crtc.regs[6] * (crtc.regs[9] + 1); */

        crtc.screen_yoffset = ((int)crtc.screen_height - visible_height) / 2;
        if (crtc.screen_yoffset < CRTC_SCREEN_BORDERHEIGHT) {
            crtc.screen_yoffset = CRTC_SCREEN_BORDERHEIGHT;
        }

/* printf("visible_height=%d -> yoffset=%d\n",
                                visible_height, crtc.screen_height); */
    }

    crtc.venable = new_venable;
    crtc.vsync = new_vsync;

    crtc.raster.blank_this_line = (crtc.hw_blank && crtc.blank)
                                  || !new_venable;

    /******************************************************************
     * set up new alarm
     */

    alarm_set(crtc.raster_draw_alarm, crtc.rl_start + crtc.rl_len + 1);
}

void crtc_shutdown(void)
{
    raster_shutdown(&crtc.raster);
}

/* ------------------------------------------------------------------- */

int crtc_offscreen(void)
{
    if (crtc.retrace_type & 1) {
        if (crtc.vsync) {
            return 1;
        }
    } else {
        if (!crtc.venable) {
            return 1;
        }
    }
    return 0;
}

void crtc_screen_enable(int enable)
{
    crtc.blank = !enable;
}

void crtc_enable_hw_screen_blank(int enable)
{
    crtc.hw_blank = enable;
}

/* cols60 is 80 cols with 40 cols timing */
static BYTE cols40[1] = { 40 };
static BYTE cols60[1] = { 60 };
static BYTE cols80[1] = { 80 };

static BYTE charh8[1] = { 8 };
static BYTE charh14[1] = { 14 };

void crtc_screenshot(screenshot_t *screenshot)
{
    raster_screenshot(&crtc.raster, screenshot);

    screenshot->chipid = "CRTC";
    screenshot->video_regs = crtc.regs;
    screenshot->screen_ptr = crtc.screen_base;
    screenshot->chargen_ptr = crtc.chargen_base + crtc.chargen_rel;

    /* Use the bitmap_ptr for possibly enabled hires graphics adapters (dww/hre) */
    screenshot->bitmap_ptr = crtc_get_active_bitmap();

    /* Use the bitmap_low_ptr as indicator for the amount of hw columns */
    if (crtc.hw_cols & 2) {
        screenshot->bitmap_low_ptr = cols80;
    } else {
        if (crtc.vaddr_mask == 0x7ff) {
            screenshot->bitmap_low_ptr = cols60;
        } else {
            screenshot->bitmap_low_ptr = cols40;
        }
    }

    /* Use the bitmap_high_ptr as indicator for the height of the chars */
    if (crtc.screen_height == 382) {
        screenshot->bitmap_high_ptr = charh14;
    } else {
        screenshot->bitmap_high_ptr = charh8;
    }
    screenshot->color_ram_ptr = NULL;
}

void crtc_async_refresh(struct canvas_refresh_s *refresh)
{
    raster_async_refresh(&crtc.raster, refresh);
}

int crtc_dump(struct crtc_s *crtc_context)
{
    /* FIXME: dump details using mon_out(). return 0 on success */
    return -1;
}
