/*
 * c64dtv-cmdline-options.c
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
#include <string.h>

#include "c64dtv-cmdline-options.h"
#include "c64dtvmodel.h"
#include "cmdline.h"
#include "machine.h"

struct model_s {
    const char *name;
    int model;
};

static struct model_s model_match[] = {
    { "v2", DTVMODEL_V2_PAL },
    { "v2pal", DTVMODEL_V2_PAL },
    { "v2ntsc", DTVMODEL_V2_NTSC },
    { "v3", DTVMODEL_V3_PAL },
    { "v3pal", DTVMODEL_V3_PAL },
    { "v3ntsc", DTVMODEL_V3_NTSC },
    { "hummer", DTVMODEL_HUMMER_NTSC },
    { NULL, DTVMODEL_UNKNOWN }
};

static int set_dtv_model(const char *param, void *extra_param)
{
    int model = DTVMODEL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(model_match[i].name, param) == 0) {
            model = model_match[i].model;
        }
        i++;
    } while ((model == DTVMODEL_UNKNOWN) && (model_match[i].name != NULL));

    if (model == DTVMODEL_UNKNOWN) {
        return -1;
    }

    dtvmodel_set(model);

    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    { "-pal", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_PAL,
      NULL, "Use PAL sync factor" },
    { "-ntsc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSC,
      NULL, "Use NTSC sync factor" },
    { "-kernal", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalName", NULL,
      "<Name>", "Specify name of Kernal ROM image" },
    { "-basic", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicName", NULL,
      "<Name>", "Specify name of BASIC ROM image" },
    { "-chargen", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenName", NULL,
      "<Name>", "Specify name of character generator ROM image" },
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_dtv_model, NULL, NULL, NULL,
      "<Model>", "Set DTV model (v2/v2pal/v2ntsc, v3/v3pal/v3ntsc, hummer)" },
    { "-hummeradc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "HummerADC", (void *)1,
      NULL, "Enable Hummer ADC" },
    { "+hummeradc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "HummerADC", (void *)0,
      NULL, "Disable Hummer ADC" },
    CMDLINE_LIST_END
};

int c64dtv_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
