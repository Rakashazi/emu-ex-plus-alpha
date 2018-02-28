/*
 * sid-cmdline-options.c - SID command line options.
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
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "hardsid.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "translate.h"
#include "util.h"

static char *sid_address_range = NULL;

struct engine_s {
    const char *name;
    int engine;
};

static struct engine_s engine_match[] = {
    { "0", SID_FASTSID_6581 },
    { "fast", SID_FASTSID_6581 },
    { "fastold", SID_FASTSID_6581 },
    { "fast6581", SID_FASTSID_6581 },
    { "1", SID_FASTSID_8580 },
    { "fastnew", SID_FASTSID_8580 },
    { "fast8580", SID_FASTSID_8580 },
#ifdef HAVE_RESID
    { "256", SID_RESID_6581 },
    { "resid", SID_RESID_6581 },
    { "residold", SID_RESID_6581 },
    { "resid6581", SID_RESID_6581 },
    { "257", SID_RESID_8580 },
    { "residnew", SID_RESID_8580 },
    { "resid8580", SID_RESID_8580 },
    { "258", SID_RESID_8580D },
    { "residdigital", SID_RESID_8580D },
    { "residd", SID_RESID_8580D },
    { "residnewd", SID_RESID_8580D },
    { "resid8580d", SID_RESID_8580D },
    { "260", SID_RESID_DTVSID },
    { "dtv", SID_RESID_DTVSID },
    { "c64dtv", SID_RESID_DTVSID },
    { "dtvsid", SID_RESID_DTVSID },
#endif
#ifdef HAVE_CATWEASELMKIII
    { "512", SID_CATWEASELMKIII },
    { "catweaselmkiii", SID_CATWEASELMKIII },
    { "catweasel3", SID_CATWEASELMKIII },
    { "catweasel", SID_CATWEASELMKIII },
    { "cwmkiii", SID_CATWEASELMKIII },
    { "cw3", SID_CATWEASELMKIII },
    { "cw", SID_CATWEASELMKIII },
#endif
#ifdef HAVE_HARDSID
    { "768", SID_HARDSID },
    { "hardsid", SID_HARDSID },
    { "hard", SID_HARDSID },
    { "hs", SID_HARDSID },
#endif
#ifdef HAVE_PARSID
    { "1024", SID_PARSID },
    { "parsid", SID_PARSID },
    { "par", SID_PARSID },
    { "lpt", SID_PARSID },
#endif
#ifdef HAVE_SSI2001
    { "1280", SID_SSI2001 },
    { "ssi2001", SID_SSI2001 },
    { "ssi", SID_SSI2001 },
#endif
    { NULL, -1 }
};

int sid_common_set_engine_model(const char *param, void *extra_param)
{
    int engine;
    int model;
    int temp = -1;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(engine_match[i].name, param) == 0) {
            temp = engine_match[i].engine;
        }
        i++;
    } while ((temp == -1) && (engine_match[i].name != NULL));

    if (temp == -1) {
        return -1;
    }

    engine = (temp >> 8) & 0xff;
    model = temp & 0xff;

    return sid_set_engine_model(engine, model);
}

static const cmdline_option_t sidengine_cmdline_options[] = {
    { "-sidenginemodel", CALL_FUNCTION, 1,
      sid_common_set_engine_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_ENGINE_MODEL, IDCLS_SPECIFY_SID_ENGINE_MODEL,
      NULL, NULL },
    CMDLINE_LIST_END
};

#ifdef HAVE_RESID
static const cmdline_option_t siddtvengine_cmdline_options[] = {
    { "-sidenginemodel", CALL_FUNCTION, 1,
      sid_common_set_engine_model, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_ENGINE_MODEL, IDCLS_SPECIFY_SIDDTV_ENGINE_MODEL,
      NULL, NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t resid_cmdline_options[] = {
    { "-residsamp", SET_RESOURCE, 1,
      NULL, NULL, "SidResidSampling", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_METHOD, IDCLS_RESID_SAMPLING_METHOD,
      NULL, NULL },
    { "-residpass", SET_RESOURCE, 1,
      NULL, NULL, "SidResidPassband", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PERCENT, IDCLS_PASSBAND_PERCENTAGE,
      NULL, NULL },
    { "-residgain", SET_RESOURCE, 1,
      NULL, NULL, "SidResidGain", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PERCENT, IDCLS_RESID_GAIN_PERCENTAGE,
      NULL, NULL },
    { "-residfilterbias", SET_RESOURCE, 1,
      NULL, NULL, "SidResidFilterBias", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_RESID_FILTER_BIAS,
      NULL, NULL, },
    CMDLINE_LIST_END
};
#endif

#ifdef HAVE_HARDSID
static const cmdline_option_t hardsid_cmdline_options[] = {
    { "-hardsidmain", SET_RESOURCE, 1,
      NULL, NULL, "SidHardSIDMain", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_DEVICE, IDCLS_HARDSID_MAIN,
      NULL, NULL },
    { "-hardsidright", SET_RESOURCE, 1,
      NULL, NULL, "SidHardSIDRight", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_DEVICE, IDCLS_HARDSID_RIGHT,
      NULL, NULL },
    CMDLINE_LIST_END
};
#endif

static cmdline_option_t stereo_cmdline_options[] = {
    { "-sidstereo", SET_RESOURCE, 1,
      NULL, NULL, "SidStereo", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_AMOUNT, IDCLS_AMOUNT_EXTRA_SIDS,
      NULL, NULL },
    { "-sidstereoaddress", SET_RESOURCE, 1,
      NULL, NULL, "SidStereoAddressStart", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_SPECIFY_SID_2_ADDRESS,
      NULL, NULL },
    { "-sidtripleaddress", SET_RESOURCE, 1,
      NULL, NULL, "SidTripleAddressStart", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_SPECIFY_SID_3_ADDRESS,
      NULL, NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t common_cmdline_options[] = {
    { "-sidfilters", SET_RESOURCE, 0,
      NULL, NULL, "SidFilters", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SID_FILTERS,
      NULL, NULL },
    { "+sidfilters", SET_RESOURCE, 0,
      NULL, NULL, "SidFilters", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SID_FILTERS,
      NULL, NULL },
    CMDLINE_LIST_END
};

static char *generate_sid_address_range(void)
{
    char *temp1, *temp2, *temp3;

    temp3 = lib_stralloc(". (");

    temp1 = util_gen_hex_address_list(0xd420, 0xd500, 0x20);
    temp2 = util_concat(temp3, temp1, "/", NULL);
    lib_free(temp3);
    lib_free(temp1);
    temp3 = temp2;

    if (machine_class == VICE_MACHINE_C128) {
        temp1 = util_gen_hex_address_list(0xd700, 0xd800, 0x20);
    } else {
        temp1 = util_gen_hex_address_list(0xd500, 0xd800, 0x20);
    }

    temp2 = util_concat(temp3, temp1, "/", NULL);
    lib_free(temp3);
    lib_free(temp1);
    temp3 = temp2;

    temp1 = util_gen_hex_address_list(0xde00, 0xe000, 0x20);
    temp2 = util_concat(temp3, temp1, ")", NULL);
    lib_free(temp3);
    lib_free(temp1);

    return temp2;
}

int sid_cmdline_options_init(void)
{
#ifdef HAVE_RESID
    if (machine_class == VICE_MACHINE_C64DTV) {
        if (cmdline_register_options(siddtvengine_cmdline_options) < 0) {
            return -1;
        }
    } else {
        if (cmdline_register_options(sidengine_cmdline_options) < 0) {
            return -1;
        }
    }
#else
    if (cmdline_register_options(sidengine_cmdline_options) < 0) {
        return -1;
    }
#endif

#ifdef HAVE_RESID
    if (cmdline_register_options(resid_cmdline_options) < 0) {
        return -1;
    }
#endif

#ifdef HAVE_HARDSID
    if (hardsid_available()) {
        if (cmdline_register_options(hardsid_cmdline_options) < 0) {
            return -1;
        }
    }
#endif

    if ((machine_class != VICE_MACHINE_C64DTV) &&
        (machine_class != VICE_MACHINE_VIC20) &&
        (machine_class != VICE_MACHINE_PLUS4) &&
        (machine_class != VICE_MACHINE_PET) &&
        (machine_class != VICE_MACHINE_CBM5x0) &&
        (machine_class != VICE_MACHINE_CBM6x0)) {

        sid_address_range = generate_sid_address_range();
        stereo_cmdline_options[1].description = sid_address_range;
        stereo_cmdline_options[2].description = sid_address_range;

        if (cmdline_register_options(stereo_cmdline_options) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(common_cmdline_options);
}

void sid_cmdline_options_shutdown(void)
{
    if (sid_address_range) {
        lib_free(sid_address_range);
    }
}
