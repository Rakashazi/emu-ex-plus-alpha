/*
 * vsyn.h - Common vsync API.
 *
 * Written by
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

#ifndef VICE_VSYNC_H
#define VICE_VSYNC_H

struct video_canvas_s;

extern int vsync_frame_counter;

extern void vsync_suspend_speed_eval(void);
extern void vsync_sync_reset(void);
extern int vsync_resources_init(void);
extern int vsync_cmdline_options_init(void);
extern void vsync_init(void (*hook)(void));
extern void vsync_set_machine_parameter(double refresh_rate, long cycles);
extern double vsync_get_refresh_frequency(void);
extern int vsync_do_vsync(struct video_canvas_s *c, int been_skipped);
extern int vsync_disable_timer(void);

#endif
