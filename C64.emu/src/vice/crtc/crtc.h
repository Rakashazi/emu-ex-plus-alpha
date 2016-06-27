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

typedef void (*machine_crtc_retrace_signal_t)(unsigned int);
typedef void (*crtc_hires_draw_t)(BYTE *p, int xstart, int xend, int scr_rel, int ymod8);

struct snapshot_s;
struct screenshot_s;
struct canvas_refresh_s;

extern struct raster_s *crtc_init(void);
extern struct video_canvas_s *crtc_get_canvas(void);
extern void crtc_reset(void);

extern int crtc_resources_init(void);
extern int crtc_cmdline_options_init(void);

extern int crtc_snapshot_write_module(struct snapshot_s *s);
extern int crtc_snapshot_read_module(struct snapshot_s *s);

extern void crtc_set_screen_addr(BYTE *screen);
extern void crtc_set_chargen_offset(int offset);
extern void crtc_set_chargen_addr(BYTE *chargen, int cmask);
extern void crtc_set_screen_options(int num_cols, int rasterlines);
extern void crtc_set_hw_options(int hwflag, int vmask, int vchar, int vcoffset,
                                int vrevmask);
extern void crtc_set_retrace_callback(machine_crtc_retrace_signal_t callback);
extern void crtc_set_retrace_type(int type);
extern void crtc_set_hires_draw_callback(crtc_hires_draw_t);
extern void crtc_enable_hw_screen_blank(int enable);
extern void crtc_screenshot(struct screenshot_s *screenshot);
extern void crtc_async_refresh(struct canvas_refresh_s *refresh);
extern void crtc_shutdown(void);

extern void crtc_screen_enable(int);

extern int crtc_offscreen(void);

extern void crtc_update_window(void);
extern void crtc_update_renderer(void);

extern BYTE *crtc_get_active_bitmap(void);

#endif
