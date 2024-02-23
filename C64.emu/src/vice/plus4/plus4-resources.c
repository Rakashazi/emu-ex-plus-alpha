/*
 * plus4-resources.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "mem.h"
#include "plus4-resources.h"
#include "plus4mem.h"
#include "plus4memrom.h"
#include "plus4memcsory256k.h"
#include "plus4memhacks.h"
#include "plus4memhannes256k.h"
#include "plus4rom.h"
#include "plus4cart.h"
#include "resources.h"
#include "util.h"
#include "vsync.h"
#include "ted-resources.h"

/* What sync factor between the CPU and the drive?  If equal to
   `MACHINE_SYNC_PAL', the same as PAL machines.  If equal to
   `MACHINE_SYNC_NTSC', the same as NTSC machines.  The sync factor is
   calculated as 65536 * drive_clk / clk_[main machine] */
static int sync_factor;

#if 0
/* Frequency of the power grid in Hz */
static int power_freq = 1;
#endif

/* Name of the BASIC ROM.  */
static char *basic_rom_name = NULL;

/* Name of the Kernal ROM.  */
static char *kernal_rom_name = NULL;

/* Name of the Function (3plus1) ROMs.  */
static char *func_lo_rom_name = NULL;
static char *func_hi_rom_name = NULL;

/* FIXME: c2lo/hi can be external or internal  */
static char *c2lo_rom_name = NULL;
static char *c2hi_rom_name = NULL;

/* Size of RAM installed in kbytes */
static int ram_size_plus4 = 64;

static int set_kernal_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_rom_name, val)) {
        return 0;
    }

    return plus4rom_load_kernal(kernal_rom_name);
}

static int set_basic_rom_name(const char *val, void *param)
{
    if (util_string_set(&basic_rom_name, val)) {
        return 0;
    }

    return plus4rom_load_basic(basic_rom_name);
}

static int set_func_lo_rom_name(const char *val, void *param)
{
    if (util_string_set(&func_lo_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_func_lo(func_lo_rom_name);
}

static int set_func_hi_rom_name(const char *val, void *param)
{
    if (util_string_set(&func_hi_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_func_hi(func_hi_rom_name);
}

static int set_c2lo_rom_name(const char *val, void *param)
{
    if (util_string_set(&c2lo_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_c2lo(c2lo_rom_name);
}

static int set_c2hi_rom_name(const char *val, void *param)
{
    if (util_string_set(&c2hi_rom_name, val)) {
        return 0;
    }

    return plus4cart_load_c2hi(c2hi_rom_name);
}

static int set_ram_size_plus4(int rs, void *param)
{
    int hack;

    /* FIXME: this doesn't do anything -- compyx, 2017-10-05 */
    switch (rs) {
        case 16:
        case 32:
        case 64:
        case 256:
        case 1024:
        case 4096:
            break;
    }

    ram_size_plus4 = rs;

    if (ram_size_plus4 <= 64) {
        resources_get_int("MemoryHack", &hack);
        if (hack) {
            resources_set_int("MemoryHack", 0);
        }
    }

    vsync_suspend_speed_eval();
    mem_initialize_memory();
    machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);

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
                machine_change_timing(MACHINE_SYNC_PAL, 50, ted_resources.border_mode);
            }
            break;
        case MACHINE_SYNC_NTSC:
            sync_factor = val;
            if (change_timing) {
                machine_change_timing(MACHINE_SYNC_NTSC, 60, ted_resources.border_mode);
            }
            break;
        default:
            return -1;
    }
    return 0;
}

#if 0
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
            machine_change_timing(sync_factor, val, ted_resources.border_mode);
        }
    }

    return 0;
}
#endif

static const resource_string_t resources_string[] = {
    { "KernalName", PLUS4_KERNAL_PAL_REV5_NAME, RES_EVENT_NO, NULL,
      &kernal_rom_name, set_kernal_rom_name, NULL },
    { "BasicName", PLUS4_BASIC_NAME, RES_EVENT_NO, NULL,
      &basic_rom_name, set_basic_rom_name, NULL },
    { "FunctionLowName", PLUS4_3PLUS1LO_NAME, RES_EVENT_NO, NULL,
      &func_lo_rom_name, set_func_lo_rom_name, NULL },
    { "FunctionHighName", PLUS4_3PLUS1HI_NAME, RES_EVENT_NO, NULL,
      &func_hi_rom_name, set_func_hi_rom_name, NULL },
    { "c2loName", "", RES_EVENT_NO, NULL,
      &c2lo_rom_name, set_c2lo_rom_name, NULL },
    { "c2hiName", "", RES_EVENT_NO, NULL,
      &c2hi_rom_name, set_c2hi_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
#if 0
    { "MachinePowerFrequency", 50, RES_EVENT_SAME, NULL,
      &power_freq, set_power_freq, NULL },
#endif
    { "RamSize", 64, RES_EVENT_SAME, NULL,
      &ram_size_plus4, set_ram_size_plus4, NULL },
    RESOURCE_INT_LIST_END
};

int plus4_resources_init(void)
{
    if (plus4_memory_hacks_resources_init() < 0) {
        return -1;
    }
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void plus4_resources_shutdown(void)
{
    lib_free(basic_rom_name);
    lib_free(kernal_rom_name);
    lib_free(func_lo_rom_name);
    lib_free(func_hi_rom_name);
    lib_free(c2lo_rom_name);
    lib_free(c2hi_rom_name);
}
