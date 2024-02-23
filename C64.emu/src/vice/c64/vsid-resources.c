/*
 * vsid-resources.c
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
#include <string.h>

#include "c64-resources.h"
#include "c64acia.h"
#include "c64cart.h"
#include "c64cia.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "cartio.h"
#include "cartridge.h"
#include "cia.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "reu.h"
#include "georam.h"
#include "sid-resources.h"
#include "util.h"
#include "vicii-resources.h"
#include "vicii.h"
#include "c64fastiec.h"
#include "hvsc.h"
#include "archdep.h"

/* force  commit */

#define KBD_INDEX_C64_SYM   0
#define KBD_INDEX_C64_POS   1
#define KBD_INDEX_C64_SYMDE 2

/* What sync factor between the CPU and the drive?  If equal to
   `MACHINE_SYNC_PAL', the same as PAL machines.  If equal to
   `MACHINE_SYNC_NTSC', the same as NTSC machines.  The sync factor is
   calculated as 65536 * drive_clk / clk_[main machine] */
static int sync_factor;

#if 0
/* Frequency of the power grid in Hz */
static int power_freq = 1;
#endif

/* Name of the character ROM.  */
static char *chargen_rom_name = NULL;

/* Name of the BASIC ROM.  */
static char *basic_rom_name = NULL;

/* Name of the Kernal ROM.  */
static char *kernal_rom_name = NULL;

/** \brief  Root directory of the High Voltage collection
 */
static char *hvsc_root = NULL;


/* Kernal revision for ROM patcher.  */
int kernal_revision = C64_KERNAL_REV3;

int cia1_model = CIA_MODEL_6526;
int cia2_model = CIA_MODEL_6526;

static int set_chargen_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_rom_name, val)) {
        return 0;
    }

    return c64rom_load_chargen(chargen_rom_name);
}

static int set_kernal_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_rom_name, val)) {
        return 0;
    }

    /* load kernal without a kernal overriding buffer */
    return c64rom_load_kernal(kernal_rom_name, NULL);
}

static int set_basic_rom_name(const char *val, void *param)
{
    if (util_string_set(&basic_rom_name, val)) {
        return 0;
    }

    return c64rom_load_basic(basic_rom_name);
}

struct kernal_s {
    const char *name;
    int rev;
};

static struct kernal_s kernal_match[] = {
    { C64_KERNAL_REV1_NAME, C64_KERNAL_REV1 },
    { C64_KERNAL_REV2_NAME, C64_KERNAL_REV2 },
    { C64_KERNAL_REV3_NAME, C64_KERNAL_REV3 },
    { C64_KERNAL_JAP_NAME,  C64_KERNAL_JAP },
    { C64_KERNAL_SX64_NAME, C64_KERNAL_SX64 },
    { C64_KERNAL_GS64_NAME, C64_KERNAL_GS64 },
    { C64_KERNAL_4064_NAME, C64_KERNAL_4064 },
    { C64_KERNAL_NONE_NAME, C64_KERNAL_NONE },
    { NULL, C64_KERNAL_UNKNOWN }
};

static int set_kernal_revision(int val, void *param)
{
    int n = 0, rev = C64_KERNAL_UNKNOWN;
    const char *name = NULL;
    log_verbose("set_kernal_revision was kernal_revision: %d new val:%d", kernal_revision, val);

    if (val == C64_KERNAL_UNKNOWN) {
        kernal_revision = C64_KERNAL_UNKNOWN;
        return 0;
    }

    /* find given revision */
    do {
        if (kernal_match[n].rev == val) {
            rev = kernal_match[n].rev;
            name = kernal_match[n].name;
        }
        ++n;
    } while ((rev == C64_KERNAL_UNKNOWN) && (kernal_match[n].name != NULL));

    if (rev == C64_KERNAL_UNKNOWN) {
        log_error(LOG_DEFAULT, "invalid kernal revision (%d)", val);
        return -1;
    }

    log_verbose("set_kernal_revision found rev:%d name: %s", rev, name);

    if (resources_set_string("KernalName", name) < 0) {
        log_error(LOG_DEFAULT, "failed to set kernal name (%s)", name);
        return -1;
    }

    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);

    if (kernal_revision != rev) {
        machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
    }

    kernal_revision = rev;
    log_verbose("set_kernal_revision new kernal_revision: %d", kernal_revision);
    return 0;
}

static int set_sync_factor(int val, void *param)
{
    int change_timing = 0;
    int pf;

    if (sync_factor != val) {
        change_timing = 1;
    }

    switch (val) {
        case MACHINE_SYNC_PAL:
        case MACHINE_SYNC_PALN:
            pf = 50;
            break;
        case MACHINE_SYNC_NTSC:
        case MACHINE_SYNC_NTSCOLD:
            pf = 60;
            break;
        default:
            return -1;
    }
    sync_factor = val;
    if (change_timing) {
        machine_change_timing(val, pf, 0);
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
            machine_change_timing(sync_factor, val, 0);
        }
    }
    return 0;
}
#endif

static int set_hvsc_root(const char *path, void *param)
{
    char *result = NULL;

    /* empty means use the 'HVSC_BASE' env var */
    if (path != NULL && *path != '\0') {
        /* expand ~, no effect on Windows */
        archdep_expand_path(&result, path);
    }

    util_string_set(&hvsc_root, result);

    /* "reboot" hvsclib */
    hvsc_exit();
    hvsc_init(result);
    lib_free(result);
    return 0;
}


static const resource_string_t resources_string[] = {
    { "ChargenName", C64_CHARGEN_NAME, RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &chargen_rom_name, set_chargen_rom_name, NULL },
    { "KernalName", C64_KERNAL_REV3_NAME, RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &kernal_rom_name, set_kernal_rom_name, NULL },
    { "BasicName", C64_BASIC_NAME, RES_EVENT_NO, NULL,
      /* FIXME: should be same but names may differ */
      &basic_rom_name, set_basic_rom_name, NULL },
    /* When empty the HVSC_BASE env var is used */
    { "HVSCRoot", "", RES_EVENT_NO, NULL,
      &hvsc_root, set_hvsc_root, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
#if 0
    { "MachinePowerFrequency", 50, RES_EVENT_SAME, NULL,
      &power_freq, set_power_freq, NULL },
#endif
    { "KernalRev", C64_KERNAL_REV3, RES_EVENT_SAME, NULL,
      &kernal_revision, set_kernal_revision, NULL },
    { "Sid2AddressStart", 0xde00, RES_EVENT_SAME, NULL,
      (int *)&sid2_address_start, sid_set_sid2_address, NULL },
    { "Sid3AddressStart", 0xdf00, RES_EVENT_SAME, NULL,
      (int *)&sid3_address_start, sid_set_sid3_address, NULL },
    { "Sid4AddressStart", 0xdf80, RES_EVENT_SAME, NULL,
      (int *)&sid4_address_start, sid_set_sid4_address, NULL },
    { "Sid5AddressStart", 0xde80, RES_EVENT_SAME, NULL,
      (int *)&sid5_address_start, sid_set_sid5_address, NULL },
    { "Sid6AddressStart", 0xdf40, RES_EVENT_SAME, NULL,
      (int *)&sid6_address_start, sid_set_sid6_address, NULL },
    { "Sid7AddressStart", 0xde40, RES_EVENT_SAME, NULL,
      (int *)&sid7_address_start, sid_set_sid7_address, NULL },
    { "Sid8AddressStart", 0xdfc0, RES_EVENT_SAME, NULL,
      (int *)&sid8_address_start, sid_set_sid8_address, NULL },
    RESOURCE_INT_LIST_END
};

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
    if (hvsc_root != NULL) {
        lib_free(hvsc_root);
    }
    hvsc_exit();
}
