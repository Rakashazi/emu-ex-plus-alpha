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
#include "translate.h"

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
    { "-functionlo", SET_RESOURCE, 1,
      NULL, NULL, "FunctionLowName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_FUNCTION_LOW_ROM_NAME,
      NULL, NULL },
    { "-functionhi", SET_RESOURCE, 1,
      NULL, NULL, "FunctionHighName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_FUNCTION_HIGH_ROM_NAME,
      NULL, NULL },
    { "-c1lo", SET_RESOURCE, 1,
      NULL, NULL, "c1loName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CART_1_LOW_ROM_NAME,
      NULL, NULL },
    { "-c1hi", SET_RESOURCE, 1,
      NULL, NULL, "c1hiName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CART_1_HIGH_ROM_NAME,
      NULL, NULL },
    { "-c2lo", SET_RESOURCE, 1,
      NULL, NULL, "c2loName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CART_2_LOW_ROM_NAME,
      NULL, NULL },
    { "-c2hi", SET_RESOURCE, 1,
      NULL, NULL, "c2hiName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CART_2_HIGH_ROM_NAME,
      NULL, NULL },
    { "-ramsize", SET_RESOURCE, 1,
      NULL, NULL, "RamSize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_RAMSIZE, IDCLS_SPECIFY_RAM_INSTALLED,
      NULL, NULL },
    { "-model", CALL_FUNCTION, 1,
      set_plus4_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_PLUS4_MODEL,
      NULL, NULL },
    { NULL }
};

int plus4_cmdline_options_init(void)
{
    if (plus4_memory_hacks_cmdline_options_init() < 0) {
        return -1;
    }
    return cmdline_register_options(cmdline_options);
}
