/*
 * scpu64model.c - SCPU64 model detection and setting.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *  groepaz <groepaz@gmx.net>
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

#include <string.h>

#include "c64-resources.h"
#include "c64model.h"
#include "cia.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "types.h"
#include "vicii.h"

/******************************************************************************/

#define CIA_MODEL_DEFAULT_OLD CIA_MODEL_6526
#define CIA_MODEL_DEFAULT_NEW CIA_MODEL_6526A

static int scpu64model_get_temp(int vicii_model, int sid_model, int glue_logic,
                                int cia1_model, int cia2_model, int new_luma, int iecreset,
                                const char *chargen);
static void scpu64model_set_temp(int model, int *vicii_model, int *sid_model,
                                 int *glue_logic, int *cia1_model, int *cia2_model,
                                 int *new_luma, int *iecreset,
                                 const char *chargen);

/******************************************************************************/

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

struct model_s {
    int vicii;      /* VIC-II model */
    int video;      /* machine video timing */
    int luma;       /* old or new */
    int cia;        /* old or new */
    int glue;       /* discrete or ASIC */
    int sid;        /* old or new */
    int iecreset;
    int iec;
    int userport;
    int keyboard;
    char *chargenname;
};

static struct model_s scpu64models[] = {

    /* C64 PAL / PET64 PAL */
    { VICII_MODEL_6569, MACHINE_SYNC_PAL, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64C PAL */
    { VICII_MODEL_8565, MACHINE_SYNC_PAL, NEW_LUMA,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, IEC_HARD_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64 OLD PAL */
    { VICII_MODEL_6569R1, MACHINE_SYNC_PAL, OLD_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64 NTSC / PET64 NTSC */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64C NTSC */
    { VICII_MODEL_8562, MACHINE_SYNC_NTSC, NEW_LUMA,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, IEC_HARD_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64 OLD NTSC */
    { VICII_MODEL_6567R56A, MACHINE_SYNC_NTSCOLD, OLD_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64 PAL-N */
    { VICII_MODEL_6572, MACHINE_SYNC_PALN, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* SX64 PAL, FIXME: guessed */
    { VICII_MODEL_6569, MACHINE_SYNC_PAL, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* SX64 NTSC, FIXME: guessed */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "chargen" },

    /* C64 Japanese, FIXME: guessed */
    { VICII_MODEL_6567, MACHINE_SYNC_NTSC, NEW_LUMA,
      CIA_MODEL_DEFAULT_OLD, GLUE_DISCRETE, OLD_SID, IEC_SOFT_RESET,
      HAS_IEC, HAS_USERPORT, HAS_KEYBOARD, "jpchrgen" },

    /* C64 GS, FIXME: guessed */
    { VICII_MODEL_8565, MACHINE_SYNC_PAL, NEW_LUMA,
      CIA_MODEL_DEFAULT_NEW, GLUE_CUSTOM_IC, NEW_SID, IEC_HARD_RESET,
      NO_IEC, NO_USERPORT, NO_KEYBOARD, "chargen" },

    /* end of list */
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, NULL },
};

/* ------------------------------------------------------------------------- */

static int scpu64model_get_temp(int vicii_model, int sid_model, int glue_logic,
                                int cia1_model, int cia2_model, int new_luma,
                                int iecreset, const char *chargen)
{
    int new_sid;
    int new_cia;
    int i;

    if (cia1_model != cia2_model) {
        return C64MODEL_UNKNOWN;
    }

    new_sid = is_new_sid(sid_model);
    new_cia = is_new_cia(cia1_model);

    for (i = 0; scpu64models[i].chargenname != NULL; ++i) {
        if ((scpu64models[i].vicii == vicii_model)
            && (scpu64models[i].luma == new_luma)
            && (is_new_cia(scpu64models[i].cia) == new_cia)
            && (scpu64models[i].glue == glue_logic)
            && (scpu64models[i].sid == new_sid)
            && (scpu64models[i].iecreset == iecreset)
            && (!strcmp(scpu64models[i].chargenname, chargen))) {
            return i;
        }
    }

    return C64MODEL_UNKNOWN;
}

/* get model from details */
int c64model_get_model(c64model_details_t *details)
{
    return scpu64model_get_temp(details->vicii_model, details->sid_model, details->glue_logic,
                                details->cia1_model, details->cia2_model, details->new_luma, 
                                details->iecreset, details->chargen);
}

int c64model_get(void)
{
    int vicii_model, sid_model, glue_logic, cia1_model, cia2_model, new_luma, iecreset;
    char c[0x10];
    const char *chargen = c;

    if ((resources_get_int("VICIIModel", &vicii_model) < 0)
        || (resources_get_int("SidModel", &sid_model) < 0)
        || (resources_get_int("GlueLogic", &glue_logic) < 0)
        || (resources_get_int("CIA1Model", &cia1_model) < 0)
        || (resources_get_int("CIA2Model", &cia2_model) < 0)
        || (resources_get_int("VICIINewLuminances", &new_luma) < 0)
        || (resources_get_int("IECReset", &iecreset) < 0)
        || (resources_get_string("ChargenName", &chargen) < 0)) {
        return -1;
    }

    return scpu64model_get_temp(vicii_model, sid_model, glue_logic,
                                cia1_model, cia2_model, new_luma, iecreset,
                                chargen);
}

static void scpu64model_set_temp(int model, int *vicii_model, int *sid_model,
                                 int *glue_logic, int *cia1_model, int *cia2_model,
                                 int *new_luma, int *iecreset,
                                 const char *chargen)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int new_sid_model;
    int old_type;
    int new_type;

    old_model = scpu64model_get_temp(*vicii_model, *sid_model, *glue_logic,
                                     *cia1_model, *cia2_model, *new_luma, 
                                     *iecreset, chargen);

    if ((model == old_model) || (model == C64MODEL_UNKNOWN)) {
        return;
    }

    *vicii_model = scpu64models[model].vicii;
    *cia1_model = scpu64models[model].cia;
    *cia2_model = scpu64models[model].cia;
    *glue_logic = scpu64models[model].glue;
    *new_luma = scpu64models[model].luma;
    *iecreset = scpu64models[model].iecreset;

    /* Only change the SID model if the model changes from 6581 to 8580.
       This allows to switch between "pal"/"oldpal" without changing
       the specific SID model. The current engine is preserved. */
    old_engine = (*sid_model >> 8);
    old_sid_model = (*sid_model & 0xff);

    new_sid_model = scpu64models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        *sid_model = (old_engine << 8) | new_sid_model;
    }
}

/* get details for model */
void c64model_set_details(c64model_details_t *details, int model)
{
    scpu64model_set_temp(model, &details->vicii_model, &details->sid_model,
                       &details->glue_logic, &details->cia1_model, &details->cia2_model,
                       &details->new_luma, &details->iecreset, details->chargen);
}

void c64model_set(int model)
{
    int old_model;
    int old_engine;
    int old_sid_model;
    int old_type;
    int new_sid_model;
    int new_type;

    old_model = c64model_get();

    if ((model == old_model) || (model == C64MODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("VICIIModel", scpu64models[model].vicii);
    resources_set_int("CIA1Model", scpu64models[model].cia);
    resources_set_int("CIA2Model", scpu64models[model].cia);
    resources_set_int("GlueLogic", scpu64models[model].glue);
    resources_set_int("IECReset", scpu64models[model].iecreset);

    resources_set_string("ChargenName", scpu64models[model].chargenname);

    /* Only change the SID model if the model changes from 6581 to 8580
       or the specific SID type changes if residfp is used. This allows
       to switch between "pal"/"oldpal" without changing the specific
       SID model. The current engine is preserved. */
    resources_get_int("SidEngine", &old_engine);
    resources_get_int("SidModel", &old_sid_model);
    new_sid_model = scpu64models[model].sid;

    old_type = is_new_sid(old_sid_model);
    new_type = is_new_sid(new_sid_model);

    if (old_type != new_type) {
        sid_set_engine_model(old_engine, new_sid_model);
    }
}
