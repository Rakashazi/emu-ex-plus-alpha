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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "machine.h"
#include "ram.h"
#include "resources.h"
#include "types.h"

#define RANDOM_CHANCE_MAX   0x1000

static RAMINITPARAM mainramparam = {
    .start_value = 255,
    .value_invert = 128,
    .value_offset = 0,

    .pattern_invert = 0,
    .pattern_invert_value = 0,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

static int set_start_value(int val, void *param)
{
    mainramparam.start_value = val;
    if (mainramparam.start_value < 0) {
        mainramparam.start_value = 0;
    }
    if (mainramparam.start_value > 0xff) {
        mainramparam.start_value = 0xff;
    }
    return 0;
}

static int set_value_invert(int val, void *param)
{
    mainramparam.value_invert = val;
    return 0;
}

static int set_value_offset(int val, void *param)
{
    mainramparam.value_offset = val;
    return 0;
}

static int set_pattern_invert(int val, void *param)
{
    mainramparam.pattern_invert = val;
    return 0;
}

static int set_pattern_invert_value(int val, void *param)
{
    mainramparam.pattern_invert_value = val;
    if (mainramparam.pattern_invert_value < 0) {
        mainramparam.pattern_invert_value = 0;
    }
    if (mainramparam.pattern_invert_value > 0xff) {
        mainramparam.pattern_invert_value = 0xff;
    }
    return 0;
}


static int set_random_start(int val, void *param)
{
    mainramparam.random_start = val;
    if (mainramparam.random_start < 0) {
        mainramparam.random_start = 0;
    }
    if (mainramparam.random_start > 0xff) {
        mainramparam.random_start = 0xff;
    }
    return 0;
}

static int set_random_repeat(int val, void *param)
{
    mainramparam.random_repeat = val;
    return 0;
}

static int set_random_chance(int val, void *param)
{
    if (val > RANDOM_CHANCE_MAX) {
        val = RANDOM_CHANCE_MAX;
    } else if (val < 0) {
        val = 0;
    }
    mainramparam.random_chance = val;
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
      &mainramparam.value_offset, set_value_offset, NULL },
    { "RAMInitStartValue", 0, RES_EVENT_SAME, NULL,
      &mainramparam.start_value, set_start_value, NULL },
    { "RAMInitValueInvert", 4, RES_EVENT_SAME, NULL,
      &mainramparam.value_invert, set_value_invert, NULL },
    { "RAMInitPatternInvert", 16384, RES_EVENT_SAME, NULL,
      &mainramparam.pattern_invert, set_pattern_invert, NULL },
    { "RAMInitPatternInvertValue", 255, RES_EVENT_SAME, NULL,
      &mainramparam.pattern_invert_value, set_pattern_invert_value, NULL },
    { "RAMInitStartRandom", 0, RES_EVENT_SAME, NULL,
      &mainramparam.random_start, set_random_start, NULL },
    { "RAMInitRepeatRandom", 0, RES_EVENT_SAME, NULL,
      &mainramparam.random_repeat, set_random_repeat, NULL },
    { "RAMInitRandomChance", 1, RES_EVENT_SAME, NULL,
      &mainramparam.random_chance, set_random_chance, NULL },
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

/* Methods for randomly selecting bits to flip. */
typedef enum {
  RANDOM_METHOD_NONE,   /* flip no (or all) bits */
  RANDOM_METHOD_GEOM,   /* generate bit intervals between flips */
  RANDOM_METHOD_UNIFORM /* generate discrete uniform per bit */
} random_method_t;

/* Generate a random variate from the geometric distribution with success
 * probability p, where the function parameter is the value log(1 - p).  If each
 * bit has a probability p of being flipped, this distribution corresponds to
 * the number of non-flipped bits between each subsequent flipped bit. */
static unsigned int random_method_geom_next(double log_1mp)
{
    /* For a uniform random variable U \in [0, 1], then X = log(U) / log(1 - p))
     * is exponentially distributed with rate -log(1 - p), and floor(X) is
     * geometrically distributed with success probability p. */
    double u = lib_double_rand_unit();
    /* u may be 0 but not 1; use 1 - u to avoid taking the log of 0. */
    double g = floor(log1p(-u) / log_1mp);
    /* Avoid overflow when converting to an unsigned int. */
    return (g > (double)UINT_MAX) ? UINT_MAX : g;
}

/* this can be used to init arbitrary memory */
void ram_init_with_pattern(uint8_t *memram, unsigned int ramsize, RAMINITPARAM *ramparam)
{
    unsigned int offset, j, k;
    uint8_t value;

    random_method_t random_method = RANDOM_METHOD_NONE;
    unsigned int random_mask_initial = 0;
    double log_1mp = -INFINITY;
    unsigned int random_next = UINT_MAX;

    if (ramparam->random_chance <= 0) {
        /* flipping no bits */
        random_method = RANDOM_METHOD_NONE;
        random_mask_initial = 0x00;
    } else if (ramparam->random_chance >= RANDOM_CHANCE_MAX) {
        /* flipping all bits; same as no bits, but with the opposite mask */
        random_method = RANDOM_METHOD_NONE;
        random_mask_initial = 0xff;
    } else if (ramparam->random_chance == (RANDOM_CHANCE_MAX / 2)) {
        /* flipping bits or not with equal probability; worst-case for the
         * geometric spacing method, so handle separately */
        random_method = RANDOM_METHOD_UNIFORM;
    } else if (ramparam->random_chance < (RANDOM_CHANCE_MAX / 2)) {
        /* some other probability less than 0.5; generate the number of bits
         * un-flipped between each flipped bit. */
        random_method = RANDOM_METHOD_GEOM;
        random_mask_initial = 0x00;
        log_1mp = log1p((double)-ramparam->random_chance / RANDOM_CHANCE_MAX);
        random_next = random_method_geom_next(log_1mp);
    } else {
        /* some other probability greater than 0.5; generate the number of bits
         * flipped between each un-flipped bit. */
        random_method = RANDOM_METHOD_GEOM;
        random_mask_initial = 0xff;
        log_1mp = log((double)ramparam->random_chance / RANDOM_CHANCE_MAX);
        random_next = random_method_geom_next(log_1mp);
    }

    for (offset = 0; offset < ramsize; offset++) {

        j = k = 0;
        if (ramparam->value_invert) {
            j = (((offset + ramparam->value_offset) / ramparam->value_invert) & 1) ? 0xff : 0x00;
        }

        if (ramparam->pattern_invert) {
            k = ((offset / ramparam->pattern_invert) & 1) ? ramparam->pattern_invert_value : 0x00;
        }

        value = ramparam->start_value ^ j ^ k;

        k = 0;
        if (ramparam->random_start && ramparam->random_repeat) {
            k = ((offset % ramparam->random_repeat) < ramparam->random_start) ? lib_unsigned_rand(0, 0xff) : 0;
        }

        j = 0;
        switch (random_method) {
            case RANDOM_METHOD_NONE:
                j = random_mask_initial;
                break;
            case RANDOM_METHOD_UNIFORM:
                j = lib_unsigned_rand(0x00, 0xff);
                break;
            case RANDOM_METHOD_GEOM:
                j = random_mask_initial;
                while (random_next < 8) {
                    j ^= (1 << random_next);
                    random_next += 1 + random_method_geom_next(log_1mp);
                }
                random_next -= 8;
                break;
        }

        value ^= k ^ j;

        memram[offset] = value;
    }
}

/* used to initialize the main memory of the machine */
void ram_init(uint8_t *memram, unsigned int ramsize)
{
    ram_init_with_pattern(memram, ramsize, &mainramparam);
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
