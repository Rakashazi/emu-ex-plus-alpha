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

static BYTE machine_sid2_read(WORD addr)
{
    return sid2_read(addr);
}


static void machine_sid2_store(WORD addr, BYTE byte)
{
    sid2_store(addr, byte);
}

static BYTE machine_sid3_read(WORD addr)
{
    return sid3_read(addr);
}

static void machine_sid3_store(WORD addr, BYTE byte)
{
    sid3_store(addr, byte);
}

/* ---------------------------------------------------------------------*/

static io_source_t stereo_sid_device = {
    "Stereo SID",
    IO_DETACH_RESOURCE,
    "SidStereo",
    0xde00, 0xde1f, 0x1f,
    1, /* read is always valid */
    machine_sid2_store,
    machine_sid2_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    0,
    0,
    0
};

static io_source_t triple_sid_device = {
    "Triple SID",
    IO_DETACH_RESOURCE,
    "SidStereo",
    0xdf00, 0xdf1f, 0x1f,
    1, /* read is always valid */
    machine_sid3_store,
    machine_sid3_read,
    NULL, /* TODO: peek */
    NULL, /* TODO: dump */
    0,
    0,
    0
};

static io_source_list_t *stereo_sid_list_item = NULL;
static io_source_list_t *triple_sid_list_item = NULL;

/* ---------------------------------------------------------------------*/

static sound_chip_t sid_sound_chip = {
    sid_sound_machine_open,
    sid_sound_machine_init,
    sid_sound_machine_close,
    sid_sound_machine_calculate_samples,
    sid_sound_machine_store,
    sid_sound_machine_read,
    sid_sound_machine_reset,
    sid_sound_machine_cycle_based,
    sid_sound_machine_channels,
    1 /* chip enabled */
};

static WORD sid_sound_chip_offset = 0;

void sid_sound_chip_init(void)
{
    sid_sound_chip_offset = sound_chip_register(&sid_sound_chip);
}

/* ---------------------------------------------------------------------*/

int machine_sid2_check_range(unsigned int sid2_adr)
{
    if (machine_class == VICE_MACHINE_C128) {
        if ((sid2_adr >= 0xd400 && sid2_adr <= 0xd4e0) || (sid2_adr >= 0xd700 && sid2_adr <= 0xdfe0)) {
            sid_stereo_address_start = sid2_adr;
            stereo_sid_device.start_address = sid2_adr;
            sid_stereo_address_end = sid2_adr + 0x1f;
            stereo_sid_device.end_address = sid2_adr + 0x1f;
            if (stereo_sid_list_item != NULL) {
                io_source_unregister(stereo_sid_list_item);
                stereo_sid_list_item = io_source_register(&stereo_sid_device);
            } else {
                if (sid_stereo >= 1) {
                    stereo_sid_list_item = io_source_register(&stereo_sid_device);
                }
            }
            return 0;
        }
    } else {
        if (sid2_adr >= 0xd400 && sid2_adr <= 0xdfe0) {
            sid_stereo_address_start = sid2_adr;
            stereo_sid_device.start_address = sid2_adr;
            sid_stereo_address_end = sid2_adr + 0x1f;
            stereo_sid_device.end_address = sid2_adr + 0x1f;
            if (stereo_sid_list_item != NULL) {
                io_source_unregister(stereo_sid_list_item);
                stereo_sid_list_item = io_source_register(&stereo_sid_device);
            } else {
                if (sid_stereo >= 1) {
                    stereo_sid_list_item = io_source_register(&stereo_sid_device);
                }
            }
            return 0;
        }
    }
    return -1;
}

int machine_sid3_check_range(unsigned int sid3_adr)
{
    if (machine_class == VICE_MACHINE_C128) {
        if ((sid3_adr >= 0xd400 && sid3_adr <= 0xd4e0) || (sid3_adr >= 0xd700 && sid3_adr <= 0xdfe0)) {
            sid_triple_address_start = sid3_adr;
            triple_sid_device.start_address = sid3_adr;
            sid_triple_address_end = sid3_adr + 0x1f;
            triple_sid_device.end_address = sid3_adr + 0x1f;
            if (triple_sid_list_item != NULL) {
                io_source_unregister(triple_sid_list_item);
                triple_sid_list_item = io_source_register(&triple_sid_device);
            } else {
                if (sid_stereo >= 2) {
                    triple_sid_list_item = io_source_register(&triple_sid_device);
                }
            }
            return 0;
        }
    } else {
        if (sid3_adr >= 0xd400 && sid3_adr <= 0xdfe0) {
            sid_triple_address_start = sid3_adr;
            triple_sid_device.start_address = sid3_adr;
            sid_triple_address_end = sid3_adr + 0x1f;
            triple_sid_device.end_address = sid3_adr + 0x1f;
            if (triple_sid_list_item != NULL) {
                io_source_unregister(triple_sid_list_item);
                triple_sid_list_item = io_source_register(&triple_sid_device);
            } else {
                if (sid_stereo >= 2) {
                    triple_sid_list_item = io_source_register(&triple_sid_device);
                }
            }
            return 0;
        }
    }
    return -1;
}

void machine_sid2_enable(int val)
{
    if (stereo_sid_list_item != NULL) {
        io_source_unregister(stereo_sid_list_item);
        stereo_sid_list_item = NULL;
    }
    if (triple_sid_list_item != NULL) {
        io_source_unregister(triple_sid_list_item);
        triple_sid_list_item = NULL;
    }

    if (val >= 1) {
        stereo_sid_list_item = io_source_register(&stereo_sid_device);
    }
    if (val >= 2) {
        triple_sid_list_item = io_source_register(&triple_sid_device);
    }
}

void sound_machine_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
    sid_sound_machine_prevent_clk_overflow(psid, sub);
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
