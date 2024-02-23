/*
 * cbm2model.c - CBM2 model detection and setting.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

/* #define DEBUG_MODEL */

#ifdef DEBUG_MODEL
#define DBG(_x_)        log_error _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "cbm2-resources.h"
#include "cbm2cart.h"
#include "cbm2mem.h"
#include "cbm2model.h"
#include "cbm2rom.h"
#include "cia.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "types.h"

struct model_s {
    int video;   /* machine video timing */
    int hasvicii;
    int ramsize;
    const char *basicname;
    const char *chargenname;
    const char *kernalname;
    int line; /* 0=7x0 (50 Hz), 1=6x0 60Hz, 2=6x0 50Hz */
};

static const struct model_s cbm2models[] = {
    { MACHINE_SYNC_PAL,  HAS_VICII,  64, CBM2_BASIC500_NAME, CBM2_CHARGEN500_NAME, CBM2_KERNAL500_NAME, LINE_6x0_50HZ  }, /* 510 */
    { MACHINE_SYNC_NTSC, HAS_VICII,  64, CBM2_BASIC500_NAME, CBM2_CHARGEN500_NAME, CBM2_KERNAL500_NAME, LINE_6x0_60HZ  }, /* 510 */
    { MACHINE_SYNC_PAL,  HAS_CRTC,  128, CBM2_BASIC128_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_50HZ  }, /* 610 */
    { MACHINE_SYNC_NTSC, HAS_CRTC,  128, CBM2_BASIC128_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_60HZ  }, /* 610 */
    { MACHINE_SYNC_PAL,  HAS_CRTC,  256, CBM2_BASIC256_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_50HZ  }, /* 620 */
    { MACHINE_SYNC_NTSC, HAS_CRTC,  256, CBM2_BASIC256_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_60HZ  }, /* 620 */
    { MACHINE_SYNC_PAL,  HAS_CRTC, 1024, CBM2_BASIC256_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_50HZ  }, /* 620+ */
    { MACHINE_SYNC_NTSC, HAS_CRTC, 1024, CBM2_BASIC256_NAME, CBM2_CHARGEN600_NAME, CBM2_KERNAL_NAME,    LINE_6x0_60HZ  }, /* 620+ */
    { MACHINE_SYNC_NTSC, HAS_CRTC,  128, CBM2_BASIC128_NAME, CBM2_CHARGEN700_NAME, CBM2_KERNAL_NAME,    LINE_7x0_50HZ  }, /* 710 */
    { MACHINE_SYNC_NTSC, HAS_CRTC,  256, CBM2_BASIC256_NAME, CBM2_CHARGEN700_NAME, CBM2_KERNAL_NAME,    LINE_7x0_50HZ  }, /* 720 */
    { MACHINE_SYNC_NTSC, HAS_CRTC, 1024, CBM2_BASIC256_NAME, CBM2_CHARGEN700_NAME, CBM2_KERNAL_NAME,    LINE_7x0_50HZ  }, /* 720+ */
};

/* ------------------------------------------------------------------------- */
static int cbm2model_get_temp(int video, int ramsz, int hasvicii, int line,
                              int sid, int cia)
{
    int i;

    for (i = 0; i < CBM2MODEL_NUM; ++i) {
        if ((cbm2models[i].video == video)
            && (cbm2models[i].ramsize == ramsz)
            && (cbm2models[i].hasvicii == hasvicii)
            && (cbm2models[i].line == line)
            && (sid == SID_MODEL_6581)
            && (cia == CIA_MODEL_6526)) {
            return i;
        }
    }

    return CBM2MODEL_UNKNOWN;
}

int cbm2model_get(void)
{
    int video, ramsz, hasvicii, line, sid, cia;

    hasvicii = (machine_class == VICE_MACHINE_CBM5x0);

    if ((resources_get_int("MachineVideoStandard", &video) < 0)
        || (resources_get_int("RamSize", &ramsz) < 0)
        || (resources_get_int("ModelLine", &line) < 0)
        || (resources_get_int("CIA1Model", &cia) < 0)
        || (resources_get_int("SidModel", &sid) < 0)) {
        return -1;
    }

    return cbm2model_get_temp(video, ramsz, hasvicii, line, sid, cia);
}

#if 0
static void cbm2model_set_temp(int model, int *video_sync, int *ramsize, int *hasvicii, int *line)
{
    int old_model;

    old_model = cbm2model_get_temp(*video_sync, *ramsize, *hasvicii, *line);

    if ((model == old_model) || (model == CBM2MODEL_UNKNOWN)) {
        return;
    }

    *video_sync = cbm2models[model].video;
    *ramsize = cbm2models[model].ramsize;
    *line = cbm2models[model].line;
    *hasvicii = cbm2models[model].hasvicii;
}
#endif

void cbm2model_set(int model)
{
    int old_model;
    int pf;

    old_model = cbm2model_get();

    if ((model == old_model) || (model == CBM2MODEL_UNKNOWN)) {
        return;
    }

    DBG(("cbm2model_set (%d)", model));

    resources_set_int("ModelLine", cbm2models[model].line);
    resources_set_int("MachineVideoStandard", cbm2models[model].video);
    /* Determine the power net frequency for this model. */
    switch(cbm2models[model].video) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            pf = 50;
            break;
        default:
            pf = 60;
            break;
    }
    resources_set_int("MachinePowerFrequency", pf);
    resources_set_int("RamSize", cbm2models[model].ramsize);

    resources_set_string("KernalName", cbm2models[model].kernalname);
    resources_set_string("BasicName", cbm2models[model].basicname);
    resources_set_string("ChargenName", cbm2models[model].chargenname);
}
