/*
 * crtc-resources.c - A line-based CRTC emulation (under construction).
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

/* #define DEBUG_CRTC */

#ifdef DEBUG_CRTC
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "crtc-resources.h"
#include "crtctypes.h"
#include "fullscreen.h"
#include "log.h"
#include "raster-resources.h"
#include "resources.h"
#include "video.h"


static video_chip_cap_t video_chip_cap;

static int crtc_stretchy;

void crtc_update_renderer(void)
{
    DBG(("crtc_update_renderer crtc.hw_cols: %d", crtc.screen_width));

    if ((crtc_stretchy) && (crtc.screen_width > ((384 + 704) / 2))) {
        /* 80 columns */
        crtc.video_chip_cap->single_mode.sizex = 1;
        crtc.video_chip_cap->single_mode.sizey = 2;
        crtc.video_chip_cap->single_mode.rmode = VIDEO_RENDER_CRT_1X2;
        crtc.video_chip_cap->double_mode.sizex = 2;
        crtc.video_chip_cap->double_mode.sizey = 4;
        crtc.video_chip_cap->double_mode.rmode = VIDEO_RENDER_CRT_2X4;
        crtc.video_chip_cap->scale2x_allowed = 0;
    } else {
        /* 40 columns */
        crtc.video_chip_cap->single_mode.sizex = 1;
        crtc.video_chip_cap->single_mode.sizey = 1;
        crtc.video_chip_cap->single_mode.rmode = VIDEO_RENDER_CRT_1X1;
        crtc.video_chip_cap->double_mode.sizex = 2;
        crtc.video_chip_cap->double_mode.sizey = 2;
        crtc.video_chip_cap->double_mode.rmode = VIDEO_RENDER_CRT_2X2;
        crtc.video_chip_cap->scale2x_allowed = ARCHDEP_CRTC_DSIZE;
    }
}

static int set_stretch(int val, void *param)
{
    DBG(("set_stretch"));
    crtc_stretchy = val ? 1 : 0;
    crtc_update_renderer();
    resources_touch("CrtcDoubleSize");
    return 0;
}

static const resource_int_t resources_int[] =
{
    { "CrtcStretchVertical", 1, RES_EVENT_SAME, NULL,
      &crtc_stretchy, set_stretch, NULL },
    { NULL, 0, 0, NULL,
      NULL, NULL, NULL }
};

int crtc_resources_init(void)
{
    video_chip_cap.dsize_allowed = ARCHDEP_CRTC_DSIZE;
    video_chip_cap.dsize_default = 0;
    video_chip_cap.dsize_limit_width = 800; /* 2 times the 80cols screen */
    video_chip_cap.dsize_limit_height = 700; /* 4 times the 80cols screen */
    video_chip_cap.dscan_allowed = ARCHDEP_CRTC_DSCAN;
    video_chip_cap.hwscale_allowed = ARCHDEP_CRTC_HWSCALE;
    video_chip_cap.external_palette_name = "green";
    video_chip_cap.double_buffering_allowed = ARCHDEP_CRTC_DBUF;
    fullscreen_capability(&(video_chip_cap.fullscreen));

    if (raster_resources_chip_init("Crtc", &crtc.raster, &video_chip_cap) < 0) {
        return -1;
    }
    crtc.video_chip_cap = &video_chip_cap;
    crtc_update_renderer();

    return resources_register_int(resources_int);
}
