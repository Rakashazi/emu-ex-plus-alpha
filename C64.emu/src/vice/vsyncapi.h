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

/* current performance metrics */
void vsyncarch_get_metrics(double *cpu_percent, double *emulated_fps, int *warp_enabled);

/* this is called before vsync_do_vsync does the synchroniation */
void vsyncarch_presync(void);

/* this is called after vsync_do_vsync did the synchroniation */
void vsyncarch_postsync(void);

/* called to advance the emulation by one frame */
void vsyncarch_advance_frame(void);

int vsyncarch_vbl_sync_enabled(void);

void VICE_API vsyncarch_refresh_frequency_changed(double rate);

#endif
