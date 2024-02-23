/*
 * crtc.h - A CRTC emulation (under construction)
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

#ifndef VICE_CRTC_H
#define VICE_CRTC_H

#include "vice.h"

#include "types.h"

typedef void (*machine_crtc_retrace_signal_t)(unsigned int, CLOCK offset);
typedef void (*crtc_hires_draw_t)(uint8_t *p, int xstart, int xend, int scr_rel, int ymod8);

struct snapshot_s;
struct screenshot_s;
struct canvas_refresh_s;

struct raster_s *crtc_init(void);
struct video_canvas_s *crtc_get_canvas(void);
void crtc_reset(void);

int crtc_resources_init(void);
int crtc_cmdline_options_init(void);

int crtc_snapshot_write_module(struct snapshot_s *s);
int crtc_snapshot_read_module(struct snapshot_s *s);

void crtc_set_screen_addr(uint8_t *screen);
void crtc_set_chargen_offset(int offset);
void crtc_set_chargen_addr(uint8_t *chargen, int cmask);
void crtc_set_screen_options(int num_cols, int rasterlines);
void crtc_set_hw_options(int hwflag, int vmask, int vchar, int vcoffset, int vrevmask);
#define CRTC_HW_CURSOR          1       /* do we have a hardware cursor */
#define CRTC_HW_DOUBLE_CHARS    2       /* 2 chars per cycle */
#define CRTC_HW_LATE_BEAM       4       /* 1 cycle more time to catch the beam */
void crtc_set_retrace_callback(machine_crtc_retrace_signal_t callback);
void crtc_set_retrace_type(int type);
#define CRTC_RETRACE_TYPE_DISCRETE      0
#define CRTC_RETRACE_TYPE_CRTC          1
void crtc_set_hires_draw_callback(crtc_hires_draw_t);
void crtc_fetch_prefetch(void);
void crtc_enable_hw_screen_blank(int enable);
void crtc_screenshot(struct screenshot_s *screenshot);
void crtc_async_refresh(struct canvas_refresh_s *refresh);
void crtc_shutdown(void);

void crtc_screen_enable(int);

int crtc_offscreen(void);

void crtc_update_window(void);
void crtc_update_renderer(void);

uint8_t *crtc_get_active_bitmap(void);

/*
 * If CRTC_BEAM_RACING is true, then some racing-the-beam code is
 * enabled.
 * Since rendering of scanlines happens all-at-once at the end of the line
 * (a.k.a. the start of the next line), normally even memory stores that are in
 * the current text line but "behind the beam" would still have effect.
 * When set, we prefetch the text line at the start of each scan line. In the
 * video memory store functions, we update the cache if the store is ahead of
 * the beam. Stores that are "too late" will then not be displayed in this line.
 *
 * Additionally, the timing of the "off_screen" signal for non-CRTC is
 * corrected, at the cost of extra alarms.
 */
#define CRTC_BEAM_RACING        1
#if CRTC_BEAM_RACING
void crtc_update_prefetch(uint16_t addr, uint8_t value);
#endif

/*
 * The CRTC is only used on machines with the non-SC core, so CPU stores
 * happen with the clock already incremented. To get the real clock value,
 * subtract 1.
 */
#define CRTC_STORE_OFFSET       1

#endif
