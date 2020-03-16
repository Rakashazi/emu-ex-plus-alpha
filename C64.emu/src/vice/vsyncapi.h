/*
 * vsyncapi.h - general vsync archdependant functions
 *
 * Written by
 *  Thomas Bretz <tbretz@gsi.de>
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

#ifndef VSYNCAPI_H
#define VSYNCAPI_H

#include "vice.h"

struct video_canvas_s;

typedef void (*void_hook_t)(void);

/* number of timer units per second - used to calc speed and fps */
extern unsigned long vsyncarch_frequency(void);

/* provide the actual time in timer units */
extern unsigned long vsyncarch_gettime(void);

/* call when vsync_init is called */
extern void vsyncarch_init(void);

/* display speed(%) and framerate(fps) */
extern void vsyncarch_display_speed(double speed, double fps, int warp_enabled);

/* sleep the given amount of timer units */
extern void vsyncarch_sleep(unsigned long delay);

/* this is called before vsync_do_vsync does the synchroniation */
extern void vsyncarch_presync(void);

/* this is called after vsync_do_vsync did the synchroniation */
extern void vsyncarch_postsync(void);

/* called to advance the emulation by one frame */
extern void vsyncarch_advance_frame(void);

/* set ui dispatcher function */
extern void_hook_t vsync_set_event_dispatcher(void_hook_t hook);

extern int vsyncarch_vbl_sync_enabled(void);

void VICE_API vsyncarch_refresh_frequency_changed(double rate);

#endif
