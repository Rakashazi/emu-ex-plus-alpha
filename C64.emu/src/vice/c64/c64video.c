/*
 * c64video.c - Machine specific video handling.
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

#include "vice.h"

#include <stdio.h>

#include "machine-video.h"
#include "vicii.h"
#include "video.h"


void machine_video_init(void)
{
    video_render_2x2_init();
    video_render_pal_init();
}

int machine_video_resources_init(void)
{
    if (video_resources_init() < 0) {
        return -1;
    }

    return 0;
}

struct video_canvas_s *machine_video_canvas_get(unsigned int window)
{
    if (window == 0) {
        return vicii_get_canvas();
    }

    return NULL;
}
