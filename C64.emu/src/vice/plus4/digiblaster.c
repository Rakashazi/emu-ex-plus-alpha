/*
 * digimax.c - Digimax DAC cartridge emulation.
 *
 * Written by
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
#include "digiblaster.h"
#include "lib.h"
#include "maincpu.h"
#include "resources.h"
#include "sound.h"
#include "uiapi.h"
#include "translate.h"

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static int digiblaster_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec);
static int digiblaster_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int sound_output_channels, int sound_chip_channels, int *delta_t);
static void digiblaster_sound_machine_store(sound_t *psid, WORD addr, BYTE val);
static BYTE digiblaster_sound_machine_read(sound_t *psid, WORD addr);
static void digiblaster_sound_reset(sound_t *psid, CLOCK cpu_clk);

static int digiblaster_sound_machine_cycle_based(void)
{
    return 0;
}

static int digiblaster_sound_machine_channels(void)
{
    return 1;
}

static sound_chip_t digiblaster_sound_chip = {
    NULL, /* no open */
    digiblaster_sound_machine_init,
    NULL, /* no close */
    digiblaster_sound_machine_calculate_samples,
    digiblaster_sound_machine_store,
    digiblaster_sound_machine_read,
    digiblaster_sound_reset,
    digiblaster_sound_machine_cycle_based,
    digiblaster_sound_machine_channels,
    0 /* chip enabled */
};

static WORD digiblaster_sound_chip_offset = 0;
static sound_dac_t digiblaster_dac;

void digiblaster_sound_chip_init(void)
{
    digiblaster_sound_chip_offset = sound_chip_register(&digiblaster_sound_chip);
}

/* ---------------------------------------------------------------------*/

int digiblaster_enabled(void)
{
    return digiblaster_sound_chip.chip_enabled;
}

static int set_digiblaster_enabled(int val, void *param)
{
    digiblaster_sound_chip.chip_enabled = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DIGIBLASTER", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &digiblaster_sound_chip.chip_enabled, set_digiblaster_enabled, NULL },
    { NULL }
};

int digiblaster_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-digiblaster", SET_RESOURCE, 0,
      NULL, NULL, "DIGIBLASTER", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_DIGIBLASTER,
      NULL, NULL },
    { "+digiblaster", SET_RESOURCE, 0,
      NULL, NULL, "DIGIBLASTER", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_DIGIBLASTER,
      NULL, NULL },
    { NULL }
};

int digiblaster_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/

static BYTE digiblaster_sound_data;

struct digiblaster_sound_s {
    BYTE voice0;
};

static struct digiblaster_sound_s snd;

static int digiblaster_sound_machine_calculate_samples(sound_t **psid, SWORD *pbuf, int nr, int soc, int scc, int *delta_t)
{
    return sound_dac_calculate_samples(&digiblaster_dac, pbuf, (int)snd.voice0 * 128, nr, soc, (soc > 1) ? 3 : 1);
}

static int digiblaster_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sound_dac_init(&digiblaster_dac, speed);
    snd.voice0 = 0;

    return 1;
}

static void digiblaster_sound_machine_store(sound_t *psid, WORD addr, BYTE val)
{
    snd.voice0 = val;
}

static BYTE digiblaster_sound_machine_read(sound_t *psid, WORD addr)
{
    return 0;
}

static void digiblaster_sound_reset(sound_t *psid, CLOCK cpu_clk)
{
    snd.voice0 = 0;
    digiblaster_sound_data = 0;
}

/* ---------------------------------------------------------------------*/

void digiblaster_store(WORD addr, BYTE value)
{
    digiblaster_sound_data = value;
    sound_store(digiblaster_sound_chip_offset, value, 0);
}

BYTE digiblaster_read(WORD addr)
{
    return sound_read(digiblaster_sound_chip_offset, 0);
}
