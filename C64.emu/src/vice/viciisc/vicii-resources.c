/*
 * vicii-resources.c - Resources for the MOS 6569 (VIC-II) emulation.
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

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "fullscreen.h"
#include "machine.h"
#include "raster-resources.h"
#include "resources.h"
#include "vicii-chip-model.h"
#include "vicii-color.h"
#include "vicii-resources.h"
#include "vicii-timing.h"
#include "vicii.h"
#include "viciitypes.h"
#include "video.h"


vicii_resources_t vicii_resources;
static video_chip_cap_t video_chip_cap;


static int set_border_mode(int val, void *param)
{
    if (vicii_resources.border_mode != val) {
        vicii_resources.border_mode = val;
        /* this works because vicii-timing.c only handles borders in
           viciisc. */
        vicii_change_timing(0, vicii_resources.border_mode);
    }
    return 0;
}

static int set_sprite_sprite_collisions_enabled(int val, void *param)
{
    vicii_resources.sprite_sprite_collisions_enabled = val;
    return 0;
}

static int set_sprite_background_collisions_enabled(int val, void *param)
{
    vicii_resources.sprite_background_collisions_enabled = val;
    return 0;
}

static int set_vsp_bug_enabled(int val, void *param)
{
    vicii_resources.vsp_bug_enabled = val;
    return 0;
}

static int set_new_luminances(int val, void *param)
{
    vicii_resources.new_luminances = val;
    return vicii_color_update_palette(vicii.raster.canvas);
}

struct vicii_model_info_s {
    int video;
    int luma;
};

static struct vicii_model_info_s vicii_info[] = {
    /* VICII_MODEL_6569: PAL, 63 cycle, 9 luma, "old" */
    { MACHINE_SYNC_PAL, 1 },

    /* VICII_MODEL_8565: PAL, 63 cycle, 9 luma, "new" */
    { MACHINE_SYNC_PAL, 1 },

    /* VICII_MODEL_6569R1: PAL, 63 cycle, 5 luma, "old" */
    { MACHINE_SYNC_PAL, 0 },

    /* VICII_MODEL_6567: NTSC, 65 cycle, 9 luma, "old" */
    { MACHINE_SYNC_NTSC, 1 },

    /* VICII_MODEL_8562: NTSC, 65 cycle, 9 luma, "new" */
    { MACHINE_SYNC_NTSC, 1 },

    /* VICII_MODEL_6567R56A: NTSC, 64 cycle, 5? luma, "old" */
    { MACHINE_SYNC_NTSCOLD, 0 },

    /* VICII_MODEL_6572: PAL-N, 65 cycle, 9? luma, "?" */
    { MACHINE_SYNC_PALN, 1 }
};

static int set_model(int model, void *param)
{
    int old;

    switch (model) {
        case VICII_MODEL_6569:
        case VICII_MODEL_8565:
        case VICII_MODEL_6569R1:
        case VICII_MODEL_6567:
        case VICII_MODEL_8562:
        case VICII_MODEL_6567R56A:
        case VICII_MODEL_6572:
            break;
        default:
            return -1;
    }

    old = vicii_resources.model;
    vicii_resources.model = model;

    if (model != old) {
        vicii_chip_model_init();
        resources_set_int("MachineVideoStandard", vicii_info[model].video);
        set_new_luminances(vicii_info[model].luma, NULL);
    }

    return 0;
}

static const resource_int_t resources_int[] =
{
    { "VICIIBorderMode", VICII_NORMAL_BORDERS, RES_EVENT_SAME, NULL,
      &vicii_resources.border_mode,
      set_border_mode, NULL },
    { "VICIICheckSsColl", 1, RES_EVENT_SAME, NULL,
      &vicii_resources.sprite_sprite_collisions_enabled,
      set_sprite_sprite_collisions_enabled, NULL },
    { "VICIICheckSbColl", 1, RES_EVENT_SAME, NULL,
      &vicii_resources.sprite_background_collisions_enabled,
      set_sprite_background_collisions_enabled, NULL },
    { "VICIINewLuminances", 1, RES_EVENT_NO, NULL,
      &vicii_resources.new_luminances,
      set_new_luminances, NULL },
    { "VICIIModel", VICII_MODEL_6569, RES_EVENT_NO, NULL,
      &vicii_resources.model,
      set_model, NULL },
    { "VICIIVSPBug", 0, RES_EVENT_SAME, NULL,
      &vicii_resources.vsp_bug_enabled,
      set_vsp_bug_enabled, NULL },
    { NULL }
};


int vicii_resources_init(void)
{
    video_chip_cap.dsize_allowed = ARCHDEP_VICII_DSIZE;
    video_chip_cap.dsize_default = ARCHDEP_VICII_DSIZE;
    video_chip_cap.dsize_limit_width = 0;
    video_chip_cap.dsize_limit_height = 0;
    video_chip_cap.dscan_allowed = ARCHDEP_VICII_DSCAN;
    video_chip_cap.hwscale_allowed = ARCHDEP_VICII_HWSCALE;
    video_chip_cap.scale2x_allowed = ARCHDEP_VICII_DSIZE;
    video_chip_cap.external_palette_name = "default";
    video_chip_cap.double_buffering_allowed = ARCHDEP_VICII_DBUF;
    video_chip_cap.single_mode.sizex = 1;
    video_chip_cap.single_mode.sizey = 1;
    video_chip_cap.single_mode.rmode = VIDEO_RENDER_PAL_1X1;
    video_chip_cap.double_mode.sizex = 2;
    video_chip_cap.double_mode.sizey = 2;
    video_chip_cap.double_mode.rmode = VIDEO_RENDER_PAL_2X2;

    fullscreen_capability(&(video_chip_cap.fullscreen));

    vicii.video_chip_cap = &video_chip_cap;

    if (raster_resources_chip_init("VICII", &vicii.raster, &video_chip_cap) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}
