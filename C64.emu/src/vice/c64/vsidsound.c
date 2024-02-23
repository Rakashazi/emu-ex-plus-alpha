/*
 * c64sound.c - C64/C128 sound emulation.
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
#include <string.h>

#include "cartio.h"
#include "cartridge.h"
#include "machine.h"
#include "sid.h"
#include "sid-resources.h"
#include "sound.h"
#include "types.h"

/* ---------------------------------------------------------------------*/

#ifdef SOUND_SYSTEM_FLOAT
/* stereo mixing placement of the VSID SID sound */
static sound_chip_mixing_spec_t sid_sound_mixing_spec[SOUND_CHIP_CHANNELS_MAX] = {
    {
        100, /* left channel volume % in case of stereo output, default output to both */
        100  /* right channel volume % in case of stereo output, default output to both */
    }
};
#endif

/* VSID SID sound chip */
static sound_chip_t sid_sound_chip = {
    sid_sound_machine_open,              /* sound chip open function */
    sid_sound_machine_init,              /* sound chip init function */
    sid_sound_machine_close,             /* sound chip close function */
    sid_sound_machine_calculate_samples, /* sound chip calculate samples function */
    sid_sound_machine_store,             /* sound chip store function */
    sid_sound_machine_read,              /* sound chip read function */
    sid_sound_machine_reset,             /* sound chip reset function */
    sid_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, RESID engine is cycle based, everything else is NOT */
    sid_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, depends on how many extra SIDs are active */
#ifdef SOUND_SYSTEM_FLOAT
    sid_sound_mixing_spec,               /* stereo mixing placement specs */
#endif
    1                                    /* chip is always enabled */
};

static uint16_t sid_sound_chip_offset = 0;

void sid_sound_chip_init(void)
{
    sid_sound_chip_offset = sound_chip_register(&sid_sound_chip);
}

/* ---------------------------------------------------------------------*/

#define SIDx_CHECK_RANGE(sid_nr)                                \
    int machine_sid##sid_nr##_check_range(unsigned int sid_adr) \
    {                                                           \
        if (sid_adr >= 0xd400 && sid_adr <= 0xdfe0) {           \
            sid##sid_nr##_address_start = sid_adr;              \
            sid##sid_nr##_address_end = sid_adr + 0x1f;         \
            return 0;                                           \
        }                                                       \
        return -1;                                              \
    }

SIDx_CHECK_RANGE(2)
SIDx_CHECK_RANGE(3)
SIDx_CHECK_RANGE(4)
SIDx_CHECK_RANGE(5)
SIDx_CHECK_RANGE(6)
SIDx_CHECK_RANGE(7)
SIDx_CHECK_RANGE(8)

void machine_sid2_enable(int val)
{
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
