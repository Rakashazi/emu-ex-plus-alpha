/*
 * scpu64-cmdline-options.c
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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
#include "scpu64-cmdline-options.h"
#include "scpu64-resources.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "translate.h"
#include "vicii.h"

int set_cia_model(const char *value, void *extra_param)
{
    int model;

    model = atoi(value);
    scpu64_resources_update_cia_models(model);

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
    int vicii_model;

    resources_get_int("VICIIModel", &vicii_model);

    switch (value) {
        case MACHINE_SYNC_PAL:
        default:
            if (vicii_model == VICII_MODEL_8562 || vicii_model == VICII_MODEL_8565) {
                return resources_set_int("VICIIModel", VICII_MODEL_8565);
            } else if (vicii_model == VICII_MODEL_6567R56A) {
                return resources_set_int("VICIIModel", VICII_MODEL_6569R1);
            } else {
                return resources_set_int("VICIIModel", VICII_MODEL_6569);
            }
            break;
        case MACHINE_SYNC_NTSC:
            if (vicii_model == VICII_MODEL_8562 || vicii_model == VICII_MODEL_8565) {
                return resources_set_int("VICIIModel", VICII_MODEL_8562);
            } else {
                return resources_set_int("VICIIModel", VICII_MODEL_6567);
            }
            break;
        case MACHINE_SYNC_NTSCOLD:
                return resources_set_int("VICIIModel", VICII_MODEL_6567R56A);
        case MACHINE_SYNC_PALN:
                return resources_set_int("VICIIModel", VICII_MODEL_6572);
    }
    return 0;
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
    { "-scpu64", SET_RESOURCE, 1,
      NULL, NULL, "SCPU64Name", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_SCPU64_ROM_NAME,
      NULL, NULL },
    { "-chargen", SET_RESOURCE, 1,
      NULL, NULL, "ChargenName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_CHARGEN_ROM_NAME,
      NULL, NULL },
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
      IDCLS_P_MODEL, IDCLS_SET_SCPU64_MODEL,
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
    { "-simmsize", SET_RESOURCE, 1,
      NULL, NULL, "SIMMSize", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SET_SIMM_SIZE,
      NULL, NULL },
    { "-jiffyswitch", SET_RESOURCE, 0,
      NULL, NULL, "JiffySwitch", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_JIFFY_SWITCH,
      NULL, NULL },
    { "+jiffyswitch", SET_RESOURCE, 0,
      NULL, NULL, "JiffySwitch", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_JIFFY_SWITCH,
      NULL, NULL },
    { "-speedswitch", SET_RESOURCE, 0,
      NULL, NULL, "SpeedSwitch", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SPEED_SWITCH,
      NULL, NULL },
    { "+speedswitch", SET_RESOURCE, 0,
      NULL, NULL, "SpeedSwitch", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SPEED_SWITCH,
      NULL, NULL },
    { NULL }
};

int scpu64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
