/*
 * raster-resources.c - Raster-based video chip emulation helper, resources.
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

#include "lib.h"
#include "machine.h"
#include "raster.h"
#include "raster-resources.h"
#include "resources.h"
#include "types.h"
#include "util.h"
#include "video.h"


#ifdef __MSDOS__
#define DEFAULT_VideoCache_VALUE 0
#else
#define DEFAULT_VideoCache_VALUE 99
#endif

struct raster_resource_chip_s {
    raster_t *raster;
    int video_cache_enabled;
};
typedef struct raster_resource_chip_s raster_resource_chip_t;

static int set_video_cache_enabled(int val, void *param)
{
    raster_resource_chip_t *raster_resource_chip;

    raster_resource_chip = (raster_resource_chip_t *)param;

    if (val == 99) {
        /* HACK: some machines do not have a working video cache, so
                 disable it by default */
        if ((machine_class == VICE_MACHINE_C64DTV) ||
            (machine_class == VICE_MACHINE_SCPU64) ||
            (machine_class == VICE_MACHINE_C64SC) ||
            (machine_class == VICE_MACHINE_PLUS4)) {
            val = 0;
        } else {
            val = 1;
        }
    }

    if (val >= 0) {
        raster_resource_chip->video_cache_enabled = val;
    }

    raster_enable_cache(raster_resource_chip->raster,
                        raster_resource_chip->video_cache_enabled);

    return 0;
}

static const char *rname_chip[] = { "VideoCache", NULL };

static resource_int_t resources_chip[] =
{
    { NULL, DEFAULT_VideoCache_VALUE, RES_EVENT_NO, NULL,
      NULL, set_video_cache_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int raster_resources_chip_init(const char *chipname, raster_t *raster,
                               struct video_chip_cap_s *video_chip_cap)
{
    unsigned int i;
    raster_resource_chip_t *raster_resource_chip;

    raster_resource_chip = lib_calloc(1, sizeof(raster_resource_chip_t));

    raster->raster_resource_chip = raster_resource_chip;
    raster_resource_chip->raster = raster;

    if (machine_class != VICE_MACHINE_VSID) {
        for (i = 0; rname_chip[i] != NULL; i++) {
            resources_chip[i].name = util_concat(chipname, rname_chip[i], NULL);
            resources_chip[i].value_ptr
                = &(raster_resource_chip->video_cache_enabled);
            resources_chip[i].param = (void *)raster_resource_chip;
        }
    }

    raster->canvas = video_canvas_init();

    if (machine_class != VICE_MACHINE_VSID) {
        if (resources_register_int(resources_chip) < 0) {
            return -1;
        }

        for (i = 0; rname_chip[i] != NULL; i++) {
            lib_free(resources_chip[i].name);
        }
    } else {
        set_video_cache_enabled(0, (void *)raster_resource_chip);
    }

    return video_resources_chip_init(chipname, &raster->canvas, video_chip_cap);
}

void raster_resources_chip_shutdown(raster_t *raster)
{
    video_resources_chip_shutdown(raster->canvas);
    lib_free(raster->raster_resource_chip);
    video_canvas_shutdown(raster->canvas);
}
