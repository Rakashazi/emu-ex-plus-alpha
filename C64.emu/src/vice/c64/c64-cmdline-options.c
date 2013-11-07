/*
 * c64-cmdline-options.c
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
#include <stdlib.h>
#include <string.h>

#include "c64model.h"
#include "c64-cmdline-options.h"
#include "c64-resources.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "translate.h"

int set_cia_model(const char *value, void *extra_param)
{
    int model;

    model = atoi(value);
    c64_resources_update_cia_models(model);

    return 0;
}

struct model_s {
    const char *name;
    int model;
};

static struct model_s model_match[] = {
    { "c64", C64MODEL_C64_PAL },
    { "breadbox", C64MODEL_C64_PAL },
    { "pal", C64MODEL_C64_PAL },
    { "c64c", C64MODEL_C64C_PAL },
    { "c64new", C64MODEL_C64C_PAL },
    { "newpal", C64MODEL_C64C_PAL },
    { "c64old", C64MODEL_C64_OLD_PAL },
    { "oldpal", C64MODEL_C64_OLD_PAL },
    { "ntsc", C64MODEL_C64_NTSC },
    { "c64ntsc", C64MODEL_C64_NTSC },
    { "c64cntsc", C64MODEL_C64C_NTSC },
    { "newntsc", C64MODEL_C64C_NTSC },
    { "c64newntsc", C64MODEL_C64C_NTSC },
    { "oldntsc", C64MODEL_C64_OLD_NTSC },
    { "c64oldntsc", C64MODEL_C64_OLD_NTSC },
    { "paln", C64MODEL_C64_PAL_N },
    { "drean", C64MODEL_C64_PAL_N },
    { "sx64", C64MODEL_C64SX_PAL },
    { "sx64pal", C64MODEL_C64SX_PAL },
    { "sx64ntsc", C64MODEL_C64SX_NTSC },
    { "pet64", C64MODEL_PET64_PAL },
    { "pet64pal", C64MODEL_PET64_PAL },
    { "pet64ntsc", C64MODEL_PET64_NTSC },
    { "max", C64MODEL_ULTIMAX },
    { "ultimax", C64MODEL_ULTIMAX },
    { "gs", C64MODEL_C64_GS },
    { "c64gs", C64MODEL_C64_GS },
    { "jap", C64MODEL_C64_JAP },
    { "c64jap", C64MODEL_C64_JAP },
    { NULL, C64MODEL_UNKNOWN }
};

static int set_c64_model(const char *param, void *extra_param)
{
    int model = C64MODEL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(model_match[i].name, param) == 0) {
            model = model_match[i].model;
        }
        i++;
    } while ((model == C64MODEL_UNKNOWN) && (model_match[i].name != NULL));

    if (model == C64MODEL_UNKNOWN) {
        return -1;
    }

    c64model_set(model);

    return 0;
}

static int set_video_standard(const char *param, void *extra_param)
{
    int value = vice_ptr_to_int(extra_param);

    switch (machine_class) {
        case VICE_MACHINE_C64SC:
            switch (value) {
                case MACHINE_SYNC_PAL:
                default:
                    return set_c64_model("pal", NULL);

                case MACHINE_SYNC_NTSC:
                    return set_c64_model("ntsc", NULL);

                case MACHINE_SYNC_NTSCOLD:
                    return set_c64_model("oldntsc", NULL);

                case MACHINE_SYNC_PALN:
                    return set_c64_model("paln", NULL);
            }
        default:
            return resources_set_int("MachineVideoStandard", value);
    }
}

static const cmdline_option_t cmdline_options[] = {
    { "-pal", CALL_FUNCTION, 0,
      set_video_standard, (void *)MACHINE_SYNC_PAL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PAL_SYNC_FACTOR,
      NULL, NULL },
    { "-ntsc", CALL_FUNCTION, 0,
      set_video_standard, (void *)MACHINE_SYNC_NTSC, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-ntscold", CALL_FUNCTION, 0,
      set_video_standard, (void *)MACHINE_SYNC_NTSCOLD, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_OLD_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-paln", CALL_FUNCTION, 0,
      set_video_standard, (void *)MACHINE_SYNC_PALN, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PALN_SYNC_FACTOR,
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
    { "-kernalrev", SET_RESOURCE, 1,
      NULL, NULL, "KernalRev", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_REVISION, IDCLS_PATCH_KERNAL_TO_REVISION,
      NULL, NULL },
#ifdef HAVE_RS232
    { "-acia1", SET_RESOURCE, 0,
      NULL, NULL, "Acia1Enable", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DEXX_ACIA_RS232_EMU,
      NULL, NULL },
    { "+acia1", SET_RESOURCE, 0,
      NULL, NULL, "Acia1Enable", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DEXX_ACIA_RS232_EMU,
      NULL, NULL },
#endif
#ifdef COMMON_KBD
    { "-keymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapIndex", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_INDEX_KEYMAP_FILE_0_2,
      NULL, NULL },
    { "-symkeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapSymFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_SYM_KEYMAP_FILE_NAME,
      NULL, NULL },
    { "-symdekeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapSymDeFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_NAME_SYM_DE_KEYMAP,
      NULL, NULL },
    { "-poskeymap", SET_RESOURCE, 1,
      NULL, NULL, "KeymapPosFile", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_POS_KEYMAP_FILE_NAME,
      NULL, NULL },
#endif
    { "-ciamodel", CALL_FUNCTION, 1,
      set_cia_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_BOTH_CIA_MODELS,
      NULL, NULL },
    { "-cia1model", SET_RESOURCE, 1,
      NULL, NULL, "CIA1Model", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_CIA1_MODEL,
      NULL, NULL },
    { "-cia2model", SET_RESOURCE, 1,
      NULL, NULL, "CIA2Model", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_CIA2_MODEL,
      NULL, NULL },
    { "-model", CALL_FUNCTION, 1,
      set_c64_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODEL, IDCLS_SET_C64_MODEL,
      NULL, NULL },
    { "-burstmod", SET_RESOURCE, 1,
      NULL, NULL, "BurstMod", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_BURST_MOD,
      NULL, NULL },
    { "-iecreset", SET_RESOURCE, 1,
      NULL, NULL, "IECReset", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_IEC_RESET,
      NULL, NULL },
    { NULL }
};

int c64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
