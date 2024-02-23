/*
 * plus4-cmdline-options.c
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
#include <string.h>

#include "cmdline.h"
#include "machine.h"
#include "plus4-cmdline-options.h"
#include "plus4memcsory256k.h"
#include "plus4memhacks.h"
#include "plus4memhannes256k.h"
#include "plus4model.h"

struct model_s {
    const char *name;
    int model;
};

static struct model_s model_match[] = {
    { "c16", PLUS4MODEL_C16_PAL },
    { "c16pal", PLUS4MODEL_C16_PAL },
    { "c16ntsc", PLUS4MODEL_C16_NTSC },
    { "plus4", PLUS4MODEL_PLUS4_PAL },
    { "plus4pal", PLUS4MODEL_PLUS4_PAL },
    { "plus4ntsc", PLUS4MODEL_PLUS4_NTSC },
    { "v364", PLUS4MODEL_V364_NTSC },
    { "cv364", PLUS4MODEL_V364_NTSC },
    { "c232", PLUS4MODEL_232_NTSC },
    { NULL, PLUS4MODEL_UNKNOWN }
};

static int set_plus4_model(const char *param, void *extra_param)
{
    int model = PLUS4MODEL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(model_match[i].name, param) == 0) {
            model = model_match[i].model;
        }
        i++;
    } while ((model == PLUS4MODEL_UNKNOWN) && (model_match[i].name != NULL));

    if (model == PLUS4MODEL_UNKNOWN) {
        return -1;
    }

    plus4model_set(model);

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
#if 0
    { "-power50", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachinePowerFrequency", (void *)50,
      NULL, "Use 50Hz Power-grid frequency" },
    { "-power60", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachinePowerFrequency", (void *)60,
      NULL, "Use 60Hz Power-grid frequency" },
#endif
    { "-kernal", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalName", NULL,
      "<Name>", "Specify name of Kernal ROM image" },
    { "-basic", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicName", NULL,
      "<Name>", "Specify name of BASIC ROM image" },
    { "-functionlo", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FunctionLowName", NULL,
      "<Name>", "Specify name of Function low ROM image" },
    { "-functionhi", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FunctionHighName", NULL,
      "<Name>", "Specify name of Function high ROM image" },
    { "-c2lo", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "c2loName", NULL,
      "<Name>", "Specify name of Cartridge 2 low ROM image" },
    { "-c2hi", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "c2hiName", NULL,
      "<Name>", "Specify name of Cartridge 2 high ROM image" },
    { "-ramsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RamSize", NULL,
      "<RAM size>", "Specify size of RAM installed in KiB (16/32/64)" },
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_plus4_model, NULL, NULL, NULL,
      "<Model>", "Set Plus4 model (c16/c16pal/c16ntsc, plus4/plus4pal/plus4ntsc, v364/cv364, c232)" },
    CMDLINE_LIST_END
};

int plus4_cmdline_options_init(void)
{
    if (plus4_memory_hacks_cmdline_options_init() < 0) {
        return -1;
    }
    return cmdline_register_options(cmdline_options);
}
