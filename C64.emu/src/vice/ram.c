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
#include "lib.h"
#include "machine.h"
#include "ram.h"
#include "resources.h"
#include "types.h"

static int start_value = 255;
static int value_invert = 128;
static int value_offset = 0;

static int pattern_invert = 0;
static int pattern_invert_value = 0;

static int random_start = 0;
static int random_repeat = 0;
static int random_chance = 0;

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

static int set_value_offset(int val, void *param)
{
    value_offset = val;
    return 0;
}

static int set_pattern_invert(int val, void *param)
{
    pattern_invert = val;
    return 0;
}

static int set_pattern_invert_value(int val, void *param)
{
    pattern_invert_value = val;
    if (pattern_invert_value < 0) {
        pattern_invert_value = 0;
    }
    if (pattern_invert_value > 0xff) {
        pattern_invert_value = 0xff;
    }
    return 0;
}


static int set_random_start(int val, void *param)
{
    random_start = val;
    if (random_start < 0) {
        random_start = 0;
    }
    if (random_start > 0xff) {
        random_start = 0xff;
    }
    return 0;
}

static int set_random_repeat(int val, void *param)
{
    random_repeat = val;
    return 0;
}

static int set_random_chance(int val, void *param)
{
    random_chance = val;
    return 0;
}

/* FIXME: the defaults have been choosen so the result matches a real reported
          pattern in x64sc, AND from those one was picked so all raminitvalue 
          tests pass.
          
          however, the respective defaults should probably be different per
          emulator/machine.
*/
/* RAM-related resources. */
static const resource_int_t resources_int[] = {
    { "RAMInitValueOffset", 2, RES_EVENT_SAME, NULL,
      &value_offset, set_value_offset, NULL },
    { "RAMInitStartValue", 0, RES_EVENT_SAME, NULL,
      &start_value, set_start_value, NULL },
    { "RAMInitValueInvert", 4, RES_EVENT_SAME, NULL,
      &value_invert, set_value_invert, NULL },
    { "RAMInitPatternInvert", 16384, RES_EVENT_SAME, NULL,
      &pattern_invert, set_pattern_invert, NULL },
    { "RAMInitPatternInvertValue", 255, RES_EVENT_SAME, NULL,
      &pattern_invert_value, set_pattern_invert_value, NULL },
    { "RAMInitStartRandom", 0, RES_EVENT_SAME, NULL,
      &random_start, set_random_start, NULL },
    { "RAMInitRepeatRandom", 0, RES_EVENT_SAME, NULL,
      &random_repeat, set_random_repeat, NULL },
    { "RAMInitRandomChance", 1, RES_EVENT_SAME, NULL,
      &random_chance, set_random_chance, NULL },
    RESOURCE_INT_LIST_END
};

int ram_resources_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return resources_register_int(resources_int);
    }
    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    { "-raminitvalueoffset", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitValueOffset", NULL,
      "<offset>", "The first pattern is shifted by this many bytes" },
    { "-raminitstartvalue", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitStartValue", NULL,
      "<value>", "Set the value for the very first RAM address after powerup" },
    { "-raminitvalueinvert", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitValueInvert", NULL,
      "<num of bytes>", "Length of memory block initialized with the same value" },
    { "-raminitpatterninvert", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitPatternInvert", NULL,
      "<num of bytes>", "Length of memory block initialized with the same pattern" },
    { "-raminitpatterninvertvalue", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitPatternInvertValue", NULL,
      "<value>", "Value to invert with in second pattern" },
    { "-raminitstartrandom", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitStartRandom", NULL,
      "<num of bytes>", "Number of random bytes in random pattern" },
    { "-raminitrepeatrandom", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitRepeatRandom", NULL,
      "<num of bytes>", "Repeat random pattern after this many bytes" },
    { "-raminitrandomchance", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMInitRandomChance", NULL,
      "<value>", "Random chance for any bit to flip (0-0xfff)" },
    CMDLINE_LIST_END
};

int ram_cmdline_options_init(void)
{
    if (machine_class != VICE_MACHINE_VSID) {
        return cmdline_register_options(cmdline_options);
    }
    return 0;
}


void ram_init(uint8_t *memram, unsigned int ramsize)
{
    unsigned int offset, j, k;
    uint8_t value;
    
    for (offset = 0; offset < ramsize; offset++) {
        
        j = k = 0;
        if (value_invert) {
            j = (((offset + value_offset) / value_invert) & 1) ? 0xff : 0x00;
        }

        if (pattern_invert) {
            k = ((offset / pattern_invert) & 1) ? pattern_invert_value : 0x00;
        }

        value = start_value ^ j ^ k;
        
        j = k = 0;
        if (random_start && random_repeat) {
            k = ((offset % random_repeat) < random_start) ? lib_unsigned_rand(0, 0xff) : 0;
        }
        if (random_chance) {
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x80 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x40 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x20 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x10 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x08 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x04 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x02 : 0;
            j |= lib_unsigned_rand(0, 0x1000) < random_chance ? 0x01 : 0;
        }
        
        value ^= k ^ j;
        
        memram[offset] = value;
    }
}

/* create a preview of the RAM init pattern - this should be as fast as
   possible since it is used in the GUI */
void ram_init_print_pattern(char *s, int len, char *eol)
{
    unsigned char *mem;
    char *p = s, *pp;
    int i, a;
    const char hextab[16] = "0123456789abcdef";

    mem = lib_malloc(len);
    ram_init(mem, len);

    for (a = 0; a < len;) {
        /* add address at start if line */
        *p++ = hextab[(a >> 12) & 0x0f];
        *p++ = hextab[(a >> 8) & 0x0f];
        *p++ = hextab[(a >> 4) & 0x0f];
        *p++ = hextab[a & 0x0f];
        *p++ = ':';
        *p++ = ' ';
        /* add 16 bytes hex dump */
        for (i = 0; i < 16; i++, a++) {
            *p++ = hextab[(mem[a] >> 4) & 0x0f];
            *p++ = hextab[(mem[a]) & 0x0f];
            *p++ = ' ';
        }
        /* add end of line */
        pp = eol;
        while (*pp) {
            *p++ = *pp++;
        }
        /* after each full page add another end of line */
        if ((a & 0xff) == 0) {
            pp = eol;
            while (*pp) {
                *p++ = *pp++;
            }
        }
    }
    *p = 0;
    lib_free(mem);
}
