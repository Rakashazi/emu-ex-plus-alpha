/*
 * sid-resources.c - SID resources.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "catweaselmkiii.h"
#include "hardsid.h"
#include "log.h"
#include "machine.h"
#ifdef HAVE_PARSID
#include "parsid.h"
#endif
#include "resources.h"
#include "sid-resources.h"
#include "sid.h"
#include "ssi2001.h"
#include "sound.h"
#include "types.h"

/* Resource handling -- Added by Ettore 98-04-26.  */

/* FIXME: We need sanity checks!  And do we really need all of these
   `close_sound()' calls?  */

/* #define SID_ENGINE_MODEL_DEBUG */

static int sid_filters_enabled;       /* app_resources.sidFilters */
static int sid_model;                 /* app_resources.sidModel */
#if defined(HAVE_RESID)
static int sid_resid_sampling;
static int sid_resid_passband;
static int sid_resid_gain;
static int sid_resid_filter_bias;
static int sid_resid_8580_passband;
static int sid_resid_8580_gain;
static int sid_resid_8580_filter_bias;
#endif
int sid_stereo = 0;
int checking_sid_stereo;
unsigned int sid2_address_start;
unsigned int sid2_address_end;
unsigned int sid3_address_start;
unsigned int sid3_address_end;
unsigned int sid4_address_start;
unsigned int sid4_address_end;
unsigned int sid5_address_start;
unsigned int sid5_address_end;
unsigned int sid6_address_start;
unsigned int sid6_address_end;
unsigned int sid7_address_start;
unsigned int sid7_address_end;
unsigned int sid8_address_start;
unsigned int sid8_address_end;
static int sid_engine;
#ifdef HAVE_HARDSID
static int sid_hardsid_main;
static int sid_hardsid_right;
#endif

static int set_sid_engine(int set_engine, void *param)
{
    int engine = set_engine;

    if (engine == SID_ENGINE_DEFAULT) {
#ifdef HAVE_RESID
        engine = SID_ENGINE_RESID;
#else
        engine = SID_ENGINE_FASTSID;
#endif
    }

    switch (engine) {
#ifdef HAVE_FASTSID
        case SID_ENGINE_FASTSID:
#endif
#ifdef HAVE_RESID
        case SID_ENGINE_RESID:
#endif
#ifdef HAVE_CATWEASELMKIII
        case SID_ENGINE_CATWEASELMKIII:
#endif
#ifdef HAVE_HARDSID
        case SID_ENGINE_HARDSID:
#endif
#ifdef HAVE_PARSID
        case SID_ENGINE_PARSID:
#endif
#ifdef HAVE_SSI2001
        case SID_ENGINE_SSI2001:
#endif
            break;
        default:
            return -1;
    }

    if (sid_engine_set(engine) < 0) {
        return -1;
    }

    sid_engine = engine;

#ifdef SID_ENGINE_MODEL_DEBUG
    log_debug("SID engine set to %d", engine);
#endif
    sound_state_changed = 1;

    return 0;
}

static int set_sid_filters_enabled(int val, void *param)
{
    sid_filters_enabled = val ? 1 : 0;

    sid_state_changed = 1;

    return 0;
}

static int set_sid_stereo(int val, void *param)
{
    if ((machine_class == VICE_MACHINE_C64DTV) ||
        (machine_class == VICE_MACHINE_VIC20) ||
        (machine_class == VICE_MACHINE_PLUS4) ||
        (machine_class == VICE_MACHINE_PET) ||
        (machine_class == VICE_MACHINE_CBM5x0) ||
        (machine_class == VICE_MACHINE_CBM6x0)) {
        sid_stereo = 0;
    } else {
        if (val != sid_stereo) {
            if (val < 0 || val > (SOUND_SIDS_MAX - 1)) {
                return -1;
            }
            sid_stereo = val;
            sound_state_changed = 1;
            machine_sid2_enable(val);
        }
    }
    return 0;
}

#define SET_SIDx_ADDRESS(sid_nr)                                        \
    int sid_set_sid##sid_nr##_address(int val, void *param)             \
    {                                                                   \
        unsigned int sid_adr;                                           \
                                                                        \
        sid_adr = (unsigned int)val;                                    \
                                                                        \
        if (machine_sid##sid_nr##_check_range(sid_adr) < 0) {           \
            return -1;                                                  \
        }                                                               \
                                                                        \
        sid##sid_nr##_address_start = sid_adr;                          \
        sid##sid_nr##_address_end = sid##sid_nr##_address_start + 0x20; \
        return 0;                                                       \
    }

SET_SIDx_ADDRESS(2)
SET_SIDx_ADDRESS(3)
SET_SIDx_ADDRESS(4)
SET_SIDx_ADDRESS(5)
SET_SIDx_ADDRESS(6)
SET_SIDx_ADDRESS(7)
SET_SIDx_ADDRESS(8)

static int set_sid_model(int val, void *param)
{
    sid_model = val;

    if (sid_model == SID_MODEL_DEFAULT) {
        sid_model = SID_MODEL_6581;
#ifdef HAVE_RESID
        if (machine_class == VICE_MACHINE_C64DTV) {
            sid_model = SID_MODEL_DTVSID;
        } else
#endif
        if ((machine_class == VICE_MACHINE_C128) || 
            (machine_class == VICE_MACHINE_C64) ||
            (machine_class == VICE_MACHINE_C64SC) ||
            (machine_class == VICE_MACHINE_SCPU64)){
            sid_model = SID_MODEL_8580;
        }
    }

    switch (sid_model) {
        case SID_MODEL_6581:
        case SID_MODEL_8580:
        case SID_MODEL_8580D:
#ifdef HAVE_RESID
        case SID_MODEL_DTVSID:
#endif
            break;
        default:
            return -1;
    }

#ifdef SID_ENGINE_MODEL_DEBUG
    log_debug("SID model set to %d", sid_model);
#endif
    sid_state_changed = 1;
    return 0;
}

#if defined(HAVE_RESID) || defined(HAVE_RESID_DTV)
static int set_sid_resid_sampling(int val, void *param)
{
    switch (val) {
        case SID_RESID_SAMPLING_FAST:
        case SID_RESID_SAMPLING_INTERPOLATION:
        case SID_RESID_SAMPLING_RESAMPLING:
        case SID_RESID_SAMPLING_FAST_RESAMPLING:
            break;
        default:
            return -1;
    }

    sid_resid_sampling = val;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_passband(int i, void *param)
{
    if (i < 0) {
        i = 0;
    } else if (i > 90) {
        i = 90;
    }

    sid_resid_passband = i;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_gain(int i, void *param)
{
    if (i < 90) {
        i = 90;
    } else if (i > 100) {
        i = 100;
    }

    sid_resid_gain = i;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_filter_bias(int i, void *param)
{
    if (i < -5000) {
        i = -5000;
    } else if (i > 5000) {
        i = 5000;
    }

    sid_resid_filter_bias = i;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_8580_passband(int i, void *param)
{
    if (i < 0) {
        i = 0;
    } else if (i > 90) {
        i = 90;
    }

    sid_resid_8580_passband = i;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_8580_gain(int i, void *param)
{
    if (i < 90) {
        i = 90;
    } else if (i > 100) {
        i = 100;
    }

    sid_resid_8580_gain = i;
    sid_state_changed = 1;
    return 0;
}

static int set_sid_resid_8580_filter_bias(int i, void *param)
{
    if (i < -5000) {
        i = -5000;
    } else if (i > 5000) {
        i = 5000;
    }

    sid_resid_8580_filter_bias = i;
    sid_state_changed = 1;
    return 0;
}

#endif

#ifdef HAVE_HARDSID
static int set_sid_hardsid_main(int val, void *param)
{
    sid_hardsid_main = (unsigned int)val;
    hardsid_set_device(0, sid_hardsid_main);

    return 0;
}

static int set_sid_hardsid_right(int val, void *param)
{
    sid_hardsid_right = (unsigned int)val;
    hardsid_set_device(1, sid_hardsid_right);

    return 0;
}
#endif

#ifdef HAVE_RESID
static int sid_enabled = 1;

void sid_set_enable(int value)
{
    int val = value ? 1 : 0;

    if (val == sid_enabled) {
        return;
    }

#ifdef HAVE_FASTSID
    if (val) {
        sid_engine_set(SID_ENGINE_FASTSID);
    } else 
#endif
    {
        sid_engine_set(sid_engine);
    }
    sid_enabled = val;
    sid_state_changed = 1;
}
#endif

#if defined(HAVE_RESID) || defined(HAVE_RESID_DTV)
static const resource_int_t resid_resources_int[] = {
    { "SidResidSampling", SID_RESID_SAMPLING_RESAMPLING, RES_EVENT_NO, NULL,
      &sid_resid_sampling, set_sid_resid_sampling, NULL },
    { "SidResidPassband", 90, RES_EVENT_NO, NULL,
      &sid_resid_passband, set_sid_resid_passband, NULL },
    { "SidResidGain", 97, RES_EVENT_NO, NULL,
      &sid_resid_gain, set_sid_resid_gain, NULL },
    { "SidResidFilterBias", 500, RES_EVENT_NO, NULL,
      &sid_resid_filter_bias, set_sid_resid_filter_bias, NULL },
    { "SidResid8580Passband", 90, RES_EVENT_NO, NULL,
      &sid_resid_8580_passband, set_sid_resid_8580_passband, NULL },
    { "SidResid8580Gain", 97, RES_EVENT_NO, NULL,
      &sid_resid_8580_gain, set_sid_resid_8580_gain, NULL },
#ifdef HAVE_NEW_8580_FILTER
    { "SidResid8580FilterBias", -3000, RES_EVENT_NO, NULL,
      &sid_resid_8580_filter_bias, set_sid_resid_8580_filter_bias, NULL },
#else
    { "SidResid8580FilterBias", 0, RES_EVENT_NO, NULL,
      &sid_resid_8580_filter_bias, set_sid_resid_8580_filter_bias, NULL },
#endif
    RESOURCE_INT_LIST_END
};
#endif

static const resource_int_t common_resources_int[] = {
#ifdef HAVE_RESID
    { "SidEngine", SID_ENGINE_DEFAULT,
      RES_EVENT_STRICT, (resource_value_t)SID_ENGINE_RESID,
      &sid_engine, set_sid_engine, NULL },
#else
    { "SidEngine", SID_ENGINE_DEFAULT,
      RES_EVENT_STRICT, (resource_value_t)SID_ENGINE_FASTSID,
      &sid_engine, set_sid_engine, NULL },
#endif
    { "SidFilters", 1, RES_EVENT_SAME, NULL,
      &sid_filters_enabled, set_sid_filters_enabled, NULL },
    { "SidModel", SID_MODEL_DEFAULT, RES_EVENT_SAME, NULL,
      &sid_model, set_sid_model, NULL },
    RESOURCE_INT_LIST_END
};

#ifdef HAVE_HARDSID
static const resource_int_t hardsid_resources_int[] = {
    { "SidHardSIDMain", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &sid_hardsid_main, set_sid_hardsid_main, NULL },
    { "SidHardSIDRight", 1, RES_EVENT_NO, NULL,
      &sid_hardsid_right, set_sid_hardsid_right, NULL },
    RESOURCE_INT_LIST_END
};
#endif

static const resource_int_t stereo_resources_int[] = {
    { "SidStereo", 0, RES_EVENT_SAME, NULL,
      &sid_stereo, set_sid_stereo, NULL },
    RESOURCE_INT_LIST_END
};

int sid_common_resources_init(void)
{
#ifdef HAVE_HARDSID
    if (hardsid_available()) {
        if (resources_register_int(hardsid_resources_int) < 0) {
            return -1;
        }
    }
#endif
    return resources_register_int(common_resources_int);
}

int sid_resources_init(void)
{
#if defined(HAVE_RESID) || defined(HAVE_RESID_DTV)
    if (resources_register_int(resid_resources_int) < 0) {
        return -1;
    }
#endif

    if (resources_register_int(stereo_resources_int) < 0) {
        return -1;
    }

    return sid_common_resources_init();
}

/* ------------------------------------------------------------------------- */

#ifdef SID_SETTINGS_DIALOG
static sid_engine_model_t *sid_engine_model_list[22];
static int num_sid_engine_models;

#ifdef HAVE_RESID_DTV
static sid_engine_model_t sid_engine_models_resid_dtv[] = {
    { "DTVSID (ReSID-DTV)", SID_RESID_DTVSID },
    { NULL, -1 }
};
#endif

#ifdef HAVE_FASTSID
#ifdef HAVE_RESID
static sid_engine_model_t sid_engine_models_fastsid[] = {
    { "6581 (Fast SID)", SID_FASTSID_6581 },
    { "8580 (Fast SID)", SID_FASTSID_8580 },
    { NULL, -1 }
};
#else
static sid_engine_model_t sid_engine_models_fastsid[] = {
    { "6581", SID_FASTSID_6581 },
    { "8580", SID_FASTSID_8580 },
    { NULL, -1 }
};
#endif
#endif

#ifdef HAVE_RESID
#ifdef HAVE_FASTSID
static sid_engine_model_t sid_engine_models_resid[] = {
    { "6581 (ReSID)", SID_RESID_6581 },
    { "8580 (ReSID)", SID_RESID_8580 },
    { "8580 + digi boost (ReSID)", SID_RESID_8580D },
    { NULL, -1 }
};
#else
static sid_engine_model_t sid_engine_models_resid[] = {
    { "6581", SID_RESID_6581 },
    { "8580", SID_RESID_8580 },
    { "8580 + digi boost", SID_RESID_8580D },
    { NULL, -1 }
};
#endif
#endif

#ifdef HAVE_CATWEASELMKIII
static sid_engine_model_t sid_engine_models_catweaselmkiii[] = {
    { "Catweasel MK3", SID_CATWEASELMKIII },
    { NULL, -1 }
};
#endif

#ifdef HAVE_HARDSID
static sid_engine_model_t sid_engine_models_hardsid[] = {
    { "HardSID", SID_HARDSID },
    { NULL, -1 }
};
#endif

#ifdef HAVE_PARSID
static sid_engine_model_t sid_engine_models_parsid[] = {
    { "ParSID", SID_PARSID },
    { NULL, -1 }
};
#endif

#ifdef HAVE_SSI2001
static sid_engine_model_t sid_engine_models_ssi2001[] = {
    { "SSI2001", SID_SSI2001 },
    { NULL, -1 }
};
#endif

static void add_sid_engine_models(sid_engine_model_t *sid_engine_models)
{
    int i = 0;

    while (sid_engine_models[i].name) {
        sid_engine_model_list[num_sid_engine_models++] = &sid_engine_models[i++];
    }
}

sid_engine_model_t **sid_get_engine_model_list(void)
{
    num_sid_engine_models = 0;

#ifdef HAVE_RESID_DTV
    if (machine_class == VICE_MACHINE_C64DTV) {
        add_sid_engine_models(sid_engine_models_resid_dtv);
    }
#endif

#ifdef HAVE_FASTSID
    add_sid_engine_models(sid_engine_models_fastsid);
#endif

#ifdef HAVE_RESID
    /* Should we have if (machine_class != VICE_MACHINE_C64DTV) here? */
    add_sid_engine_models(sid_engine_models_resid);
#endif

#ifdef HAVE_CATWEASELMKIII
    if (catweaselmkiii_available()) {
        add_sid_engine_models(sid_engine_models_catweaselmkiii);
    }
#endif

#ifdef HAVE_HARDSID
    if (hardsid_available()) {
        add_sid_engine_models(sid_engine_models_hardsid);
    }
#endif

#ifdef HAVE_PARSID
    if (parsid_available()) {
        add_sid_engine_models(sid_engine_models_parsid);
    }
#endif

#ifdef HAVE_SSI2001
    if (ssi2001_available()) {
        add_sid_engine_models(sid_engine_models_ssi2001);
    }
#endif

    sid_engine_model_list[num_sid_engine_models] = NULL;

    return sid_engine_model_list;
}
#endif /* SID_SETTINGS_DIALOG */

static int sid_check_engine_model(int engine, int model)
{
    switch (engine) {
        case SID_ENGINE_CATWEASELMKIII:
        case SID_ENGINE_HARDSID:
        case SID_ENGINE_PARSID:
        case SID_ENGINE_SSI2001:
            return 0;
        default:
            break;
    }

    switch (engine << 8 | model) {
#ifdef HAVE_FASTSID
        case SID_FASTSID_6581:
        case SID_FASTSID_8580:
#endif
#ifdef HAVE_RESID
        case SID_RESID_6581:
        case SID_RESID_8580:
        case SID_RESID_8580D:
#endif
            return 0;
#ifdef HAVE_RESID_DTV
        case SID_RESID_DTVSID:
            if (machine_class == VICE_MACHINE_C64DTV) {
                return 0;
            } else {
                return -1;
            }
#endif
        default:
            return -1;
    }
}

int sid_set_engine_model(int engine, int model)
{
    if (sid_check_engine_model(engine, model) < 0) {
        return -1;
    }
    resources_set_int("SidEngine", engine);
    resources_set_int("SidModel", model);

    return 0;
}
