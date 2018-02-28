/*
 * c64-resources.c
 *
 * Written by
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

#include "c64-resources.h"
#include "c64acia.h"
#include "c64cart.h"
#include "c64cia.h"
#include "c64rom.h"
#include "c64memrom.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "patchrom.h"
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

/* Name of the BASIC ROM.  */
static char *basic_rom_name = NULL;

/* Name of the Kernal ROM.  */
static char *kernal_rom_name = NULL;

/* Kernal revision for ROM patcher.  */
int kernal_revision = C64_KERNAL_REV3;

int cia1_model;
int cia2_model;

static int board_type = 0;
static int iec_reset = 0;

static int set_chargen_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_rom_name, val)) {
        return 0;
    }

    return c64rom_load_chargen(chargen_rom_name);
}

static int set_kernal_rom_name(const char *val, void *param)
{
    int ret, changed = 1;
    log_verbose("set_kernal_rom_name val:%s.", val);
    if ((val != NULL) && (kernal_rom_name != NULL)) {
        changed = (strcmp(val, kernal_rom_name) != 0);
    }
    if (util_string_set(&kernal_rom_name, val)) {
        return 0;
    }
    /* load kernal without a kernal overriding buffer */
    ret = c64rom_load_kernal(kernal_rom_name, NULL);
    if (changed) {
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    return ret;
}

static int set_basic_rom_name(const char *val, void *param)
{
    int ret, changed = 1;
    if ((val != NULL) && (basic_rom_name != NULL)) {
        changed = (strcmp(val, basic_rom_name) != 0);
    }
    if (util_string_set(&basic_rom_name, val)) {
        return 0;
    }
    ret = c64rom_load_basic(basic_rom_name);
    if (changed) {
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    return ret;
}

static int set_board_type(int val, void *param)
{
    int old_board_type = board_type;
    if ((val < 0) || (val > 1)) {
        return -1;
    }
    board_type = val;
    if (old_board_type != board_type) {
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    return 0;
}

static int set_iec_reset(int val, void *param)
{
    iec_reset = val ? 1 : 0;
    return 0;
}

static int set_cia1_model(int val, void *param)
{
    int old_cia_model = cia1_model;

    switch (val) {
        case CIA_MODEL_6526:
        case CIA_MODEL_6526A:
            cia1_model = val;
            break;
        default:
            return -1;
    }

    if (old_cia_model != cia1_model) {
        cia1_update_model();
    }

    return 0;
}

static int set_cia2_model(int val, void *param)
{
    int old_cia_model = cia2_model;

    switch (val) {
        case CIA_MODEL_6526:
        case CIA_MODEL_6526A:
            cia2_model = val;
            break;
        default:
            return -1;
    }

    if (old_cia_model != cia2_model) {
        cia2_update_model();
    }

    return 0;
}

static int set_kernal_revision(int val, void *param)
{
    int trapfl;

    log_verbose("set_kernal_revision val:%d kernal_revision: %d", val, kernal_revision);
    if(!c64rom_isloaded()) {
        return 0;
    }
    /* disable device traps before kernal patching */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_get_int("VirtualDevices", &trapfl);
        resources_set_int("VirtualDevices", 0);
    }
    /* patch kernal to given revision */
    if ((val != -1) && (patch_rom_idx(val) < 0)) {
        val = -1;
    }
    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
    if (kernal_revision != val) {
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    /* restore traps */
    if (machine_class != VICE_MACHINE_VSID) {
        resources_set_int("VirtualDevices", trapfl);
    }
    kernal_revision = val;
    log_verbose("set_kernal_revision new kernal_revision: %d", kernal_revision);
    return 0;
}

static int set_sync_factor(int val, void *param)
{
    int change_timing = 0;

    if (sync_factor != val) {
        change_timing = 1;
    }

    switch (val) {
        case MACHINE_SYNC_PAL:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_PAL, vicii_resources.border_mode);
            }
            break;
        case MACHINE_SYNC_NTSC:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_NTSC, vicii_resources.border_mode);
            }
            break;
        case MACHINE_SYNC_NTSCOLD:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_NTSCOLD, vicii_resources.border_mode);
            }
            break;
        case MACHINE_SYNC_PALN:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_PALN, vicii_resources.border_mode);
            }
            break;
        default:
            return -1;
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "ChargenName", "chargen", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &chargen_rom_name, set_chargen_rom_name, NULL },
    { "KernalName", "kernal", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &kernal_rom_name, set_kernal_rom_name, NULL },
    { "BasicName", "basic", RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &basic_rom_name, set_basic_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
    { "BoardType", 0, RES_EVENT_SAME, NULL,
      &board_type, set_board_type, NULL },
    { "IECReset", 0, RES_EVENT_SAME, NULL,
      &iec_reset, set_iec_reset, NULL },
    { "CIA1Model", CIA_MODEL_6526A, RES_EVENT_SAME, NULL,
      &cia1_model, set_cia1_model, NULL },
    { "CIA2Model", CIA_MODEL_6526A, RES_EVENT_SAME, NULL,
      &cia2_model, set_cia2_model, NULL },
    { "KernalRev", C64_KERNAL_REV3, RES_EVENT_SAME, NULL,
      &kernal_revision, set_kernal_revision, NULL },
    { "SidStereoAddressStart", 0xde00, RES_EVENT_SAME, NULL,
      (int *)&sid_stereo_address_start, sid_set_sid_stereo_address, NULL },
    { "SidTripleAddressStart", 0xdf00, RES_EVENT_SAME, NULL,
      (int *)&sid_triple_address_start, sid_set_sid_triple_address, NULL },
    { "BurstMod", BURST_MOD_NONE, RES_EVENT_NO, NULL,
      &burst_mod, set_burst_mod, NULL },
    RESOURCE_INT_LIST_END
};

void c64_resources_update_cia_models(int model)
{
    set_cia1_model(model, NULL);
    set_cia2_model(model, NULL);
}

int c64_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void c64_resources_shutdown(void)
{
    lib_free(chargen_rom_name);
    lib_free(basic_rom_name);
    lib_free(kernal_rom_name);
}
