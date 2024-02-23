/*
 * cbm5x0-resources.c - CBM-5x0 resources.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "archdep.h"
#include "cbm2-resources.h"
#include "cbm2.h"
#include "cbm2cia.h"
#include "cbm2mem.h"
#include "cbm2model.h"
#include "cbm2rom.h"
#include "cbm2tpi.h"
#include "cia.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "resources.h"
#include "sid-resources.h"
#include "util.h"
#include "vicii-resources.h"
#include "vicii.h"
#include "vsync.h"

static int sync_factor;

/* Frequency of the power grid in Hz */
static int power_freq = 1;

static char *kernal_rom_name = NULL;
static char *chargen_name = NULL;
static char *basic_rom_name = NULL;

int cbm2_model_line = 0;

int cia1_model = CIA_MODEL_6526;

static int set_cbm2_model_line(int val, void *param)
{
    static const uint8_t model_port_mask[] = { 0xc0, 0x40, 0x00 };

    switch (val) {
        case LINE_6x0_60HZ:
        case LINE_6x0_50HZ:
            break;
        default:
            return -1;
    }

    cbm2_model_line = val;

    set_cbm2_model_port_mask(model_port_mask[cbm2_model_line]);

    /* FIXME: VIC-II config */
    return 0;
}

/* ramsize starts counting at 0x10000 if less than 512. If 512 or more,
   it starts counting at 0x00000.
   CBM2MEM module requires(!) that ramsize never gets 512-64 = 448
   (Just don't ask...)
   In a C500, that has RAM in bank 0, the actual size of RAM is 64k larger
   than ramsize when ramsize is less than 512, otherwise the same as ramsize.
*/

int ramsize;

static int set_ramsize(int rs, void *param)
{
    switch (rs) {
        case 64:
        case 128:
        case 256:
        case 512:
        case 1024:
            break;
        default:
            return -1;
    }

    ramsize = rs;
    vsync_suspend_speed_eval();
    mem_initialize_memory();
    machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);

    return 0;
}

static int set_chargen_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_name, val)) {
        return 0;
    }

    return cbm2rom_load_chargen(chargen_name);
}

static int set_kernal_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_rom_name, val)) {
        return 0;
    }

    return cbm2rom_load_kernal(kernal_rom_name);
}

static int set_basic_rom_name(const char *val, void *param)
{
    if (util_string_set(&basic_rom_name, val)) {
        return 0;
    }

    return cbm2rom_load_basic(basic_rom_name);
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

static int cbm5x0_set_sync_factor(int val, void *param)
{
    int change_timing = 0;

    if (sync_factor != val) {
        change_timing = 1;
    }

    switch (val) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_NTSC:
            break;
        default:
            return -1;
    }

    sync_factor = val;
    if (change_timing) {
        if (power_freq > 0) {
            machine_change_timing(val, power_freq, vicii_resources.border_mode);
        }
    }
    return 0;
}

static int set_power_freq(int val, void *param)
{
    int change_timing = 0;

    if (power_freq != val) {
        change_timing = 1;
    }

    switch (val) {
        case 50:
        case 60:
            break;
        default:
            return -1;
    }

    power_freq = val;
    if (change_timing) {
        if (sync_factor > 0) {
            machine_change_timing(sync_factor, val, vicii_resources.border_mode);
        }
    }

    return 0;
}

static const resource_string_t cbm5x0_resources_string[] = {
    { "ChargenName", CBM2_CHARGEN500_NAME, RES_EVENT_NO, NULL,
      &chargen_name, set_chargen_rom_name, NULL },
    { "KernalName", CBM2_KERNAL500_NAME, RES_EVENT_NO, NULL,
      &kernal_rom_name, set_kernal_rom_name, NULL },
    { "BasicName", CBM2_BASIC500_NAME, RES_EVENT_NO, NULL,
      &basic_rom_name, set_basic_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};

#include "cbm2-common-resources.c"

static const resource_int_t cbm5x0_resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, cbm5x0_set_sync_factor, NULL },
    { "MachinePowerFrequency", 50, RES_EVENT_SAME, NULL,
      &power_freq, set_power_freq, NULL },
    { "RamSize", 64, RES_EVENT_SAME, NULL,
      &ramsize, set_ramsize, NULL },
    { "ModelLine", LINE_6x0_50HZ, RES_EVENT_SAME, NULL,
      &cbm2_model_line, set_cbm2_model_line, NULL },
    RESOURCE_INT_LIST_END
};

int cbm2_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    if (resources_register_string(cbm5x0_resources_string) < 0) {
        return -1;
    }
    return resources_register_int(cbm5x0_resources_int);
}

void cbm2_resources_shutdown(void)
{
    lib_free(kernal_rom_name);
    lib_free(chargen_name);
    lib_free(basic_rom_name);
}
