/*
 * ted.h
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

#ifndef VICE_TED_H
#define VICE_TED_H

#include "types.h"

/* TED border mode defines */
#define TED_NORMAL_BORDERS 0
#define TED_FULL_BORDERS   1
#define TED_DEBUG_BORDERS  2
#define TED_NO_BORDERS     3

struct canvas_refresh_s;
struct machine_timing_s;
struct snapshot_s;
struct screenshot_s;

extern CLOCK last_write_cycle;
extern CLOCK first_write_cycle;

int ted_resources_init(void);
int ted_cmdline_options_init(void);
struct raster_s *ted_init(void);
struct video_canvas_s *ted_get_canvas(void);

void ted_reset(void);
void ted_powerup(void);
void ted_reset_registers(void);
void ted_handle_pending_alarms(CLOCK num_write_cycles);
void ted_screenshot(struct screenshot_s *screenshot);
void ted_async_refresh(struct canvas_refresh_s *r);
void ted_shutdown(void);
void ted_change_timing(struct machine_timing_s *machine_timing, int bordermode);

void ted_snapshot_prepare(void);
int ted_snapshot_write_module(struct snapshot_s *s);
int ted_snapshot_read_module(struct snapshot_s *s);

int ted_dump(void);

#endif
