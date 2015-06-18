/*
 * userport_dac.c - Generic userport 8bit DAC emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 * Filtering by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "sound.h"
#include "translate.h"
#include "uiapi.h"
#include "userport_dac.h"

/* ------------------------------------------------------------------------- */

static sound_dac_t userport_dac_dac;

/* Some prototypes are needed */
static int userport_dac_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int userport_dac_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void userport_dac_sound_machine_store(sound_t *psid, WORD addr, BYTE val);
static BYTE userport_dac_sound_machine_read(sound_t *psid, WORD addr);
static void userport_dac_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int userport_dac_sound_machine_cycle_based(void)
{
    return 0;
}

static int userport_dac_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t userport_dac_sound_chip = {
    NULL, /* no open */
    userport_dac_sound_machine_init,
    NULL, /* no close */
    userport_dac_sound_machine_calculate_samples,
    userport_dac_sound_machine_store,
    userport_dac_sound_machine_read,
    userport_dac_sound_reset,
    userport_dac_sound_machine_cycle_based,
    userport_dac_sound_machine_channels,
    0 /* chip enabled */
};

static WORD userport_dac_sound_chip_offset = 0;

void userport_dac_sound_chip_init(void)
{
    userport_dac_sound_chip_offset = sound_chip_register(&userport_dac_sound_chip);
}

/* ------------------------------------------------------------------------- */

static int set_userport_dac_enabled(int val, void *param)
{
    userport_dac_sound_chip.chip_enabled = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "UserportDAC", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &userport_dac_sound_chip.chip_enabled, set_userport_dac_enabled, NULL },
    { NULL }
};

int userport_dac_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-userportdac", SET_RESOURCE, 0,
      NULL, NULL, "UserportDAC", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_DAC,
      NULL, NULL },
    { "+userportdac", SET_RESOURCE, 0,
      NULL, NULL, "UserportDAC", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_DAC,
      NULL, NULL },
    { NULL }
};

int userport_dac_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static BYTE userport_dac_sound_data;

void userport_dac_store(BYTE value)
{
    if (userport_dac_sound_chip.chip_enabled) {
        userport_dac_sound_data = value;
        sound_store(userport_dac_sound_chip_offset, value, 0);
    }
}

struct userport_dac_sound_s {
    BYTE voice0;
};

static struct userport_dac_sound_s snd;

static int userport_dac_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    return sound_dac_calculate_samples(&userport_dac_dac, pbuf, (int)snd.voice0 * 128, nr, soc, (soc > 1) ? 3 : 1);
}

static int userport_dac_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sound_dac_init(&userport_dac_dac, speed);
    snd.voice0 = 0;

    return 1;
}

static void userport_dac_sound_machine_store(sound_t *psid, WORD addr, BYTE val)
{
    snd.voice0 = val;
}

static BYTE userport_dac_sound_machine_read(sound_t *psid, WORD addr)
{
    /* FIXME: most likely needs to return 0, but not sure */
    return userport_dac_sound_data;
}

static void userport_dac_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    snd.voice0 = 0;
    userport_dac_sound_data = 0;
}
