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
#include "crtc-cmdline-options.h"
#include "crtc-color.h"
#include "crtc-draw.h"
#include "crtc-mem.h"
#include "crtc-resources.h"
#include "crtc.h"
#include "crtctypes.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
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
#if CRTC_BEAM_RACING
inline void crtc_fetch_prefetch(void);
static void crtc_adjusted_retrace_alarm_handler(CLOCK offset, void *data);
#endif


/*--------------------------------------------------------------------*/
/* CRTC variables */

/* the first variable is the initialized flag. We don't want that be uninitialized... */
/* FIXME: do not statically initialize anything in this struct, do this somewhere
          else at runtime */
crtc_t crtc = {
    .initialized =      0,

    .screen_width =     340,
    .screen_height =    270,

    .hw_cursor =        0,
    .hw_cols =          1,
    .hw_blank =         0,
    .beam_offset =      0,
    .vaddr_mask =       0x3ff,
    .vaddr_charswitch = 0x2000,
    .vaddr_charoffset = 512,
    .vaddr_revswitch =  0x1000,

    .screen_base =      NULL,
    .chargen_base =     NULL,
    .chargen_mask =     0,
    .chargen_offset =   0,

    .chargen_rel =      0,
    .screen_rel =       0,

    .regno =            0,

    .rl_start =         0,
    .rl_visible =       0,
    .rl_sync =          0,
    .rl_len =           0,
    .sync_diff =        0,

    .prev_rl_visible =  0,
    .prev_rl_sync =     0,
    .prev_rl_len =      0,
    .prev_screen_rel =  0,

    .hjitter =          0,
    .xoffset =          0,
    .screen_xoffset =   0,
    .screen_hsync =     0,
    .screen_yoffset =   0,

    .henable =          0,

    .current_line =     0,
    .framelines =       0,
    .venable =          0,
    .vsync =            0,

    .current_charline = 0,

    .blank =            0,

    .frame_start =      0,
    .cycles_per_frame = 0,

    .crsrmode =         0,
    .crsrcnt =          0,
    .crsrstate =        0,
    .cursor_lines =     0,

    .retrace_callback = NULL,
    .hires_draw_callback = NULL,
    .retrace_type = 0,

    .log = 0,

    /* raster: an instance of raster_t (see src/raster/raster.h) */
    .raster = { 0 },

    /* regs */
    .regs = { 0 },

    .raster_draw_alarm = NULL,
#if CRTC_BEAM_RACING
    .adjusted_retrace_alarm = NULL,
    .prefetch = { 0 }
#endif
};

/* crtc-struct access functions */
#define CRTC_SCREEN_ADDR() \
    ((crtc.regs[CRTC_REG_DISPSTARTL] | (crtc.regs[CRTC_REG_DISPSTARTH] << 8)) & 0x3fff)

#define CRTC_SCREEN_TEXTCOLS() \
    (crtc.regs[CRTC_REG_HDISP] * crtc.hw_cols)
#define CRTC_SCREEN_TEXTLINES() \
    (crtc.regs[CRTC_REG_VDISP] & 0x7f)


#define CRTC_SCREEN_YPIX() \
    (CRTC_SCREEN_TEXTLINES() * (crtc_min(16, crtc.regs[CRTC_REG_SCANLINE] + 1)))


#define CRTC_FIRST_DISPLAYED_LINE \
    CRTC_SCREEN_BORDERHEIGHT
#define CRTC_LAST_DISPLAYED_LINE \
    (crtc.screen_height - CRTC_SCREEN_BORDERHEIGHT - 1)

#define CRTC_CYCLES_PER_LINE() \
    crtc.regs[CRTC_REG_HTOTAL]


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

    crtc.rl_visible = crtc.regs[CRTC_REG_HDISP];
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
    return VIDEO_CRT_TYPE_MONO;
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

void crtc_set_screen_addr(uint8_t *screen)
{
    crtc.screen_base = screen;
}

void crtc_set_chargen_offset(int offset)
{
    /* printf("crtc_set_chargen_offset(offset:%d)\n",offset); */
    crtc.chargen_offset = offset << 4; /* times the number of bytes/char */

    crtc_update_chargen_rel();
}

void crtc_set_chargen_addr(uint8_t *chargen, int cmask)
{
    /* printf("crtc_set_chargen_addr(mask:0x%02x)\n",cmask); */
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
    /* printf("crtc_set_hw_options(hwflag:%02x vmask:%02x vchar:%02x vcoffset:%02x vrevmask:%02x)\n",
           hwflag, vmask, vchar, vcoffset, vrevmask); */
    crtc.hw_cursor = hwflag & CRTC_HW_CURSOR;
    crtc.hw_cols = (hwflag & CRTC_HW_DOUBLE_CHARS) ? 2 : 1;
    crtc.beam_offset = (hwflag & CRTC_HW_LATE_BEAM) ? crtc.hw_cols : 0;
    crtc.vaddr_mask = vmask;
    crtc.vaddr_mask_eff = (hwflag & CRTC_HW_DOUBLE_CHARS) ? (vmask << 1) | 1
                                                          : vmask;
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

raster_t *crtc_init(void)
{
    raster_t *raster;

    DBG(("crtc_init"));

    crtc.log = log_open("CRTC");

    crtc.raster_draw_alarm = alarm_new(maincpu_alarm_context, "CrtcRasterDraw",
                                       crtc_raster_draw_alarm_handler, NULL);
#if CRTC_BEAM_RACING
    crtc.adjusted_retrace_alarm = alarm_new(maincpu_alarm_context, "CrtcRetrace",
                                            crtc_adjusted_retrace_alarm_handler, NULL);
#endif

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

    /* power-up defaults */
    /* FIXME: this is not only incomplete, but also probably not quite what
              really happens */
    if (!crtc.regs[CRTC_REG_HTOTAL]) {
        crtc.regs[CRTC_REG_HTOTAL] = 49;
    }
    if (!crtc.regs[CRTC_REG_HDISP]) {
        crtc.regs[CRTC_REG_HDISP] = 40;
    }
    if (!crtc.regs[CRTC_REG_HSYNC]) {
        crtc.regs[CRTC_REG_HSYNC] = 45;
    }
    if (!crtc.regs[CRTC_REG_VSYNC]) {
        crtc.regs[CRTC_REG_VSYNC] = 28;
    }
    if (!crtc.regs[CRTC_REG_VTOTAL]) {
        crtc.regs[CRTC_REG_VTOTAL] = 30;
    }
    if (!crtc.regs[CRTC_REG_VDISP]) {
        crtc.regs[CRTC_REG_VDISP] = 25;
    }
    if (!crtc.regs[CRTC_REG_SCANLINE]) {
        crtc.regs[CRTC_REG_SCANLINE] = 7;
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

    crtc.rl_visible = crtc.regs[CRTC_REG_HDISP];
    crtc.rl_sync = crtc.regs[CRTC_REG_HSYNC];
    crtc.rl_len = crtc.regs[CRTC_REG_HTOTAL];
    crtc.prev_rl_visible = crtc.rl_visible;
    crtc.prev_rl_sync = crtc.rl_sync;
    crtc.prev_rl_len = crtc.rl_len;

    crtc.rl_start = maincpu_clk + 1;
    crtc.frame_start = maincpu_clk + 1;

    crtc_reset_screen_ptr();

    crtc.raster.ycounter = 0;           /* scan line within a text line (0..7) */
    crtc.current_charline = 0;
    crtc.current_line = 0;              /* scan line */
    /* expected number of rasterlines for next frame */
    crtc.framelines = (crtc.regs[CRTC_REG_VTOTAL] + 1) * (crtc.regs[CRTC_REG_SCANLINE] + 1)
                      + crtc.regs[CRTC_REG_VTOTALADJ];
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
    DBG(("rl_len(HTOTAL,R0)=%d, rl_visible(HDISP,R1)=%d, rl_sync(HSYNC,R2)=%d\n",
            crtc.prev_rl_len, crtc.prev_rl_visible, crtc.prev_rl_sync));
    DBG(("new_sync_diff=%d, rasterline=%d\n", new_sync_diff, crtc.current_line));

    /* Compute the horizontal position.
     * the original PET displays have quite a variety of sync timings
     * (or I haven't found the scheme yet). Therefore we cannot simply
     * center the part between the syncs. We assume the sync in the
     * first rasterline of the screen to be the default for the next
     * frame.
     *
     * For now we simply center the displayed characters (HDISP).
     *
     * Another strategy is to assume that the default HSYNC position is
     * 50, and any differences from that shift the line.
     * This fails for the 50 and 60 Hz 4032 (use 41).
     * This works for "cbm4032 any hz" (lowers HSYNC to 40 to shift the
     * line 10*2 chars to the right) but not "cbm4032v2.1 50hz" (uses
     * 31).
     *
     * There was a different, complicated calculation here but it
     * didn't give a realistic result:
     * ((screen_width - (crtc.sync_diff * 8 * crtc.hw_cols)) / 2)
     * + ((crtc.prev_rl_len + 1
     *     - crtc.prev_rl_sync
     *     - ((crtc.regs[CRTC_REG_SYNCWIDTH] & 0x0f) / 2)
     *    ) * 8 * crtc.hw_cols);
     */

    if (crtc.raster.current_line == 0) {
        int width_chars = crtc.prev_rl_visible;
        int width_pixels = width_chars * 8 * crtc.hw_cols;
        crtc.screen_xoffset = (screen_width - width_pixels) / 2;
        crtc.screen_hsync = crtc.rl_sync;
    }

    /* Increasing the HSYNC position moves the image left */
    crtc.xoffset = crtc.screen_xoffset +
                   (crtc.screen_hsync - crtc.rl_sync) * 8 * crtc.hw_cols;

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
                ((screen_width - (new_sync_diff * crtc.hw_cols * 8)) / 2),
                (crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                - ((crtc.regs[CRTC_REG_SYNCWIDTH] & 0x0f) / 2)),
                ((screen_width - (new_sync_diff * crtc.hw_cols * 8)) / 2)
                + ((crtc.prev_rl_len + 1 - crtc.prev_rl_sync
                - ((crtc.regs[CRTC_REG_SYNCWIDTH] & 0x0f) / 2)) * crtc.hw_cols * 8),
                crtc.hjitter);
    }
*/

    crtc.prev_rl_visible = crtc.rl_visible;
    crtc.prev_rl_sync = crtc.rl_sync;
    crtc.prev_rl_len = crtc.rl_len;
    crtc.prev_screen_rel = crtc.screen_rel;

    crtc.rl_visible = crtc.regs[CRTC_REG_HDISP];
    crtc.rl_sync = crtc.regs[CRTC_REG_HSYNC];
    crtc.rl_len = crtc.regs[CRTC_REG_HTOTAL];

    /*
     * Alarm handlers for cycle N run after cpu accesses for cycle N.
     * viacore.c contains code to confirm this, conditioned on
     * CHECK_CPU_VS_ALARM_CLOCKS.
     * So the start of the next line counts as being the next cycle: + 1.
     */
    crtc.rl_start = maincpu_clk - offset + 1;

    /******************************************************************
     * handle the rasterline numbering
     */

    crtc.current_line++;        /* Set it to the next line's number */
    vsync_do_end_of_line();

    /* Did we create as many scan lines as the last time? */
    if ((crtc.framelines - crtc.current_line) == crtc.screen_yoffset) {
        crtc.raster.current_line = 0;
        raster_canvas_handle_end_of_frame(&crtc.raster);
        vsync_do_vsync(crtc.raster.canvas);
    }

    {
        /* FIXME: charheight */
        /*
         * The screen starts at the first scan line of the first char of the
         * first character line.  In each scan line, the text part is followed
         * by right border, horizontal sync/retrace, left border.
         * The text lines are followed by a bottom border, vertical retrace
         * (which includes vertical sync), and top border. This total number of
         * scan lines is expressed in VTOTAL text lines + VTOTALADJ scan lines.
         *
         * Are we past the end of the screen, i.e. the top border?
         */
        if (crtc.current_charline >= crtc.regs[CRTC_REG_VTOTAL] + 1) {
#if CRTC_BEAM_RACING
            if ((crtc.retrace_type & CRTC_RETRACE_TYPE_CRTC) == 0 && /* no CRTC */
                crtc.current_line == 32*8 + 4 - 1) {
                /* Set the retrace/vertical blank alarm, to end the IRQ,
                 * at the rhs of the visible text area but 1 line above it.
                 * Non-crtc timings are fixed so we might as well use the
                 * more efficient expression to check for the top line. */
                alarm_set(crtc.adjusted_retrace_alarm,
                          crtc.rl_start + crtc.rl_visible);
            }
#endif
            /* The real end is VTOTALADJ scan lines futher down, for fine tuning */
            if ((crtc.raster.ycounter + 1) >= crtc.regs[CRTC_REG_VTOTALADJ]) {
                long cycles;

                /* Do vsync stuff. Reset line counters to top (0). */
                /* printf("new screen at clk=%d\n",crtc.rl_start); */
                crtc_reset_screen_ptr();
                crtc.raster.ycounter = 0;
                crtc.current_charline = 0;
                new_venable = 1;        /* Re-enable video */

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
                crtc.raster.ycounter &= 0x1f;
            }
        } else {
            /* Are we NOT at the bottom most scan line of a character,
             * i.e still inside it? */
            if (crtc.raster.ycounter != crtc.regs[CRTC_REG_SCANLINE]) {
#if CRTC_BEAM_RACING
                if ((crtc.retrace_type & CRTC_RETRACE_TYPE_CRTC) == 0 && /* no CRTC */
                    /* crtc.current_charline + 1 == crtc.regs[CRTC_REG_VDISP] &&
                    crtc.raster.ycounter + 1 == crtc.regs[CRTC_REG_SCANLINE] */
                    crtc.current_line == 25*8 - 1) {
                    /* Set the retrace/vertical blank alarm, to cause an IRQ,
                     * at the end/rhs of the visible text area.
                     * Non-crtc timings are fixed so we might as well use the
                     * more efficient expression to check for the bottom line. */
                    alarm_set(crtc.adjusted_retrace_alarm,
                              crtc.rl_start + crtc.rl_visible);
                }
#endif
                crtc.raster.ycounter++;
                crtc.raster.ycounter &= 0x1f;
            } else {
                /* Start a new character line */
                crtc.raster.ycounter = 0;
                crtc.cursor_lines = 0;
                crtc.current_charline++;
                crtc.current_charline &= 0x7f;

                if (crtc.henable) {
                    crtc.screen_rel += crtc.rl_visible * crtc.hw_cols;
                }
                /* Are we past the text area? */
                if (crtc.current_charline == crtc.regs[CRTC_REG_VDISP]) {
                    new_venable = 0;            /* disable video */
                }
                /* Should the vertical sync signal start? */
                if (crtc.current_charline == crtc.regs[CRTC_REG_VSYNC]) {
                    /* printf("vsync starts at clk=%d\n",crtc.rl_start); */
                    new_vsync = (crtc.regs[CRTC_REG_SYNCWIDTH] >> 4) & 0x0f;
                    if (!new_vsync) {
                        new_vsync = 16;
                    }
                    new_vsync++;  /* compensate for the first decrease below */
                }
            }
            /* Enable or disable the cursor, if it is in the next character line */
            if (crtc.raster.ycounter == (unsigned int)(crtc.regs[CRTC_REG_CURSORSTART] & 0x1f)) {
                crtc.cursor_lines = 1;
            } else if (crtc.raster.ycounter == (unsigned int)((crtc.regs[CRTC_REG_CURSOREND] + 1) & 0x1f)) {
                crtc.cursor_lines = 0;
            }

            crtc.henable = 1;
        }
        /* If we're in the vertical sync area, count down how many lines are left. */
        if (new_vsync) {
            new_vsync--;
        }
#if CRTC_BEAM_RACING
        if (new_venable) {
            crtc_fetch_prefetch();
        }
#endif /* CRTC_BEAM_RACING */
    }

    /******************************************************************
     * signal retrace to CPU
     */

    if (crtc.retrace_type & CRTC_RETRACE_TYPE_CRTC) {
        if ((bool)crtc.vsync != (bool)new_vsync) {
            crtc.off_screen = new_vsync != 0;
            if (crtc.retrace_callback) {
                crtc.retrace_callback(crtc.off_screen, offset);
            }
        }
    } else {        /* PETs without CRTC */
#if CRTC_BEAM_RACING == 0
        if (crtc.venable != new_venable) {
            crtc.off_screen = !new_venable;
            if (crtc.retrace_callback) {
                crtc.retrace_callback(crtc.off_screen, offset);
            }
        }
#endif /* CRTC_BEAM_RACING */
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
        /* crtc.regs[CRTC_REG_VDISP] * (crtc.regs[CRTC_REG_SCANLINE] + 1); */

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
     * Set up new alarm.
     * Note that rl_start was set to maincpu_clk + 1 above, so the total
     * time between scanline alarms is properly rl_len + 1 cycles.
     */

    alarm_set(crtc.raster_draw_alarm, crtc.rl_start + crtc.rl_len);
}

#if CRTC_BEAM_RACING

inline void crtc_fetch_prefetch(void)
{
    const int length = crtc.rl_visible * crtc.hw_cols;
    const int start = crtc.screen_rel & crtc.vaddr_mask_eff;
    const int end = start + length;
    const int limit = crtc.vaddr_mask_eff + 1;

    if (end <= limit) {
        /* The usual case */
        memcpy(&crtc.prefetch[0],
               &crtc.screen_base[start],
               length);
    } else {
        /* Handle address wraparound */
        const int length1 = limit - start;
        const int length2 = end - limit;
        /* const int length1 = length - length2; */

        memcpy(&crtc.prefetch[0],
               &crtc.screen_base[start],
               length1);
        memcpy(&crtc.prefetch[length1],
               &crtc.screen_base[0],
               length2);
    }
}

/*
 * Handle the beginning and end of the retrace period on non-CRTC hardware.
 * It starts just after the last text position and ends exactly 3*20 scan line
 * times later (in the same horizontal position).
 */
static void crtc_adjusted_retrace_alarm_handler(CLOCK offset, void *data)
{
    alarm_unset(crtc.adjusted_retrace_alarm);
    /*
     * Set off_screen before the draw alarm would set it (too late).
     * Since we change off_screen before venable is changed, and normally
     * off_screen = !venable, we must omit the negation.
     */
    crtc.off_screen = crtc.venable;
    crtc.retrace_callback(crtc.off_screen, offset);
}

/*
 * Experimental approximation of snow.
 * Real snow would be of lower intensity than normal pixels because it is
 * typically only displayed for one frame.
 * Also, read access to the screen memory should probably cause it too.
 */
#define SNOW            0

/*
 * The caller must mask the addr to an acceptable range for the screen size.
 * This is needed in the caller because there it is known which addresses
 * are mirrors for the true screen memory.
 */
void crtc_update_prefetch(uint16_t addr, uint8_t value)
{
    if (addr >= crtc.screen_rel) {
        int xpos = addr - crtc.screen_rel;
        int width =  crtc.rl_visible * crtc.hw_cols;

        if (xpos < width) {
            /*
             * Which memory location is currently being fetched for display?
             * 40 cols: 1 character takes 1 clock cycle.
             * 80 cols: 2 characters take 1 clock cycle.
             * Expected values: 0...<frame duration at most>.
             *
             * We correct the cpu clock for the CRTC_STORE_OFFSET, but this
             * happens to be cancelled out with the 1 cycle delay caused by the
             * character ROM lookup.
             */
            int beampos = (int)(maincpu_clk - CRTC_STORE_OFFSET + 1 - crtc.rl_start) *
                               crtc.hw_cols
                          - crtc.beam_offset;
            /*
             * For some as yet unexplained reason, on a tested 8032 (compared
             * to 4032) you can store a screen value 1 cycle later and it will
             * still be displayed instead of the old value. See bug #1954.
             */

            if (xpos >= beampos) {
                /* Character is still to be displayed in the current scan line */
                crtc.prefetch[xpos] = value;    /* xpos < width < 2*256 */
                DBG(("updated prefetch (%d >= %d)\n", xpos, beampos));
            } else {
                DBG(("just missed updating prefetch (%d < %d)\n", xpos, beampos));
            }
        }
    }
#if SNOW
    /* snow ... */
    int beampos = (int)(maincpu_clk - CRTC_STORE_OFFSET + 1 - crtc.rl_start) *
                       crtc.hw_cols
                  - crtc.beam_offset;
    int width =  crtc.rl_visible * crtc.hw_cols;

    if (beampos >= 0 && beampos < width) {
        crtc.prefetch[beampos] = value;
    }
#endif /* SNOW */
}
#endif /* CRTC_BEAM_RACING */

void crtc_shutdown(void)
{
    raster_shutdown(&crtc.raster);
}

/* ------------------------------------------------------------------- */

int crtc_offscreen(void)
{
    /*
     * We currently shouldn't need to run pending alarms here, since this is
     * called from viacore.c which does that already in viacore_read() for
     * VIA_PRB.  For PETs (the only users) that's good enough.
     */
    return crtc.off_screen;
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
static uint8_t cols40[1] = { 40 };
static uint8_t cols60[1] = { 60 };
static uint8_t cols80[1] = { 80 };

static uint8_t charh8[1] = { 8 };
static uint8_t charh14[1] = { 14 };

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

int crtc_dump(void)
{
    uint8_t *regs = crtc.regs;
    int vsyncw,scanlines;
    int htotal, vtotal;
    double v;
    unsigned int r, c, regnum=0;

    /* Dump the internal CRTC registers */
    mon_out("CRTC Internal Registers:\n");
    for (r = 0; r < 2; r++) {
        mon_out("%02x: ", regnum);
        for (c = 0; c < 16; c++) {
            if (regnum <= 17) {
                mon_out("%02x ", regs[regnum]);
            }
            regnum++;
            if ((c & 3) == 3) {
                mon_out(" ");
            }
        }
        mon_out("\n");
    }
    htotal = regs[CRTC_REG_HTOTAL] + 1;
    vsyncw = ((regs[CRTC_REG_SYNCWIDTH] >> 4) & 0x0f);
    if (vsyncw == 0) vsyncw = 16;
    vtotal = regs[CRTC_REG_VTOTAL] + 1;
    scanlines = regs[CRTC_REG_SCANLINE] + 1;
    mon_out("HW cursor: %d blank: %d chars per cycle: %d\n\n",
            crtc.hw_cursor, crtc.hw_blank, crtc.hw_cols);
    mon_out("Horizontal total:         %3d chars.\n", htotal);
    mon_out("Horizontal sync position: %3d chars.\n", regs[CRTC_REG_HSYNC]);
    mon_out("Horizontal sync width:    %3d chars.\n", regs[CRTC_REG_SYNCWIDTH] & 0x0f);
    mon_out("Vertical total:           %3d chars +%3d lines.\n",
           vtotal, regs[CRTC_REG_VTOTALADJ]);
    mon_out("Vertical sync position:   %3d chars.\n", regs[CRTC_REG_VSYNC]);
    mon_out("Vertical sync width:      %3d lines.\n", vsyncw);
    mon_out("Display characters:       %3d x %2d\n", regs[CRTC_REG_HDISP], regs[CRTC_REG_VDISP]);
    mon_out("Scanlines per char row:    %d\n", scanlines);
    mon_out("Cursor blink mode:         ");
    switch ((regs[CRTC_REG_CURSORSTART] >> 5) & 3) {
        case 0: mon_out("display continuously\n"); break;
        case 1: mon_out("blank continuously\n"); break;
        case 2: mon_out("blink 1/16\n"); break;
        case 3: mon_out("blink 1/32\n"); break;
    }
    mon_out("Cursor start in line:     %2d, end in line: %d\n\n",
            regs[CRTC_REG_CURSORSTART] & 0x1f, regs[CRTC_REG_CURSOREND] & 0x1f);
    mon_out("Display mode control: $%02x\n"
            " interlaced: ",
            regs[CRTC_REG_MODECTRL]);
    switch (regs[CRTC_REG_MODECTRL] & 3) {
        case 1: mon_out("interlace sync"); break;
        case 3: mon_out("interlace sync & video"); break;
        default: mon_out("off");
    }
    mon_out("\n RAM addressing: %s\n"
            " display enable skew: %s, cursor skew: %s\n",
            (regs[CRTC_REG_MODECTRL] & 4) ? "row/column" : "binary",
            (regs[CRTC_REG_MODECTRL] & 16) ? "delay one character" : "no",
            (regs[CRTC_REG_MODECTRL] & 32) ? "delay one character" : "no");
    mon_out("\nEffective size of display: %d x %d (%d x %d characters)\n",
            regs[CRTC_REG_HDISP] * 8,
            regs[CRTC_REG_VDISP] * scanlines,
            regs[CRTC_REG_HDISP],
            regs[CRTC_REG_VDISP]);
    mon_out(" including overscan:       %d x %d (%d x %d characters)\n",
            (htotal * 8),
            crtc.framelines,
            htotal,
            vtotal);
    mon_out(" cycles:                   %d x %d = %d\n",
            htotal,
            crtc.framelines,
            htotal * crtc.framelines);
    v = (double)machine_get_cycles_per_second() /
                (htotal * crtc.framelines);
    mon_out(" timing:                   %d Hz horizontal, %d.%04d Hz vertical\n",
            (int)(machine_get_cycles_per_second() / htotal),
            (int)v, (int)(10000 * (v - (int)v))
           );
    if ((regs[CRTC_REG_MODECTRL] & 4) == 0) {
        /* binary mode */
        mon_out("\nMode is: binary\n");
        mon_out("Display start:     $%04x\n",
                (unsigned int)((regs[CRTC_REG_DISPSTARTH] * 256)
                    + regs[CRTC_REG_DISPSTARTL]));
        mon_out("Cursor position:   $%04x\n",
                (unsigned int)((regs[CRTC_REG_CURSORPOSH] * 256)
                    + regs[CRTC_REG_CURSORPOSL]));
        mon_out("Lightpen position: $%04x\n",
                (unsigned int)((regs[CRTC_REG_LPENH] * 256)
                    + regs[CRTC_REG_LPENL]));
    } else {
        /* row/column mode */
        mon_out("\nMode is: row/column\n");
        mon_out("Display start:     %3d x %3d\n",
                regs[CRTC_REG_DISPSTARTL], regs[CRTC_REG_DISPSTARTH]);
        mon_out("Cursor position:   %3d x %3d\n",
                regs[CRTC_REG_CURSORPOSL], regs[CRTC_REG_CURSORPOSH]);
        mon_out("Lightpen position: %3d x %3d\n",
                regs[CRTC_REG_LPENL], regs[CRTC_REG_LPENH]);
    }
    mon_out("\nBeam position (to draw next):\n"
            "charline %d, rasterline %d (ch+%d), %d lines to end of vsync\n",
            crtc.current_charline,
            crtc.current_line,
            crtc.current_line % scanlines,
            crtc.vsync);
    mon_out("CLOCK at start of frame %"PRIu64", + rasterline %"PRIu64", line length %d\n",
            crtc.frame_start,
            crtc.rl_start - crtc.frame_start,
            crtc.rl_len);

    if (crtc.raster_draw_alarm) {
        CLOCK then = crtc.raster_draw_alarm->context->next_pending_alarm_clk;
        mon_out("next raster line draw alarm: %"PRIu64" (now+%"PRIu64")\n",
                then, then - maincpu_clk);
    }
    return 0;
}
