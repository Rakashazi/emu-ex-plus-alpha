/*
 * ram.c - RAM stuff.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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
#include <string.h>

#include "cmdline.h"
#include "machine.h"
#include "ram.h"
#include "resources.h"
#include "translate.h"
#include "types.h"

static int start_value = 0;
static int value_invert = 64;
static int pattern_invert = 0;

static int set_start_value(int val, void *param)
{
    start_value = val;
    if (start_value < 0) {
        start_value = 0;
    }
    if (start_value > 0xff) {
        start_value = 0xff;
    }
    return 0;
}

static int set_value_invert(int val, void *param)
{
    value_invert = val;
    return 0;
}

static int set_pattern_invert(int val, void *param)
{
    pattern_invert = val;
    return 0;
}


/* RAM-related resources. */
static const resource_int_t resources_int[] = {
    { "RAMInitStartValue", 0, RES_EVENT_SAME, NULL,
      &start_value, set_start_value, NULL },
    { "RAMInitValueInvert", 64, RES_EVENT_SAME, NULL,
      &value_invert, set_value_invert, NULL },
    { "RAMInitPatternInvert", 0, RES_EVENT_SAME, NULL,
      &pattern_invert, set_pattern_invert, NULL },
    { NULL }
};

int ram_resources_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return resources_register_int(resources_int);
    }
    return 0;
}

static const cmdline_option_t cmdline_options[] = {
    { "-raminitstartvalue", SET_RESOURCE, 1,
      NULL, NULL, "RAMInitStartValue", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_VALUE, IDCLS_SET_FIRST_RAM_ADDRESS_VALUE,
      NULL, NULL },
    { "-raminitvalueinvert", SET_RESOURCE, 1,
      NULL, NULL, "RAMInitValueInvert", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUM_OF_BYTES, IDCLS_LENGTH_BLOCK_SAME_VALUE,
      NULL, NULL },
    { "-raminitpatterninvert", SET_RESOURCE, 1,
      NULL, NULL, "RAMInitPatternInvert", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUM_OF_BYTES, IDCLS_LENGTH_BLOCK_SAME_PATTERN,
      NULL, NULL },
    { NULL }
};

int ram_cmdline_options_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}


void ram_init(BYTE *memram, unsigned int ramsize)
{
    unsigned int i, j, k, l;
    BYTE v = start_value;

    j = value_invert - 1;
    k = pattern_invert - 1;
    for (i = 0; i < ramsize; i++)
    {
        l = (j < k) ? j : k;
        if (l >= ramsize) {
            l = ramsize - 1;
        }
        memset(memram + i, v, l - i + 1);
        i = l;

        if (i == j) {
            j += value_invert;
            v ^= 0xff;
        }

        if (i == k) {
            k += pattern_invert;
            v ^= 0xff;
        }
    }
}


const char *ram_init_print_pattern(void)
{
    static char s[512], s_tmp[16], pattern_line[64];
    BYTE v = start_value;
    int i;
    int linenum = 0;
    int last_line_drawn = 0;

    s[0] = 0;

    do {
        pattern_line[0] = 0;

        for (i = 0; i < 8; i++) {
            sprintf(s_tmp, " %02x", v);

            strcat(pattern_line, s_tmp);

            if (value_invert > 0 && (linenum * 8 + i + 1) % value_invert == 0) {
                v ^= 0xff;
            }

            if (pattern_invert > 0 && (linenum * 8 + i + 1) % pattern_invert == 0) {
                v ^= 0xff;
            }
        }

        if (linenum * 8 == 0
            || linenum * 8 == value_invert
            || linenum * 8 == pattern_invert
            || linenum * 8 == pattern_invert + value_invert) {
            sprintf(s_tmp, "%04x ", linenum * 8);
            strcat(s, s_tmp);
            strcat(s, pattern_line);
            strcat(s, "\n");
            last_line_drawn = 1;
        } else {
            if (last_line_drawn == 1) {
                strcat(s, "...\n");
            }
            last_line_drawn = 0;
        }

        linenum++;
    } while (linenum * 8 < value_invert * 2 || linenum * 8 < pattern_invert * 2);

    if (last_line_drawn == 1) {
        strcat(s, "...\n");
    }

    return s;
}
