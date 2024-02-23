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
#include "vicii-color.h"
#include "vicii-resources.h"
#include "vicii-timing.h"
#include "vicii.h"
#include "viciitypes.h"
#include "video.h"
#include "vsync.h"

/* The old code, before r43758, also delays the change of the actual resource
   value until the next vblank. This breaks eg the SDL menus (and is generally
   wrong). The new code changes the resource value immediately, and only delays
   the update of the internal state (which is the correct thing to do). However,
   this apparently breaks the rendering of the second (VDC) window in x128 for
   some reason. */

/* NOTE: this should be fixed after r44823 */

/* #define OLDCODE */

#ifdef OLDCODE
static int next_border_mode;
#endif

vicii_resources_t vicii_resources = { 0, 0, 0, 0, 0 };
static video_chip_cap_t video_chip_cap;

static void on_vsync_set_border_mode(void *unused)
{
    int sync;
    int pf;

    if (resources_get_int("MachineVideoStandard", &sync) < 0) {
        sync = MACHINE_SYNC_PAL;
    }
    if (resources_get_int("MachinePowerFrequency", &pf) < 0) {
        switch (sync) {
            case MACHINE_SYNC_PAL:
            case MACHINE_SYNC_PALN:
                pf = 50;
            break;
            default:
                pf = 60;
            break;
        }
    }
#ifdef OLDCODE
    if (vicii_resources.border_mode != next_border_mode) {
        vicii_resources.border_mode = next_border_mode;
#endif
        machine_change_timing(sync, pf, vicii_resources.border_mode);
#ifdef OLDCODE
    }
#endif
}

static int set_border_mode(int val, void *param)
{
    switch (val) {
        case VICII_NORMAL_BORDERS:
        case VICII_FULL_BORDERS:
        case VICII_DEBUG_BORDERS:
        case VICII_NO_BORDERS:
            break;
        default:
            return -1;
    }

#ifdef OLDCODE
    next_border_mode = val;
#else
    vicii_resources.border_mode = val;
#endif
    vsync_on_vsync_do(on_vsync_set_border_mode, NULL);

    return 0;
}

static int set_sprite_sprite_collisions_enabled(int val, void *param)
{
    vicii_resources.sprite_sprite_collisions_enabled = val ? 1 : 0;

    return 0;
}

static int set_sprite_background_collisions_enabled(int val, void *param)
{
    vicii_resources.sprite_background_collisions_enabled = val ? 1 : 0;

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
    RESOURCE_INT_LIST_END
};

static int set_new_luminances(int val, void *param)
{
    vicii_resources.new_luminances = val ? 1 : 0;

    return vicii_color_update_palette(vicii.raster.canvas);
}

static const resource_int_t resources_int_dtv[] =
{
    { "VICIINewLuminances", 1, RES_EVENT_NO, NULL,
      &vicii_resources.new_luminances,
      set_new_luminances, NULL },
    RESOURCE_INT_LIST_END
};

int vicii_resources_init(void)
{
    video_chip_cap.dsize_allowed = ARCHDEP_VICII_DSIZE;
    video_chip_cap.dsize_default = ARCHDEP_VICII_DSIZE;
    video_chip_cap.dsize_limit_width = 0;
    video_chip_cap.dsize_limit_height = 0;
    video_chip_cap.dscan_allowed = ARCHDEP_VICII_DSCAN;
    if (machine_class == VICE_MACHINE_C64DTV) {
        video_chip_cap.external_palette_name = "spiff";
    } else {
        video_chip_cap.external_palette_name = "pepto-pal";
    }
    video_chip_cap.video_has_palntsc = 1;

    video_chip_cap.single_mode.sizex = 1;
    video_chip_cap.single_mode.sizey = 1;
    video_chip_cap.single_mode.rmode = VIDEO_RENDER_PAL_NTSC_1X1;
    video_chip_cap.double_mode.sizex = 2;
    video_chip_cap.double_mode.sizey = 2;
    video_chip_cap.double_mode.rmode = VIDEO_RENDER_PAL_NTSC_2X2;

    fullscreen_capability(&(video_chip_cap.fullscreen));

    vicii.video_chip_cap = &video_chip_cap;

    if (raster_resources_chip_init("VICII", &vicii.raster, &video_chip_cap) < 0) {
        return -1;
    }
    if (machine_class == VICE_MACHINE_C64DTV) {
        if (resources_register_int(resources_int_dtv) < 0) {
            return -1;
        }
    }

    return resources_register_int(resources_int);
}

void vicii_comply_with_video_standard(int machine_sync)
{
    /* dummy function */
}
