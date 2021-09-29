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

/* Manually defined */
/* To enable/disable this option by hand change the 0 below to 1. */
#if 0
#define VSYNC_DEBUG
#endif

typedef void (*vsync_callback_func_t)(void *param);
typedef struct vsync_callback {
    vsync_callback_func_t callback;
    void *param;
} vsync_callback_t;

struct video_canvas_s;

extern void vsync_suspend_speed_eval(void);
extern void vsync_reset_hook(void);
extern int vsync_resources_init(void);
extern int vsync_cmdline_options_init(void);
extern void vsync_init(void (*hook)(void));
extern void vsync_set_machine_parameter(double refresh_rate, long cycles);
extern double vsync_get_refresh_frequency(void);
extern void vsync_do_end_of_line(void);
extern int vsync_do_vsync(struct video_canvas_s *c, int been_skipped);
extern int vsync_disable_timer(void);
extern void vsync_on_vsync_do(vsync_callback_func_t callback_func, void *callback_param);

#endif
