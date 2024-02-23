/*
 * vic-cmdline-options.c - Command-line options for the VIC-I emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <string.h>

#include "cmdline.h"
#include "machine.h"
#include "raster-cmdline-options.h"
#include "resources.h"
#include "vic-cmdline-options.h"
#include "vic-resources.h"
#include "vic.h"
#include "victypes.h"

static int border_set_func(const char *value, void *extra_param)
{
    if (strcmp(value, "1") == 0 || strcmp(value, "full") == 0) {
        resources_set_int("VICBorderMode", 1);
    } else if (strcmp(value, "2") == 0 || strcmp(value, "debug") == 0) {
        resources_set_int("VICBorderMode", 2);
    } else if (strcmp(value, "3") == 0 || strcmp(value, "none") == 0) {
        resources_set_int("VICBorderMode", 3);
    } else {
        resources_set_int("VICBorderMode", 0);
    }

    return 0;
}

/* VIC command-line options.  */
static const cmdline_option_t cmdline_options[] =
{
    { "-VICborders", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      border_set_func, NULL, "VICBorderMode", NULL,
      "<Mode>", "Set border display mode (0: normal, 1: full, 2: debug, 3: none)" },
    CMDLINE_LIST_END
};

int vic_cmdline_options_init(void)
{
    if (raster_cmdline_options_chip_init("VIC", vic.video_chip_cap) < 0) {
        return -1;
    }

    return cmdline_register_options(cmdline_options);
}
