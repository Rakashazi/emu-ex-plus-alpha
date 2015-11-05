/*
 * plus4model.c - Plus4 model detection and setting.
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

#include "vice.h"

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <string.h>

#include "plus4-resources.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "plus4model.h"
#include "machine.h"
#include "resources.h"
#include "types.h"

struct model_s {
    int video;   /* machine video timing */
    int ramsize;
    int hasspeech;
    int hasacia;
    int hasuserport;
    char *kernalname;
    char *basicname;
    char *plus1loname;
    char *plus1hiname;
};

/*
    C16/116    16K ram, no ACIA, no Userport
    Plus4      "same as 264". 64K ram, 3plus1 rom, ACIA
    V364       "a 264 + voice software"
    232        "a 264 with 32k ram", no 3plus1 rom, no rs232 interface (not ttl level)
*/

static struct model_s plus4models[] = {
    { MACHINE_SYNC_PAL,  RAM16K, NO_SPEECH,  NO_ACIA,  NO_USERPORT,  "kernal",     "basic", "",         "",        }, /* c16 (pal) */
    { MACHINE_SYNC_NTSC, RAM16K, NO_SPEECH,  NO_ACIA,  NO_USERPORT,  "kernal.005", "basic", "",         "",        }, /* c16 (ntsc) */
    { MACHINE_SYNC_PAL,  RAM64K, NO_SPEECH,  HAS_ACIA, HAS_USERPORT, "kernal",     "basic", "3plus1lo", "3plus1hi" }, /* plus4 (pal) */
    { MACHINE_SYNC_NTSC, RAM64K, NO_SPEECH,  HAS_ACIA, HAS_USERPORT, "kernal.005", "basic", "3plus1lo", "3plus1hi" }, /* plus4 (ntsc) */
    { MACHINE_SYNC_NTSC, RAM64K, HAS_SPEECH, HAS_ACIA, HAS_USERPORT, "kernal.364", "basic", "3plus1lo", "3plus1hi" }, /* v364 (ntsc) */
    { MACHINE_SYNC_NTSC, RAM32K, NO_SPEECH,  NO_ACIA,  NO_USERPORT,  "kernal.232", "basic", "",         ""         }, /* 232 (ntsc) */
};

/* ------------------------------------------------------------------------- */

static int plus4model_get_temp(int video, int hasspeech, int hasacia, const char *fln, const char *kernal)
{
    int i;

    for (i = 0; i < PLUS4MODEL_NUM; ++i) {
        if ((plus4models[i].video == video)
            && (plus4models[i].hasspeech == hasspeech)
            && (plus4models[i].hasacia == hasacia)
            && ((strlen(plus4models[i].plus1loname) == 0 ? 1 : 0) == (strlen(fln) == 0 ? 1 : 0))
            && (!strcmp(plus4models[i].kernalname, kernal))) {
            return i;
        }
    }

    return PLUS4MODEL_UNKNOWN;
}

int plus4model_get(void)
{
    int video, hasspeech, hasacia;
    const char *fln;
    const char *kernal;

    if ((resources_get_int("MachineVideoStandard", &video) < 0)
        || (resources_get_int("Acia1Enable", &hasacia) < 0)
        || (resources_get_string("FunctionLowName", &fln) < 0)
        || (resources_get_string("KernalName", &kernal) < 0)
        || (resources_get_int("SpeechEnabled", &hasspeech) < 0)) {
        return -1;
    }

    return plus4model_get_temp(video, hasspeech, hasacia, fln, kernal);
}

#if 0
static void plus4model_set_temp(int model, int *ted_model, int *ramsize, int *hasspeech, int *hasacia)
{
    int old_model;

    old_model = plus4model_get_temp(*ted_model, *ramsize, *hasspeech, *hasacia);

    if ((model == old_model) || (model == PLUS4MODEL_UNKNOWN)) {
        return;
    }

    *ted_model = plus4models[model].video;
    *ramsize = plus4models[model].ramsize;
    *hasacia = plus4models[model].hasacia;
    *hasspeech = plus4models[model].hasspeech;
}
#endif

void plus4model_set(int model)
{
    int old_model;

    old_model = plus4model_get();

    if ((model == old_model) || (model == PLUS4MODEL_UNKNOWN)) {
        return;
    }

    resources_set_int("MachineVideoStandard", plus4models[model].video);
    resources_set_int("RamSize", plus4models[model].ramsize);

    resources_set_string("KernalName", plus4models[model].kernalname);
    resources_set_string("BasicName", plus4models[model].basicname);

    resources_set_string("FunctionLowName", plus4models[model].plus1loname);
    resources_set_string("FunctionHighName", plus4models[model].plus1hiname);

    resources_set_int("Acia1Enable", plus4models[model].hasacia);

    resources_set_int("SpeechEnabled", 0);
    if (plus4models[model].hasspeech) {
        resources_set_string("SpeechImage", "c2lo.364");
        resources_set_int("SpeechEnabled", 1);
    }
}
