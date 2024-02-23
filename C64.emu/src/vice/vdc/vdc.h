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

#define VDC_REVISION_0  0 /* 8563 R7A */
#define VDC_REVISION_1  1 /* 8563 R8/R9 */
#define VDC_REVISION_2  2 /* 8568 */

#define VDC_NUM_REVISIONS 3

#define VDC_16KB   0
#define VDC_64KB   1

struct snapshot_s;
struct screenshot_s;
struct canvas_refresh_s;

int vdc_resources_init(void);
int vdc_cmdline_options_init(void);
struct raster_s *vdc_init(void);
struct video_canvas_s *vdc_get_canvas(void);

void vdc_reset(void);
void vdc_trigger_light_pen(CLOCK mclk);
CLOCK vdc_lightpen_timing(int x, int y);
void vdc_prepare_for_snapshot(void);
void vdc_powerup(void);
void vdc_resize(void);
void vdc_screenshot(struct screenshot_s *screenshot);
void vdc_async_refresh(struct canvas_refresh_s *r);

int vdc_write_snapshot_module(struct snapshot_s *s);
int vdc_read_snapshot_module(struct snapshot_s *s);

void vdc_set_canvas_refresh(int enable);
void vdc_shutdown(void);

#endif
