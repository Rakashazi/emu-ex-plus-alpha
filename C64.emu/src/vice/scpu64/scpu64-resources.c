/*
 * scpu64-resources.c
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *  Andreas Boose <viceteam@t-online.de>
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

#include "scpu64-resources.h"
#include "c64acia.h"
#include "c64cart.h"
#include "c64cia.h"
#include "scpu64rom.h"
#include "scpu64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "reu.h"
#include "georam.h"
#include "sid-resources.h"
#include "util.h"
#include "vicii-resources.h"
#include "vicii.h"
#include "c64fastiec.h"

/* What sync factor between the CPU and the drive?  If equal to
   `MACHINE_SYNC_PAL', the same as PAL machines.  If equal to
   `MACHINE_SYNC_NTSC', the same as NTSC machines.  The sync factor is
   calculated as 65536 * drive_clk / clk_[main machine] */
static int sync_factor;

/* Name of the character ROM.  */
static char *chargen_rom_name = NULL;

/* Name of the SCPU64 ROM.  */
static char *scpu64_rom_name = NULL;

int cia1_model;
int cia2_model;

static int scpu64_simm_size;
static int scpu64_jiffy_switch = 1;
static int scpu64_speed_switch = 1;

static int iec_reset = 0;

static int set_chargen_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_rom_name, val)) {
        return 0;
    }

    return scpu64rom_load_chargen(chargen_rom_name);
}

static int set_scpu64_rom_name(const char *val, void *param)
{
    if (util_string_set(&scpu64_rom_name, val)) {
        return 0;
    }

    return scpu64rom_load_scpu64(scpu64_rom_name);
}

static int set_iec_reset(int val, void *param)
{
    iec_reset = val ? 1 : 0;

    return 0;
}

static int set_cia1_model(int val, void *param)
{
    switch (val) {
        case CIA_MODEL_6526:
        case CIA_MODEL_6526A:
            break;
        default:
            return -1;
    }

    if (cia1_model != val) {
        cia1_model = val;
        cia1_update_model();
    }

    return 0;
}

static int set_cia2_model(int val, void *param)
{
    switch (val) {
        case CIA_MODEL_6526:
        case CIA_MODEL_6526A:
            break;
        default:
            return -1;
    }

    if (cia2_model != val) {
        cia2_model = val;
        cia2_update_model();
    }

    return 0;
}

static int set_sync_factor(int val, void *param)
{
    int change_timing = 0;
    int border_mode = VICII_BORDER_MODE(vicii_resources.border_mode);

    if (sync_factor != val) {
        change_timing = 1;
    }

    switch (val) {
        case MACHINE_SYNC_PAL:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_PAL ^ border_mode);
            }
            break;
        case MACHINE_SYNC_NTSC:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_NTSC ^ border_mode);
            }
            break;
        case MACHINE_SYNC_NTSCOLD:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_NTSCOLD ^ border_mode);
            }
            break;
        case MACHINE_SYNC_PALN:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_PALN ^ border_mode);
            }
            break;
        default:
            return -1;
    }

    return 0;
}

static int set_scpu64_simm_size(int val, void *param)
{
    switch (val) {
        case 0:
        case 1:
        case 4:
        case 8:
        case 16:
            break;
        default:
            return -1;
    }
    scpu64_simm_size = val;
    mem_set_simm_size(val);
    return 0;
}

static int set_jiffy_switch(int val, void *param)
{
    scpu64_jiffy_switch = val ? 1 : 0;

    mem_set_jiffy_switch(scpu64_jiffy_switch);

    return 0;
}

static int set_speed_switch(int val, void *param)
{
    scpu64_speed_switch = val ? 1 : 0;

    mem_set_speed_switch(scpu64_speed_switch);

    return 0;
}

static const resource_string_t resources_string[] = {
    { "ChargenName", "chargen", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &chargen_rom_name, set_chargen_rom_name, NULL },
    { "SCPU64Name", "scpu64", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &scpu64_rom_name, set_scpu64_rom_name, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
    { "IECReset", 1, RES_EVENT_SAME, NULL,
      &iec_reset, set_iec_reset, NULL },
    { "CIA1Model", CIA_MODEL_6526, RES_EVENT_SAME, NULL,
      &cia1_model, set_cia1_model, NULL },
    { "CIA2Model", CIA_MODEL_6526, RES_EVENT_SAME, NULL,
      &cia2_model, set_cia2_model, NULL },
    { "SidStereoAddressStart", 0xde00, RES_EVENT_SAME, NULL,
      (int *)&sid_stereo_address_start, sid_set_sid_stereo_address, NULL },
    { "SidTripleAddressStart", 0xdf00, RES_EVENT_SAME, NULL,
      (int *)&sid_triple_address_start, sid_set_sid_triple_address, NULL },
    { "BurstMod", BURST_MOD_NONE, RES_EVENT_NO, NULL,
      &burst_mod, set_burst_mod, NULL },
    { "SIMMSize", 16, RES_EVENT_NO, NULL,
      &scpu64_simm_size, set_scpu64_simm_size, NULL },
    { "JiffySwitch", 1, RES_EVENT_STRICT, NULL,
      &scpu64_jiffy_switch, set_jiffy_switch, NULL },
    { "SpeedSwitch", 1, RES_EVENT_STRICT, NULL,
      &scpu64_speed_switch, set_speed_switch, NULL },
    { NULL }
};

void scpu64_resources_update_cia_models(int model)
{
    set_cia1_model(model, NULL);
    set_cia2_model(model, NULL);
}

int scpu64_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void scpu64_resources_shutdown(void)
{
    lib_free(chargen_rom_name);
    lib_free(scpu64_rom_name);
}
