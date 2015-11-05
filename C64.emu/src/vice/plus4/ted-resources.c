/*
 * ted-resources.c - Resources for the TED emulation.
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

#include "archdep.h"
#include "fullscreen.h"
#include "lib.h"
#include "machine.h"
#include "raster-resources.h"
#include "resources.h"
#include "ted-resources.h"
#include "ted.h"
#include "tedtypes.h"
#include "video.h"

ted_resources_t ted_resources = { 0 };
static video_chip_cap_t video_chip_cap;

static int set_border_mode(int val, void *param)
{
    int sync;

    switch (val) {
        case TED_NORMAL_BORDERS:
        case TED_FULL_BORDERS:
        case TED_DEBUG_BORDERS:
        case TED_NO_BORDERS:
            break;
        default:
            return -1;
    }

    if (resources_get_int("MachineVideoStandard", &sync) < 0) {
        sync = MACHINE_SYNC_PAL;
    }

    if (ted_resources.border_mode != val) {
        ted_resources.border_mode = val;
        machine_change_timing(sync ^ TED_BORDER_MODE(ted_resources.border_mode));
    }
    return 0;
}

static const resource_int_t resources_int[] =
{
    { "TEDBorderMode", TED_NORMAL_BORDERS, RES_EVENT_SAME, NULL,
      &ted_resources.border_mode,
      set_border_mode, NULL },
    { NULL }
};

int ted_resources_init(void)
{
    video_chip_cap.dsize_allowed = ARCHDEP_TED_DSIZE;
    video_chip_cap.dsize_default = ARCHDEP_TED_DSIZE;
    video_chip_cap.dsize_limit_width = 0;
    video_chip_cap.dsize_limit_height = 0;
    video_chip_cap.dscan_allowed = ARCHDEP_TED_DSCAN;
    video_chip_cap.hwscale_allowed = ARCHDEP_TED_HWSCALE;
    video_chip_cap.scale2x_allowed = ARCHDEP_TED_DSIZE;
    video_chip_cap.external_palette_name = "default";
    video_chip_cap.double_buffering_allowed = ARCHDEP_TED_DBUF;
    video_chip_cap.single_mode.sizex = 1;
    video_chip_cap.single_mode.sizey = 1;
    video_chip_cap.single_mode.rmode = VIDEO_RENDER_PAL_1X1;
    video_chip_cap.double_mode.sizex = 2;
    video_chip_cap.double_mode.sizey = 2;
    video_chip_cap.double_mode.rmode = VIDEO_RENDER_PAL_2X2;

    fullscreen_capability(&(video_chip_cap.fullscreen));

    ted.video_chip_cap = &video_chip_cap;

    if (raster_resources_chip_init("TED", &ted.raster, &video_chip_cap) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}
