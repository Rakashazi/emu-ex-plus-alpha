/*
 * vdc.h - A first attempt at a MOS8563 (VDC) emulation.
 *
 * Written by
 *  Markus Brenner <markus@brenner.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VDC_H
#define VICE_VDC_H

#include "types.h"

struct snapshot_s;
struct screenshot_s;
struct canvas_refresh_s;

extern int vdc_resources_init(void);
extern int vdc_cmdline_options_init(void);
extern struct raster_s *vdc_init(void);
extern struct video_canvas_s *vdc_get_canvas(void);

extern void vdc_reset(void);
extern void vdc_trigger_light_pen(CLOCK mclk);
extern CLOCK vdc_lightpen_timing(int x, int y);
extern void vdc_prepare_for_snapshot(void);
extern void vdc_powerup(void);
extern void vdc_resize(void);
extern void vdc_screenshot(struct screenshot_s *screenshot);
extern void vdc_async_refresh(struct canvas_refresh_s *r);

extern int vdc_write_snapshot_module(struct snapshot_s *s);
extern int vdc_read_snapshot_module(struct snapshot_s *s);

extern void vdc_set_canvas_refresh(int enable);
extern void vdc_shutdown(void);

#endif
