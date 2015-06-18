/*
 * vsid-cmdline-options.c
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
#include "c64rom.h"
#include "c64-cmdline-options.h"
#include "c64-resources.h"
#include "cmdline.h"
#include "log.h"
#include "machine.h"
#include "patchrom.h"
#include "resources.h"
#include "translate.h"
#include "vicii.h"

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

static struct kernal_s kernal_match[] = {
    { "1", C64_KERNAL_REV1 },
    { "2", C64_KERNAL_REV2 },
    { "3", C64_KERNAL_REV3 },
    { "67", C64_KERNAL_SX64 },
    { "sx", C64_KERNAL_SX64 },
    { "100", C64_KERNAL_4064 },
    { "4064", C64_KERNAL_4064 },
    { NULL, C64_KERNAL_UNKNOWN }
};

static int set_kernal_revision(const char *param, void *extra_param)
{
    WORD sum;                   /* ROM checksum */
    int id;                     /* ROM identification number */
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

    if(!c64rom_isloaded()) {
        kernal_revision = rev;
        return 0;
    }

    if (c64rom_get_kernal_chksum_id(&sum, &id) < 0) {
        id = C64_KERNAL_UNKNOWN;
        kernal_revision = id;
    } else {
        if (patch_rom_idx(rev) >= 0) {
            kernal_revision = rev;
        } else {
            kernal_revision = id;
        }
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
    { "-kernalrev", CALL_FUNCTION, 1,
      set_kernal_revision, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_REVISION, IDCLS_PATCH_KERNAL_TO_REVISION,
      NULL, NULL },
    { NULL }
};

int c64_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
