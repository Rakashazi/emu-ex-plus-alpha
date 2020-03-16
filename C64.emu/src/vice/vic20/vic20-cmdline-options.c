/*
 * vic20-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "lib.h"
#include "log.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "vic20-cmdline-options.h"
#include "vic20mem.h"
#include "vic20model.h"

struct model_s {
    const char *name;
    int model;
};

static struct model_s model_match[] = {
    { "vic20", VIC20MODEL_VIC20_PAL },
    { "vic20pal", VIC20MODEL_VIC20_PAL },
    { "vic20ntsc", VIC20MODEL_VIC20_NTSC },
    { "vic21", VIC20MODEL_VIC21 },
    { NULL, VIC20MODEL_UNKNOWN }
};

static int set_vic20_model(const char *param, void *extra_param)
{
    int model = VIC20MODEL_UNKNOWN;
    int i = 0;

    if (!param) {
        return -1;
    }

    do {
        if (strcmp(model_match[i].name, param) == 0) {
            model = model_match[i].model;
        }
        i++;
    } while ((model == VIC20MODEL_UNKNOWN) && (model_match[i].name != NULL));

    if (model == VIC20MODEL_UNKNOWN) {
        return -1;
    }

    vic20model_set(model);

    return 0;
}


/* This function parses the mem config string given as `-memory' and returns
 * the appropriate values or'ed together.
 *
 * basically we accept a comma separated list of options like the following:
 * ""   - no extension
 * none - same
 * all  - all blocks
 * 3k   - 3k space in block 0 (=3k extension cartridge)
 * 8k   - 1st 8k extension block
 * 16k  - 1st and 2nd 8 extension (= 16k extension cartridge)
 * 24k  - 1-3rd extension block (=8k and 16k extension cartridges)
 *
 * 0,1,2,3,5      - memory in respective block
 * 04,20,40,60,a0 - memory at respective address (same as block #s)
 *
 * example: xvic -memory ""
 *
 *            enables unexpanded computer
 *
 *          xvic -memory 60,a0
 *
 *            enables memory in blocks 3 and 5, which is the usual
 *            configuration for 16k rom modules
 *
 * 12/27/96 Alexander Lehmann <alex@mathematik.th-darmstadt.de>
 * Edited by Ettore to fit in the new command-line parsing.
 */

static int cmdline_memory(const char *param, void *extra_param)
{
    int memconf = 0;
    const char *memstring = param, *optend;

    /* Default is all banks. */
    if (!memstring) {
        memconf = VIC_BLK_ALL;
    } else {
        char *opt;

        opt = lib_malloc(strlen(param) + 1);
        while (*memstring) {
            for (optend = memstring; *optend && *optend != ','; optend++) {
            }

            strncpy(opt, memstring, optend - memstring);
            opt[optend - memstring] = '\0';

            if (strcmp(opt, "") == 0 || strcmp(opt, "none") == 0) {
                /* no extension */
            } else if (strcmp(opt, "all") == 0) {
                memconf = VIC_BLK_ALL;
            } else if (strcmp(opt, "3k") == 0) {
                memconf |= VIC_BLK0;
            } else if (strcmp(opt, "8k") == 0) {
                memconf |= VIC_BLK1;
            } else if (strcmp(opt, "16k") == 0) {
                memconf |= VIC_BLK1 | VIC_BLK2;
            } else if (strcmp(opt, "24k") == 0) {
                memconf |= VIC_BLK1 | VIC_BLK2 | VIC_BLK3;
            } else if (strcmp(opt, "0") == 0 || strcmp(opt, "04") == 0) {
                memconf |= VIC_BLK0;
            } else if (strcmp(opt, "1") == 0 || strcmp(opt, "20") == 0) {
                memconf |= VIC_BLK1;
            } else if (strcmp(opt, "2") == 0 || strcmp(opt, "40") == 0) {
                memconf |= VIC_BLK2;
            } else if (strcmp(opt, "3") == 0 || strcmp(opt, "60") == 0) {
                memconf |= VIC_BLK3;
            } else if (strcmp(opt, "5") == 0 || strcmp(opt, "a0") == 0
                       || strcmp(opt, "A0") == 0) {
                memconf |= VIC_BLK5;
            } else {
                log_error(LOG_ERR,
                          "Unsupported memory extension option: `%s'.", opt);
                lib_free(opt);
                return -1;
            }
            memstring = optend;
            if (*memstring) {
                memstring++;    /* skip ',' */
            }
        }
        lib_free(opt);
    }

    /* FIXME: this is before log is initialized, right? */
    log_message(LOG_DEFAULT, "Extension memory enabled: ");

    if (memconf & VIC_BLK0) {
        resources_set_int("RAMBlock0", 1);
        log_message(LOG_DEFAULT, "blk0 ");
    } else {
        resources_set_int("RAMBlock0", 0);
    }
    if (memconf & VIC_BLK1) {
        resources_set_int("RAMBlock1", 1);
        log_message(LOG_DEFAULT, "blk1 ");
    } else {
        resources_set_int("RAMBlock1", 0);
    }
    if (memconf & VIC_BLK2) {
        resources_set_int("RAMBlock2", 1);
        log_message(LOG_DEFAULT, "blk2 ");
    } else {
        resources_set_int("RAMBlock2", 0);
    }
    if (memconf & VIC_BLK3) {
        resources_set_int("RAMBlock3", 1);
        log_message(LOG_DEFAULT, "blk3 ");
    } else {
        resources_set_int("RAMBlock3", 0);
    }
    if (memconf & VIC_BLK5) {
        resources_set_int("RAMBlock5", 1);
        log_message(LOG_DEFAULT, "blk5");
    } else {
        resources_set_int("RAMBlock5", 0);
    }
    if (memconf == 0) {
        log_message(LOG_DEFAULT, "none");
    }

    return 0;
}

static cmdline_option_t const cmdline_options[] =
{
    { "-pal", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_PAL,
      NULL, "Use PAL sync factor" },
    { "-ntsc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_NTSC,
      NULL, "Use NTSC sync factor" },
    { "-kernal", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalName", NULL,
      "<Name>", "Specify name of Kernal ROM image" },
    { "-basic", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicName", NULL,
      "<Name>", "Specify name of BASIC ROM image" },
    { "-chargen", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenName", NULL,
      "<Name>", "Specify name of character generator ROM image" },
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    { "-acia1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Acia1Enable", (void *)1,
      NULL, "Enable the ACIA RS232 interface emulation" },
    { "+acia1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Acia1Enable", (void *)0,
      NULL, "Disable the ACIA RS232 interface emulation" },
#endif
    { "-memory", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      cmdline_memory, NULL, NULL, NULL,
      "<spec>", "Specify memory configuration. (none/3k/8k/16k/24k/all) / (0/1/2/3/5) / (04/20/40/60/a0)" },
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      set_vic20_model, NULL, NULL, NULL,
      "<Model>", "Set VIC20 model (vic20/vic20pal/vic20ntsc, vic21)" },
    { "-vflimod", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VFLImod", (resource_value_t)1,
      NULL, "Enable VFLI modification." },
    { "+vflimod", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VFLImod", (resource_value_t)0,
      NULL, "Disable VFLI modification." },
    CMDLINE_LIST_END
};

int vic20_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
