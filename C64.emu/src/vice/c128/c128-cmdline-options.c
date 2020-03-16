/** \file   c128-cmdline-options.c
 * \brief   C128 command line options
 *
 * \author  Andreas Boose <viceteam@t-online.de>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdlib.h>
#include <string.h>

#include "c128-cmdline-options.h"
#include "c128-resources.h"
#include "c128model.h"
#include "cmdline.h"
#include "machine.h"

static int set_cia_model(const char *value, void *extra_param)
{
    int model;

    model = atoi(value);
    c128_resources_update_cia_models(model);

    return 0;
}

struct model_s {
    const char *name;
    int model;
};

static struct model_s model_match[] = {
    { "c128", C128MODEL_C128_PAL },
    { "c128dcr", C128MODEL_C128DCR_PAL },
    { "pal", C128MODEL_C128_PAL },
    { "ntsc", C128MODEL_C128_NTSC },
    { NULL, C128MODEL_UNKNOWN }
};

static int set_c128_model(const char *param, void *extra_param)
{
    int model = C128MODEL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(model_match[i].name, param) == 0) {
            model = model_match[i].model;
        }
        i++;
    } while ((model == C128MODEL_UNKNOWN) && (model_match[i].name != NULL));

    if (model == C128MODEL_UNKNOWN) {
        return -1;
    }

    c128model_set(model);

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
      NULL, NULL, "KernalIntName", NULL,
      "<Name>", "Specify name of international Kernal ROM image" },
    { "-kernalde", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalDEName", NULL,
      "<Name>", "Specify name of German Kernal ROM image" },
    { "-kernalfi", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalFIName", NULL,
      "<Name>", "Specify name of Finnish Kernal ROM image" },
    { "-kernalfr", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalFRName", NULL,
      "<Name>", "Specify name of French Kernal ROM image" },
    { "-kernalit", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalITName", NULL,
      "<Name>", "Specify name of Italian Kernal ROM image" },
    { "-kernalno", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalNOName", NULL,
      "<Name>", "Specify name of Norwegian Kernal ROM image" },
    { "-kernalse", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalSEName", NULL,
      "<Name>", "Specify name of Swedish Kernal ROM image" },
    { "-kernalch", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalCHName", NULL,
      "<Name>", "Specify name of Swiss Kernal ROM image" },
    { "-basiclo", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicLoName", NULL,
      "<Name>", "Specify name of BASIC ROM image (lower part)" },
    { "-basichi", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicHiName", NULL,
      "<Name>", "Specify name of BASIC ROM image (higher part)" },
    { "-chargen", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenIntName", NULL,
      "<Name>", "Specify name of international character generator ROM image" },
    { "-chargde", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenDEName", NULL,
      "<Name>", "Specify name of German character generator ROM image" },
    { "-chargfr", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenFRName", NULL,
      "<Name>", "Specify name of French character generator ROM image" },
    { "-chargse", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenSEName", NULL,
      "<Name>", "Specify name of Swedish character generator ROM image" },
    { "-chargch", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenCHName", NULL,
      "<Name>", "Specify name of Swiss character generator ROM image" },
    { "-chargno", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenNOName", NULL,
      "<Name>", "Specify name of Norwegian character generator ROM image" },
    { "-kernal64", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Kernal64Name", NULL,
      "<Name>", "Specify name of C64 mode Kernal ROM image" },
    { "-basic64", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Basic64Name", NULL,
      "<Name>", "Specify name of C64 mode BASIC ROM image" },
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    { "-acia1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Acia1Enable", (void *)1,
      NULL, "Enable the ACIA RS232 interface emulation" },
    { "+acia1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Acia1Enable", (void *)0,
      NULL, "Disable the ACIA RS232 interface emulation" },
#endif
    { "-ciamodel", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_cia_model, NULL, NULL, NULL,
      "<Model>", "Set both CIA models (0 = old 6526, 1 = new 8521)" },
    { "-cia1model", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "CIA1Model", NULL,
      "<Model>", "Set CIA 1 model (0 = old 6526, 1 = new 8521)" },
    { "-cia2model", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "CIA2Model", NULL,
      "<Model>", "Set CIA 2 model (0 = old 6526, 1 = new 8521)" },
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_c128_model, NULL, NULL, NULL,
      "<Model>", "Set C128 model (c128/c128dcr, pal/ntsc)" },
    { "-machinetype", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MachineType", NULL,
      "<Type>", "Set C128 machine type (0: International, 1: Finnish, 2: French, 3: German, 4: Italian, 5: Norwegian, 6: Swedish, 7: Swiss)" },
    { "-c128fullbanks", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "C128FullBanks", (void *)1,
      NULL, "Enable RAM banks 2 and 3" },
    { "+c128fullbanks", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "C128FullBanks", (void *)0,
      NULL, "Disable RAM banks 2 and 3" },
    CMDLINE_LIST_END
};

int c128_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
