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
#include "util.h"

#ifdef HAVE_CATWEASELMKIII
#include "catweaselmkiii.h"
#endif

#ifdef HAVE_HARDSID
#include "hardsid.h"
#endif

#ifdef HAVE_PARSID
#include "parsid.h"
#endif

#ifdef HAVE_SSI2001
#include "ssi2001.h"
#endif

static char *sid2_address_range = NULL;
static char *sid3_address_range = NULL;
static char *sid4_address_range = NULL;
static char *sid5_address_range = NULL;
static char *sid6_address_range = NULL;
static char *sid7_address_range = NULL;
static char *sid8_address_range = NULL;

struct engine_s {
    const char *name;
    int engine;
};

static const struct engine_s engine_match[] = {
#ifdef HAVE_FASTSID
    { "0", SID_FASTSID_6581 },
    { "fast", SID_FASTSID_6581 },
    { "fastold", SID_FASTSID_6581 },
    { "fast6581", SID_FASTSID_6581 },
    { "1", SID_FASTSID_8580 },
    { "fastnew", SID_FASTSID_8580 },
    { "fast8580", SID_FASTSID_8580 },
#endif
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

static cmdline_option_t sidengine_cmdline_options[] =
{
    { "-sidenginemodel", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      sid_common_set_engine_model, NULL, NULL, NULL,
      "<engine and model>", NULL },
    CMDLINE_LIST_END
};

#ifdef HAVE_RESID
static cmdline_option_t siddtvengine_cmdline_options[] =
{
    { "-sidenginemodel", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      sid_common_set_engine_model, NULL, NULL, NULL,
      "<engine and model>", NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t resid_cmdline_options[] =
{
    { "-residsamp", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidSampling", NULL,
      "<method>", "reSID sampling method (0: fast, 1: interpolating, 2: resampling, 3: fast resampling)" },
    { "-residpass", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidPassband", NULL,
      "<percent>", "reSID resampling passband in percentage of total bandwidth (0 - 90)" },
    { "-residgain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidGain", NULL,
      "<percent>", "reSID gain in percent (90 - 100)" },
    { "-residfilterbias", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResidFilterBias", NULL,
      "<number>", "reSID filter bias setting, which can be used to adjust DAC bias in millivolts.", },
    { "-resid8580pass", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580Passband", NULL,
      "<percent>", "reSID 8580 resampling passband in percentage of total bandwidth (0 - 90)" },
    { "-resid8580gain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580Gain", NULL,
      "<percent>", "reSID 8580 gain in percent (90 - 100)" },
    { "-resid8580filterbias", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidResid8580FilterBias", NULL,
      "<number>", "reSID 8580 filter bias setting, which can be used to adjust DAC bias in millivolts.", },
    { "-residrawoutput", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidResidEnableRawOutput", (void *)1, NULL, "Enable writing raw reSID output to resid.raw, 16bit little endian data (WARNING: 1MiB per second)." },
    { "+residrawoutput", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidResidEnableRawOutput", (void *)0, NULL, "Disable writing raw reSID output to resid.raw." },
    CMDLINE_LIST_END
};
#endif

#ifdef HAVE_HARDSID
static const cmdline_option_t hardsid_cmdline_options[] =
{
    { "-hardsidmain", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidHardSIDMain", NULL,
      "<device>", "Set the HardSID device for the main SID output" },
    { "-hardsidright", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidHardSIDRight", NULL,
      "<device>", "Set the HardSID device for the right SID output" },
    CMDLINE_LIST_END
};
#endif

static cmdline_option_t stereo_cmdline_options[] =
{
    { "-sidextra", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "SidStereo", NULL,
      "<amount>", "amount of extra SID chips. (0..3)" },
    { "-sid2address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid2AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid3address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid3AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid4address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid4AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid5address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid5AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid6address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid6AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid7address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid7AddressStart", NULL,
      "<Base address>", NULL },
    { "-sid8address", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "Sid8AddressStart", NULL,
      "<Base address>", NULL },
    CMDLINE_LIST_END
};

static const cmdline_option_t common_cmdline_options[] =
{
    { "-sidfilters", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidFilters", (void *)1,
      NULL, "Emulate SID filters" },
    { "+sidfilters", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SidFilters", (void *)0,
      NULL, "Do not emulate SID filters" },
    CMDLINE_LIST_END
};

static char *generate_sid_address_range(int nr)
{
    char *temp1, *temp2, *temp3;

    switch (nr) {
        case 2:
            temp3 = lib_strdup("Specify base address for 2nd SID. (");
            break;
        case 3:
            temp3 = lib_strdup("Specify base address for 3rd SID. (");
            break;
        case 4:
            temp3 = lib_strdup("Specify base address for 4th SID. (");
            break;
        case 5:
            temp3 = lib_strdup("Specify base address for 5th SID. (");
            break;
        case 6:
            temp3 = lib_strdup("Specify base address for 6th SID. (");
            break;
        case 7:
            temp3 = lib_strdup("Specify base address for 7th SID. (");
            break;
        default:
            temp3 = lib_strdup("Specify base address for 8th SID. (");
            break;
    }

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

static char *sid_return = NULL;

static char *build_sid_cmdline_option(int sid_type)
{
    char *old, *new;

    if (sid_return) {
        return sid_return;
    }

    /* start building up the command-line */
    old = lib_strdup("Specify SID engine and model (");

#ifdef HAVE_FASTSID
    /* add fast sid options */
    new = util_concat(old, "0: FastSID 6581, 1: FastSID 8580", NULL);
    lib_free(old);
    old = new;
#endif

#ifdef HAVE_RESID
    /* add resid options if available */
    if (sid_type != SIDTYPE_SIDCART) {
#ifdef HAVE_FASTSID
        new = util_concat(old, ", 256: ReSID 6581, 257: ReSID 8580, 258: ReSID 8580 + digiboost", NULL);
#else
        new = util_concat(old, "256: ReSID 6581, 257: ReSID 8580, 258: ReSID 8580 + digiboost", NULL);
#endif
        lib_free(old);
        old = new;
    }

    /* add residdtv options if available */
    if (sid_type == SIDTYPE_SIDDTV) {
        new = util_concat(old, ", 260: DTVSID", NULL);
        lib_free(old);
        old = new;
    }
#endif

#ifdef HAVE_CATWEASELMKIII
    /* add catweasel options if available */
    if (catweaselmkiii_available()) {
        new = util_concat(old, ", 512: Catweasel", NULL);
        lib_free(old);
        old = new;
    }
#endif

#ifdef HAVE_HARDSID
    /* add hardsid options if available */
    if (hardsid_available()) {
        new = util_concat(old, ", 768: HardSID", NULL);
        lib_free(old);
        old = new;
    }
#endif

#ifdef HAVE_PARSID
    /* add parsid options if available */
    if (parsid_available()) {
        new = util_concat(old, ", 1024: ParSID in par port 1, 1280: ParSID in par port 2, 1536: ParSID in par port 3", NULL);
        lib_free(old);
        old = new;
    }
#endif

#ifdef HAVE_SSI2001
    /* add ssi2001 options if available */
    if (ssi2001_available()) {
        new = util_concat(old, ", 1792: SSI2001", NULL);
        lib_free(old);
        old = new;
    }
#endif

    /* add ending bracket */
    new = util_concat(old, ")", NULL);
    lib_free(old);

    sid_return = new;

    return sid_return;
}

int sid_cmdline_options_init(int sid_type)
{
#ifdef HAVE_RESID
    if (sid_type == SIDTYPE_SIDDTV) {
        siddtvengine_cmdline_options[0].description = build_sid_cmdline_option(SIDTYPE_SIDDTV);
        if (cmdline_register_options(siddtvengine_cmdline_options) < 0) {
            return -1;
        }
    } else {
        sidengine_cmdline_options[0].description = build_sid_cmdline_option(sid_type);
        if (cmdline_register_options(sidengine_cmdline_options) < 0) {
            return -1;
        }
    }
#else
    sidengine_cmdline_options[0].description = build_sid_cmdline_option(sid_type);
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

        sid2_address_range = generate_sid_address_range(2);
        sid3_address_range = generate_sid_address_range(3);
        sid4_address_range = generate_sid_address_range(4);
        sid5_address_range = generate_sid_address_range(5);
        sid6_address_range = generate_sid_address_range(6);
        sid7_address_range = generate_sid_address_range(7);
        sid8_address_range = generate_sid_address_range(8);

        stereo_cmdline_options[1].description = sid2_address_range;
        stereo_cmdline_options[2].description = sid3_address_range;
        stereo_cmdline_options[3].description = sid4_address_range;
        stereo_cmdline_options[4].description = sid5_address_range;
        stereo_cmdline_options[5].description = sid6_address_range;
        stereo_cmdline_options[6].description = sid7_address_range;
        stereo_cmdline_options[7].description = sid8_address_range;


        if (cmdline_register_options(stereo_cmdline_options) < 0) {
            return -1;
        }
    }
    return cmdline_register_options(common_cmdline_options);
}

void sid_cmdline_options_shutdown(void)
{
    if (sid_return) {
        lib_free(sid_return);
        sid_return = NULL;
    }
    if (sid2_address_range) {
        lib_free(sid2_address_range);
        sid2_address_range = NULL;
    }
    if (sid3_address_range) {
        lib_free(sid3_address_range);
        sid3_address_range = NULL;
    }
    if (sid4_address_range) {
        lib_free(sid4_address_range);
        sid4_address_range = NULL;
    }
    if (sid5_address_range) {
        lib_free(sid5_address_range);
        sid5_address_range = NULL;
    }
    if (sid6_address_range) {
        lib_free(sid6_address_range);
        sid6_address_range = NULL;
    }
    if (sid7_address_range) {
        lib_free(sid7_address_range);
        sid7_address_range = NULL;
    }
    if (sid8_address_range) {
        lib_free(sid8_address_range);
        sid8_address_range = NULL;
    }
}
