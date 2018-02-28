/*
 * raster-cmdline-options.c - Raster-based video chip emulation helper,
 *                            command line options.
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

#include "cmdline.h"
#include "lib.h"
#include "machine.h"
#include "raster-cmdline-options.h"
#include "translate.h"
#include "util.h"
#include "video.h"

static const char *cname_chip[] = {
    "-", "vcache", "VideoCache",
    "+", "vcache", "VideoCache",
    NULL
};

static cmdline_option_t cmdline_options_chip[] =
{
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_VIDEO_CACHE,
      NULL, NULL },
    { NULL, SET_RESOURCE, 0,
      NULL, NULL, NULL, (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_VIDEO_CACHE,
      NULL, NULL },
    CMDLINE_LIST_END
};

int raster_cmdline_options_chip_init(const char *chipname,
                                     struct video_chip_cap_s *video_chip_cap)
{
    unsigned int i;

    if (machine_class != VICE_MACHINE_VSID) {
        for (i = 0; cname_chip[i * 3] != NULL; i++) {
            cmdline_options_chip[i].name = util_concat(cname_chip[i * 3], chipname,
                                                       cname_chip[i * 3 + 1], NULL);
            cmdline_options_chip[i].resource_name = util_concat(chipname,
                                                                cname_chip[i * 3 + 2], NULL);
        }

        if (cmdline_register_options(cmdline_options_chip) < 0) {
            return -1;
        }

        for (i = 0; cname_chip[i * 3] != NULL; i++) {
            lib_free(cmdline_options_chip[i].name);
            lib_free(cmdline_options_chip[i].resource_name);
        }
    }

    return video_cmdline_options_chip_init(chipname, video_chip_cap);
}
