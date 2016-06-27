/*
 * ted-cmdline-options.c - Command-line options for the TED emulation.
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
 * */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "machine.h"
#include "raster-cmdline-options.h"
#include "resources.h"
#include "ted-cmdline-options.h"
#include "ted-resources.h"
#include "ted.h"
#include "tedtypes.h"
#include "translate.h"

int border_set_func(const char *value, void *extra_param)
{
    int video;

    resources_get_int("MachineVideoStandard", &video);

    if (strcmp(value, "1") == 0 || strcmp(value, "full") == 0) {
        ted_resources.border_mode = TED_FULL_BORDERS;
    } else if (strcmp(value, "2") == 0 || strcmp(value, "debug") == 0) {
        ted_resources.border_mode = TED_DEBUG_BORDERS;
    } else if (strcmp(value, "3") == 0 || strcmp(value, "none") == 0) {
        ted_resources.border_mode = TED_NO_BORDERS;
    } else {
        ted_resources.border_mode = TED_NORMAL_BORDERS;
    }

    machine_change_timing(video ^ TED_BORDER_MODE(ted_resources.border_mode));

    return 0;
}

/* TED command-line options.  */
static const cmdline_option_t cmdline_options[] =
{
    { "-TEDborders", CALL_FUNCTION, 1,
      border_set_func, NULL, "TEDBorderMode", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODE, IDCLS_SET_BORDER_MODE },
    CMDLINE_LIST_END
};

int ted_cmdline_options_init(void)
{
    if (raster_cmdline_options_chip_init("TED", ted.video_chip_cap) < 0) {
        return -1;
    }

    return cmdline_register_options(cmdline_options);
}
