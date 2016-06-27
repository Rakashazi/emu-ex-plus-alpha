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
#include "translate.h"

struct modtab_s {
    const char *model;
    int modelline;
    int modelid;
};
typedef struct modtab_s modtab_t;

/* FIXME: add more/all models */
static modtab_t modtab[] = {
    { "510", VICE_MACHINE_CBM5x0, CBM2MODEL_510_PAL },
    { "610", VICE_MACHINE_CBM6x0, CBM2MODEL_610_PAL },
    { "620", VICE_MACHINE_CBM6x0, CBM2MODEL_620_PAL },
    { "620+", VICE_MACHINE_CBM6x0, CBM2MODEL_620PLUS_PAL },
    { "710", VICE_MACHINE_CBM6x0, CBM2MODEL_710_NTSC },
    { "720", VICE_MACHINE_CBM6x0, CBM2MODEL_720_NTSC },
    { "720+", VICE_MACHINE_CBM6x0, CBM2MODEL_720PLUS_NTSC },
    { NULL }
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

static const cmdline_option_t cmdline_options[] = {
    { "-pal", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_PAL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PAL_SYNC_FACTOR,
      NULL, NULL },
    { "-ntsc", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSC,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-kernal", SET_RESOURCE, 1,
      NULL, NULL, "KernalName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_KERNAL_ROM_NAME,
      NULL, NULL },
    { "-basic", SET_RESOURCE, 1,
      NULL, NULL, "BasicName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_BASIC_ROM_NAME,
      NULL, NULL },
    { "-chargen", SET_RESOURCE, 1,
      NULL, NULL, "ChargenName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CHARGEN_ROM_NAME,
      NULL, NULL },
    { "-ram08", SET_RESOURCE, 0,
      NULL, NULL, "Ram08", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_0800,
      NULL, NULL, },
    { "+ram08", SET_RESOURCE, 0,
      NULL, NULL, "Ram08", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_0800,
      NULL, NULL },
    { "-ram1", SET_RESOURCE, 0,
      NULL, NULL, "Ram1", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_1000,
      NULL, NULL },
    { "+ram1", SET_RESOURCE, 0,
      NULL, NULL, "Ram1", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_1000,
      NULL, NULL },
    { "-ram2", SET_RESOURCE, 0,
      NULL, NULL, "Ram2", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_2000,
      NULL, NULL },
    { "+ram2", SET_RESOURCE, 0,
      NULL, NULL, "Ram2", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_2000,
      NULL, NULL },
    { "-ram4", SET_RESOURCE, 0,
      NULL, NULL, "Ram4", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_4000,
      NULL, NULL },
    { "+ram4", SET_RESOURCE, 0,
      NULL, NULL, "Ram4", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_4000,
      NULL, NULL },
    { "-ram6", SET_RESOURCE, 0,
      NULL, NULL, "Ram6", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_6000,
      NULL, NULL },
    { "+ram6", SET_RESOURCE, 0,
      NULL, NULL, "Ram6", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_6000,
      NULL, NULL },
    { "-ramC", SET_RESOURCE, 0,
      NULL, NULL, "RamC", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RAM_MAPPING_IN_C000,
      NULL, NULL },
    { "+ramC", SET_RESOURCE, 0,
      NULL, NULL, "RamC", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RAM_MAPPING_IN_C000,
      NULL, NULL },
    { "-cia1model", SET_RESOURCE, 1,
      NULL, NULL, "CIA1Model", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_CIA1_MODEL,
      NULL, NULL },
    { NULL }
};

static const cmdline_option_t cbm2_cmdline_options[] = {
    { "-model", CALL_FUNCTION, 1,
      cbm2_set_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODELNUMBER, IDCLS_SPECIFY_CBM2_MODEL,
      NULL, NULL },
    { "-ramsize", SET_RESOURCE, 1,
      NULL, NULL, "RamSize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_RAMSIZE, IDCLS_SPECIFY_SIZE_OF_RAM_CBM2,
      NULL, NULL },
    { NULL }
};

static const cmdline_option_t cbm5x0_cmdline_options[] = {
    { "-model", CALL_FUNCTION, 1,
      cbm2_set_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODELNUMBER, IDCLS_SPECIFY_CBM5X0_MODEL,
      NULL, NULL },
    { "-ramsize", SET_RESOURCE, 1,
      NULL, NULL, "RamSize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_RAMSIZE, IDCLS_SPECIFY_SIZE_OF_RAM,
      NULL, NULL },
    { NULL }
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
