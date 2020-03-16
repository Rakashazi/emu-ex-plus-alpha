/*
 * cbm2-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "cbm2-cmdline-options.h"
#include "cbm2mem.h"
#include "cbm2model.h"
#include "cmdline.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"

struct modtab_s {
    const char *model;
    int modelline;
    int modelid;
};
typedef struct modtab_s modtab_t;

#define MODTAB_LIST_END { NULL, 0, 0 }

/* FIXME: add more/all models */
static modtab_t modtab[] = {
    { "510", VICE_MACHINE_CBM5x0, CBM2MODEL_510_PAL },
    { "610", VICE_MACHINE_CBM6x0, CBM2MODEL_610_PAL },
    { "620", VICE_MACHINE_CBM6x0, CBM2MODEL_620_PAL },
    { "620+", VICE_MACHINE_CBM6x0, CBM2MODEL_620PLUS_PAL },
    { "710", VICE_MACHINE_CBM6x0, CBM2MODEL_710_NTSC },
    { "720", VICE_MACHINE_CBM6x0, CBM2MODEL_720_NTSC },
    { "720+", VICE_MACHINE_CBM6x0, CBM2MODEL_720PLUS_NTSC },
    MODTAB_LIST_END
};

static int cbm2_model = 1;

static int cbm2_set_model(const char *model, void *extra)
{
    int i;

    /* vsync_suspend_speed_eval(); */

    for (i = 0; modtab[i].model; i++) {
        if (machine_class != modtab[i].modelline) {
            continue;
        }
        if (strcmp(modtab[i].model, model)) {
            continue;
        }

        cbm2model_set(modtab[i].modelid);
        cbm2_model = i;

        /* we have to wait until we did enough initialization */
        if (!cbm2_init_ok) {
            return 0;
        }

        mem_powerup();
        mem_load();
        machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
        return 0;
    }
    return -1;
}

/* ------------------------------------------------------------------------- */

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
    { "-ram08", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram08", (void *)1,
      NULL, "Enable RAM mapping in $0800-$0FFF" },
    { "+ram08", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram08", (void *)0,
      NULL, "Disable RAM mapping in $0800-$0FFF" },
    { "-ram1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram1", (void *)1,
      NULL, "Enable RAM mapping in $1000-$1FFF" },
    { "+ram1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram1", (void *)0,
      NULL, "Disable RAM mapping in $1000-$1FFF" },
    { "-ram2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram2", (void *)1,
      NULL, "Enable RAM mapping in $2000-$3FFF" },
    { "+ram2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram2", (void *)0,
      NULL, "Disable RAM mapping in $2000-$3FFF" },
    { "-ram4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram4", (void *)1,
      NULL, "Enable RAM mapping in $4000-$5FFF" },
    { "+ram4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram4", (void *)0,
      NULL, "Disable RAM mapping in $4000-$5FFF" },
    { "-ram6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram6", (void *)1,
      NULL, "Enable RAM mapping in $6000-$7FFF" },
    { "+ram6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram6", (void *)0,
      NULL, "Disable RAM mapping in $6000-$7FFF" },
    { "-ramC", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RamC", (void *)1,
      NULL, "Enable RAM mapping in $C000-$CFFF" },
    { "+ramC", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RamC", (void *)0,
      NULL, "Disable RAM mapping in $C000-$CFFF" },
    { "-cia1model", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "CIA1Model", NULL,
      "<Model>", "Set CIA 1 model (0 = old 6526, 1 = new 8521)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cbm2_cmdline_options[] =
{
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cbm2_set_model, NULL, NULL, NULL,
      "<modelnumber>", "Specify CBM-II model to emulate. (610, 620, 620+, 710, 720, 720+)" },
    { "-ramsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RamSize", NULL,
      "<RAM size>", "Specify size of RAM (128/256/512/1024 kByte)" },
    CMDLINE_LIST_END
};

static const cmdline_option_t cbm5x0_cmdline_options[] =
{
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cbm2_set_model, NULL, NULL, NULL,
      "<modelnumber>", "Specify CBM-II model to emulate. (510)" },
    { "-ramsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RamSize", NULL,
      "<RAM size>", "Specify size of RAM (64/128/256/512/1024 kByte)" },
    CMDLINE_LIST_END
};

int cbm2_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_CBM5x0) {
        if (cmdline_register_options(cbm5x0_cmdline_options) < 0) {
            return -1;
        }
    } else {
        if (cmdline_register_options(cbm2_cmdline_options) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(cmdline_options);
}
