/*
 * c128model.c - C64 model detection and setting.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#include "vice.h"

#include "c128-resources.h"
#include "c128model.h"
#include "cia.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "types.h"
#include "vicii.h"
#include "vdctypes.h"

#define CIA_MODEL_DEFAULT_OLD CIA_MODEL_6526
#define CIA_MODEL_DEFAULT_NEW CIA_MODEL_6526A

static int is_new_sid(int model)
{
    switch (model) {
        case SID_MODEL_6581:
        case SID_MODEL_6581R4:
        default:
            return 0;

        case SID_MODEL_8580:
        case SID_MODEL_8580D:
            return 1;
    }
}
static int is_new_cia(int model)
{
    switch (model) {
        case CIA_MODEL_6526:
        default:
            return 0;

        case CIA_MODEL_6526A:
            return 1;
    }
}

/*
C128:             SID 6581, VDC 8563, 16 KB VDC RAM.
C128D (plastic):  SID 6581, VDC 8563, 16 KB VDC RAM (like C128+1571)
C128D (metal):    SID 8580, VDC 8568, 64 KB VDC RAM, new BASIC ROM?, new floppy ROM?
C128DCR (like above)
 */

struct model_s {
    int video;   /* machine video timing */
    int cia;     /* old or new */
    int sid;     /* old or new */
    int vdc;     /* old or new */
    int vdc64k;
};

static struct model_s c128models[] = {
    { MACHINE_SYNC_PAL,  OLD_CIA, OLD_SID, VDC_REVISION_1, VDC16K },
    { MACHINE_SYNC_PAL,  NEW_CIA, NEW_SID, VDC_REVISION_2, VDC64K },
    { MACHINE_SYNC_NTSC, OLD_CIA, OLD_SID, VDC_REVISION_1, VDC16K },
    { MACHINE_SYNC_NTSC, NEW_CIA, NEW_SID, VDC_REVISION_2, VDC64K },
};

/* ------------------------------------------------------------------------- */

static int c128model_get_temp(int video, int sid_model, int vdc_revision, int vdc_64k,
                       int cia1_model, int cia2_model)
{
    int new_sid;
    int new_cia;
    int i;

    if (cia1_model != cia2_model) {
        return C128MODEL_UNKNOWN;
    }

    new_sid = is_new_sid(sid_model);
    new_cia = is_new_cia(cia1_model);

    for (i = 0; i < C128MODEL_NUM; ++i) {
        if ((c128models[i].video == video)
            && (c128models[i].vdc == vdc_revision)
            && (c128models[i].vdc64k == vdc_64k)
            && (c128models[i].cia == new_cia)
            && (c128models[i].sid == new_sid)) {
            return i;
        }
    }

    return C128MODEL_UNKNOWN;
}

int c128model_get(void)
{
    int video, sid_model, cia1_model, cia2_model, vdc_revision, vdc_64k;

    if ((resources_get_int("MachineVideoStandard", &video) < 0)
        || (resources_get_int("SidModel", &sid_model) < 0)
        || (resources_get_int("CIA1Model", &cia1_model) < 0)
        || (resources_get_int("CIA2Model", &cia2_model) < 0)
        || (resources_get_int("VDCRevision", &vdc_revision) < 0)
        || (resources_get_int("VDC64KB", &vdc_64k) < 0)) {
        return -1;
    }

    return c128model_get_temp(video, sid_model, vdc_revision, vdc_64k,
                              cia1_model, cia2_model);
}

#if 0
static void c128model_set_temp(int model, int *vicii_model, int *sid_model,
                        int *vdc_revision, int *vdc_64k, int *cia1_model,
                        int *cia2_model)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int new_sid_model;
    int old_type;
    int new_type;

    old_model = c128model_get_temp(*vicii_model, *sid_model, *vdc_revision,
                                   *vdc_64k, *cia1_model, *cia2_model);

    if ((model == old_model) || (model == C128MODEL_UNKNOWN)) {
        return;
    }

    *vicii_model = c128models[model].video;
    *cia1_model = c128models[model].cia;
    *cia2_model = c128models[model].cia;
    *vdc_revision = c128models[model].vdc;
    *vdc_64k = c128models[model].vdc64k;

    /* Only change the SID model if the model changes from 6581 to 8580.
       This allows to switch between "pal"/"oldpal" without changing
       the specific SID model. The current engine is preserved. */
    old_engine = (*sid_model >> 8);
    old_sid_model = (*sid_model & 0xff);

    new_sid_model = c128models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        *sid_model = (old_engine << 8 ) | new_sid_model;
    }
}
#endif

void c128model_set(int model)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int old_type;
    int new_sid_model;
    int new_type;

    old_model = c128model_get();

    if ((model == old_model) || (model == C128MODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("MachineVideoStandard", c128models[model].video);
    resources_set_int("CIA1Model", c128models[model].cia);
    resources_set_int("CIA2Model", c128models[model].cia);
    resources_set_int("VICIINewLuminances", 1);
    resources_set_int("VDCRevision", c128models[model].vdc);
    resources_set_int("VDC64KB", c128models[model].vdc64k);

    /* Only change the SID model if the model changes from 6581 to 8580
       This allows to switch between "pal"/"oldpal" without changing
       the specific SID model. The current engine is preserved. */

    resources_get_int("SidEngine", &old_engine);
    resources_get_int("SidModel", &old_sid_model);
    new_sid_model = c128models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        sid_set_engine_model(old_engine, new_sid_model);
    }
}
