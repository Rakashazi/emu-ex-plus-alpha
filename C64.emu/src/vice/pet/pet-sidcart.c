/*
 * pet-sidcart.c - PET specific SID cart emulation.
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

#include "cmdline.h"
#include "resources.h"
#include "sid.h"
#include "sidcart.h"
#include "sid-cmdline-options.h"
#include "sid-resources.h"
#include "sound.h"
#include "translate.h"

int sidcart_address;
int sidcart_clock;

/* ------------------------------------------------------------------------- */

static int sidcart_sound_machine_init(sound_t *psid, int speed, int cycles_per_sec)
{
    if (!sidcart_clock) {
        return sid_sound_machine_init_vbr(psid, speed, cycles_per_sec, 1015);
    } else {
        return sid_sound_machine_init(psid, speed, cycles_per_sec);
    }
}

static sound_chip_t sidcart_sound_chip = {
    sid_sound_machine_open,
    sidcart_sound_machine_init,
    sid_sound_machine_close,
    sid_sound_machine_calculate_samples,
    sid_sound_machine_store,
    sid_sound_machine_read,
    sid_sound_machine_reset,
    sid_sound_machine_cycle_based,
    sid_sound_machine_channels,
    0 /* chip enabled */
};

static WORD sidcart_sound_chip_offset = 0;

void sidcart_sound_chip_init(void)
{
    sidcart_sound_chip_offset = sound_chip_register(&sidcart_sound_chip);
}

/* ------------------------------------------------------------------------- */

int sidcart_enabled(void)
{
    return sidcart_sound_chip.chip_enabled;
}

static int set_sidcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val != sidcart_sound_chip.chip_enabled) {
        sidcart_sound_chip.chip_enabled = val;
        sound_state_changed = 1;
    }
    return 0;
}

static int set_sid_address(int val, void *param)
{
    switch (val) {
        case 0x8f00:
        case 0xe900:
            break;
        default:
            return -1;
    }

    sidcart_address = val;

    return 0;
}

static int set_sid_clock(int val, void *param)
{
    switch (val) {
        case SIDCART_CLOCK_C64:
        case SIDCART_CLOCK_NATIVE:
            break;
        default:
            return -1;
    }

    if (val != sidcart_clock) {
        sidcart_clock = val;
        sid_state_changed = 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_int_t sidcart_resources_int[] = {
    { "SidCart", 0, RES_EVENT_SAME, NULL,
      &sidcart_sound_chip.chip_enabled, set_sidcart_enabled, NULL },
    { "SidAddress", 0xe900, RES_EVENT_SAME, NULL,
      &sidcart_address, set_sid_address, NULL },
    { "SidClock", SIDCART_CLOCK_NATIVE, RES_EVENT_SAME, NULL,
      &sidcart_clock, set_sid_clock, NULL },
    { NULL }
};

int sidcart_resources_init(void)
{
    if (sid_resources_init() < 0) {
        return -1;
    }
    return resources_register_int(sidcart_resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t sidcart_cmdline_options[] = {
    { "-sidcart", SET_RESOURCE, 0,
      NULL, NULL, "SidCart", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_SIDCART,
      NULL, NULL },
    { "+sidcart", SET_RESOURCE, 0,
      NULL, NULL, "SidCart", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_SIDCART,
      NULL, NULL },
    { "-sidcartaddress", SET_RESOURCE, 1,
      NULL, NULL, "SidAddress", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_ADDRESS, IDCLS_PET_SIDCART_ADDRESS,
      NULL, NULL },
    { "-sidcartclock", SET_RESOURCE, 1,
      NULL, NULL, "SidClock", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_CLOCK, IDCLS_PET_SIDCART_CLOCK,
      NULL, NULL },
    { NULL }
};

int sidcart_cmdline_options_init(void)
{
    if (sid_cmdline_options_init() < 0) {
        return -1;
    }
    return cmdline_register_options(sidcart_cmdline_options);
}
