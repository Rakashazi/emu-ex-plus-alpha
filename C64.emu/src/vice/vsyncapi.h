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
extern signed long vsyncarch_frequency(void);

/* provide the actual time in timer units */
extern unsigned long vsyncarch_gettime(void);

/* call when vsync_init is called */
extern void vsyncarch_init(void);

/* display speed(%) and framerate(fps) */
extern void vsyncarch_display_speed(double speed, double fps, int warp_enabled);

/* sleep the given amount of timer units */
extern void vsyncarch_sleep(signed long delay);

#if defined (HAVE_OPENGL_SYNC) && !defined(USE_SDLUI)
/* synchronize with vertical blanks */
extern void vsyncarch_verticalblank(struct video_canvas_s *c, float rate,
                                    int frames);

/* keep vertical blank sync prepared */
extern void vsyncarch_prepare_vbl(void);
#endif

/* this is called before vsync_do_vsync does the synchroniation */
extern void vsyncarch_presync(void);

/* this is called after vsync_do_vsync did the synchroniation */
extern void vsyncarch_postsync(void);

/* set ui dispatcher function */
extern void_hook_t vsync_set_event_dispatcher(void_hook_t hook);

extern int vsyncarch_vbl_sync_enabled(void);

#if defined (HAVE_OPENGL_SYNC) && !defined(USE_SDLUI)
/* wait for next vertical retrace */
extern void vsyncarch_sync_with_raster(struct video_canvas_s *c);
#endif

#endif
