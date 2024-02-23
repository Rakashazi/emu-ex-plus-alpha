/*
 * c64-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include "c64rom.h"
#include "c64-cmdline-options.h"
#include "c64-resources.h"
#include "cmdline.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "vicii.h"

static int set_cia_model(const char *value, void *extra_param)
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
    int vicii_model;

    switch (machine_class) {
        case VICE_MACHINE_C64SC:
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
        default:
            return resources_set_int("MachineVideoStandard", value);
    }
}

struct kernal_s {
    const char *name;
    int rev;
};

/* NOTE: this table is duplicated in psid.c */
static struct kernal_s kernal_match[] = {
    { "0",    C64_KERNAL_JAP },
    { "jap",  C64_KERNAL_JAP },
    { "1",    C64_KERNAL_REV1 },
    { "2",    C64_KERNAL_REV2 },
    { "3",    C64_KERNAL_REV3 },
    { "67",   C64_KERNAL_SX64 },
    { "sx",   C64_KERNAL_SX64 },
    { "39",   C64_KERNAL_GS64 },
    { "gs",   C64_KERNAL_GS64 },
    { "100",  C64_KERNAL_4064 },
    { "4064", C64_KERNAL_4064 },
    { NULL, C64_KERNAL_UNKNOWN }
};

static int set_kernal_revision(const char *param, void *extra_param)
{
    int rev = C64_KERNAL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(kernal_match[i].name, param) == 0) {
            rev = kernal_match[i].rev;
        }
        i++;
    } while ((rev == C64_KERNAL_UNKNOWN) && (kernal_match[i].name != NULL));

    log_verbose("set_kernal_revision (\"-kernalrev\") val:'%s' rev: %d", param, rev);

    if (rev == C64_KERNAL_UNKNOWN) {
        log_error(LOG_DEFAULT, "invalid kernal revision (%d)", rev);
        return -1;
    }

    if (resources_set_int("KernalRev", rev) < 0) {
        log_error(LOG_DEFAULT, "failed to set kernal revision (%d)", rev);
    }

    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    /* NOTE: although we use CALL_FUNCTION, we put the resource that will be
             modified into the array - this helps reconstructing the cmdline */
    { "-pal", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_video_standard, (void *)MACHINE_SYNC_PAL, "MachineVideoStandard", (void *)MACHINE_SYNC_PAL,
      NULL, "Use PAL sync factor" },
    { "-ntsc", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_video_standard, (void *)MACHINE_SYNC_NTSC, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSC,
      NULL, "Use NTSC sync factor" },
    { "-ntscold", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_video_standard, (void *)MACHINE_SYNC_NTSCOLD, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSCOLD,
      NULL, "Use old NTSC sync factor" },
    { "-paln", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_video_standard, (void *)MACHINE_SYNC_PALN, "MachineVideoStandard", (void *)MACHINE_SYNC_PALN,
      NULL, "Use PAL-N sync factor" },
    { "-power50", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachinePowerFrequency", (void *)50,
      NULL, "Use 50Hz Power-grid frequency" },
    { "-power60", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachinePowerFrequency", (void *)60,
      NULL, "Use 60Hz Power-grid frequency" },
    { "-kernal", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalName", NULL,
      "<Name>", "Specify name of Kernal ROM image" },
    { "-basic", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicName", NULL,
      "<Name>", "Specify name of BASIC ROM image" },
    { "-chargen", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenName", NULL,
      "<Name>", "Specify name of character generator ROM image" },
    { "-kernalrev", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_kernal_revision, NULL, NULL, NULL,
      "<Revision>", "Patch the Kernal ROM to the specified <revision> "
      "(0/jap: japanese 1: rev. 1, 2: rev. 2, 3: rev. 3, 39/gs: C64 GS, 67/sx: sx64, 100/4064: 4064)" },
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
      set_c64_model, NULL, NULL, NULL,
      "<Model>", "Set C64 model (c64/c64c/c64old, ntsc/newntsc/oldntsc, drean, jap, c64gs, pet64, ultimax)" },
    { "-burstmod", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BurstMod", NULL,
      "<value>", "Burst modification (0 = None, 1 = CIA1, 2 = CIA2)" },
    { "-iecreset", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IECReset", NULL,
      "<value>", "Computer reset goes to IEC bus (0 = No, 1 = Yes)" },
    CMDLINE_LIST_END
};

int c64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
