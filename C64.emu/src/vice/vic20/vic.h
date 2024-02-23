/*
 * vic.h - A VIC-I emulation (under construction)
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_VIC_H
#define VICE_VIC_H

#include "vice.h"
#include "types.h"

/* VIC border mode defines */
#define VIC_NORMAL_BORDERS 0
#define VIC_FULL_BORDERS   1
#define VIC_DEBUG_BORDERS  2
#define VIC_NO_BORDERS     3
#define VIC_BORDER_MODE(v) (v << 12)

struct snapshot_s;
struct screenshot_s;
struct palette_s;
struct canvas_refresh_s;
struct video_chip_cap_s;

struct raster_s *vic_init(void);
struct video_canvas_s *vic_get_canvas(void);
void vic_reset(void);
void vic_raster_draw_handler(void);

int vic_resources_init(void);
int vic_cmdline_options_init(void);

int vic_snapshot_write_module(struct snapshot_s *s);
int vic_snapshot_read_module(struct snapshot_s *s);

void vic_screenshot(struct screenshot_s *screenshot);
void vic_async_refresh(struct canvas_refresh_s *refresh);
void vic_shutdown(void);

void vic_set_light_pen(CLOCK mclk, int state);
void vic_trigger_light_pen(CLOCK mclk);
CLOCK vic_lightpen_timing(int x, int y);
void vic_trigger_light_pen_internal(int retrigger);

struct machine_timing_s;

void vic_change_timing(struct machine_timing_s *machine_timing, int border_mode);

int vic_dump(void);

/* Debugging options.  */
/* #define VIC_RASTER_DEBUG */
/* #define VIC_REGISTERS_DEBUG */
/* #define VIC_CYCLE_DEBUG */

#ifdef VIC_RASTER_DEBUG
#define VIC_DEBUG_RASTER(x) log_debug x
#else
#define VIC_DEBUG_RASTER(x)
#endif

#ifdef VIC_REGISTERS_DEBUG
#define VIC_DEBUG_REGISTER(x) log_debug x
#else
#define VIC_DEBUG_REGISTER(x)
#endif

#ifdef VIC_CYCLE_DEBUG
#define VIC_DEBUG_CYCLE(x) log_debug x
#else
#define VIC_DEBUG_CYCLE(x)
#endif

#endif
