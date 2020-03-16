/*
 * c128-resources.c
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

#include "c128-resources.h"
#include "c128.h"
#include "c128model.h"
#include "c64cia.h"
#include "c128mem.h"
#include "c128rom.h"
#include "cia.h"
#include "kbd.h"
#include "keyboard.h"
#include "lib.h"
#include "machine.h"
#include "reu.h"
#include "resources.h"
#include "sid-resources.h"
#include "vicii-resources.h"
#include "vicii.h"
#include "util.h"

/* What sync factor between the CPU and the drive?  If equal to
   `MACHINE_SYNC_PAL', the same as PAL machines.  If equal to
   `MACHINE_SYNC_NTSC', the same as NTSC machines.  The sync factor is
   calculated as 65536 * drive_clk / clk_[main machine] */
static int sync_factor;

/* Type of machine.  */
static int machine_type;

/* Name of the international character ROM.  */
static char *chargen_int_rom_name = NULL;

/* Name of the German character ROM.  */
static char *chargen_de_rom_name = NULL;

/* Name of the French character ROM.  */
static char *chargen_fr_rom_name = NULL;

/* Name of the Swedish character ROM.  */
static char *chargen_se_rom_name = NULL;

/* Name of the Swiss character ROM.  */
static char *chargen_ch_rom_name = NULL;

/* Name of the Norwegian character ROM.  */
static char *chargen_no_rom_name = NULL;

/* Name of the BASIC LO ROM.  */
static char *basiclo_rom_name = NULL;

/* Name of the BASIC HI ROM.  */
static char *basichi_rom_name = NULL;

/* Name of the international Kernal ROM.  */
static char *kernal_int_rom_name = NULL;

/* Name of the German Kernal ROM.  */
static char *kernal_de_rom_name = NULL;

/* Name of the Finnish Kernal ROM.  */
static char *kernal_fi_rom_name = NULL;

/* Name of the French Kernal ROM.  */
static char *kernal_fr_rom_name = NULL;

/* Name of the Italian Kernal ROM.  */
static char *kernal_it_rom_name = NULL;

/* Name of the Norwegian Kernal ROM.  */
static char *kernal_no_rom_name = NULL;

/* Name of the Swedish Kernal ROM.  */
static char *kernal_se_rom_name = NULL;

/* Name of the Swiss Kernal ROM.  */
static char *kernal_ch_rom_name = NULL;

/* Name of the BASIC ROM.  */
static char *basic64_rom_name = NULL;

/* Name of the Kernal ROM.  */
static char *kernal64_rom_name = NULL;

/* Flag: Do we enable the emulation of banks 2 and 3 of ram? */
int c128_full_banks;

/* Flag: Emulate new CIA */
int cia1_model = CIA_MODEL_6526A;
int cia2_model = CIA_MODEL_6526A;

static int board_type = BOARD_C128D;

static int set_c128_full_banks(int val, void *param)
{
    c128_full_banks = val ? 1 : 0;

    return 0;
}

static int set_machine_type(int val, void *param)
{
    unsigned int type = (unsigned int)val;

    switch (val) {
        case C128_MACHINE_INT:
        case C128_MACHINE_FINNISH:
        case C128_MACHINE_FRENCH:
        case C128_MACHINE_GERMAN:
        case C128_MACHINE_ITALIAN:
        case C128_MACHINE_NORWEGIAN:
        case C128_MACHINE_SWEDISH:
        case C128_MACHINE_SWISS:
            break;
        default:
            return -1;
    }

    machine_type = type;

    mem_set_machine_type(type);

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
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

static int set_chargen_int_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_int_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_int(chargen_int_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_chargen_de_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_de_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_de(chargen_de_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_chargen_fr_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_fr_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_fr(chargen_fr_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_chargen_se_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_se_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_se(chargen_se_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_chargen_ch_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_ch_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_ch(chargen_ch_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_chargen_no_rom_name(const char *val, void *param)
{
    if (util_string_set(&chargen_no_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_chargen_no(chargen_no_rom_name) < 0) {
        return -1;
    }

    if (c128rom_chargen_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_int_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_int_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_int(kernal_int_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_de_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_de_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_de(kernal_de_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_fi_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_fi_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_fi(kernal_fi_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_fr_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_fr_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_fr(kernal_fr_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_it_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_it_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_it(kernal_it_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_no_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_no_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_no(kernal_no_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_se_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_se_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_se(kernal_se_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_kernal_ch_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal_ch_rom_name, val)) {
        return 0;
    }

    if (c128rom_load_kernal_ch(kernal_ch_rom_name) < 0) {
        return -1;
    }

    if (c128rom_kernal_setup() < 0) {
        return -1;
    }

    return 0;
}

static int set_basiclo_rom_name(const char *val, void *param)
{
    if (util_string_set(&basiclo_rom_name, val)) {
        return 0;
    }

    return c128rom_load_basiclo(basiclo_rom_name);
}

static int set_basichi_rom_name(const char *val, void *param)
{
    if (util_string_set(&basichi_rom_name, val)) {
        return 0;
    }

    return c128rom_load_basichi(basichi_rom_name);
}

static int set_kernal64_rom_name(const char *val, void *param)
{
    if (util_string_set(&kernal64_rom_name, val)) {
        return 0;
    }

    return c128rom_load_kernal64(kernal64_rom_name, NULL);
}

static int set_basic64_rom_name(const char *val, void *param)
{
    if (util_string_set(&basic64_rom_name, val)) {
        return 0;
    }

    return c128rom_load_basic64(basic64_rom_name);
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
        default:
            return -1;
    }

    return 0;
}

static const resource_string_t resources_string[] = {
    { "ChargenIntName", "chargen", RES_EVENT_NO, NULL,
      &chargen_int_rom_name, set_chargen_int_rom_name, NULL },
    { "ChargenDEName", "chargde", RES_EVENT_NO, NULL,
      &chargen_de_rom_name, set_chargen_de_rom_name, NULL },
    { "ChargenFRName", "chargfr", RES_EVENT_NO, NULL,
      &chargen_fr_rom_name, set_chargen_fr_rom_name, NULL },
    { "ChargenSEName", "chargse", RES_EVENT_NO, NULL,
      &chargen_se_rom_name, set_chargen_se_rom_name, NULL },
    { "ChargenCHName", "chargch", RES_EVENT_NO, NULL,
      &chargen_ch_rom_name, set_chargen_ch_rom_name, NULL },
    { "ChargenNOName", "chargno", RES_EVENT_NO, NULL,
      &chargen_no_rom_name, set_chargen_no_rom_name, NULL },
    { "KernalIntName", "kernal", RES_EVENT_NO, NULL,
      &kernal_int_rom_name, set_kernal_int_rom_name, NULL },
    { "KernalDEName", "kernalde", RES_EVENT_NO, NULL,
      &kernal_de_rom_name, set_kernal_de_rom_name, NULL },
    { "KernalFIName", "kernalfi", RES_EVENT_NO, NULL,
      &kernal_fi_rom_name, set_kernal_fi_rom_name, NULL },
    { "KernalFRName", "kernalfr", RES_EVENT_NO, NULL,
      &kernal_fr_rom_name, set_kernal_fr_rom_name, NULL },
    { "KernalITName", "kernalit", RES_EVENT_NO, NULL,
      &kernal_it_rom_name, set_kernal_it_rom_name, NULL },
    { "KernalNOName", "kernalno", RES_EVENT_NO, NULL,
      &kernal_no_rom_name, set_kernal_no_rom_name, NULL },
    { "KernalSEName", "kernalse", RES_EVENT_NO, NULL,
      &kernal_se_rom_name, set_kernal_se_rom_name, NULL },
    { "KernalCHName", "kernalch", RES_EVENT_NO, NULL,
      &kernal_ch_rom_name, set_kernal_ch_rom_name, NULL },
    { "BasicLoName", "basiclo", RES_EVENT_NO, NULL,
      &basiclo_rom_name, set_basiclo_rom_name, NULL },
    { "BasicHiName", "basichi", RES_EVENT_NO, NULL,
      &basichi_rom_name, set_basichi_rom_name, NULL },
    { "Kernal64Name", "kernal64", RES_EVENT_NO, NULL,
      &kernal64_rom_name, set_kernal64_rom_name, NULL },
    { "Basic64Name", "basic64", RES_EVENT_NO, NULL,
      &basic64_rom_name, set_basic64_rom_name, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "MachineVideoStandard", MACHINE_SYNC_PAL, RES_EVENT_SAME, NULL,
      &sync_factor, set_sync_factor, NULL },
    { "BoardType", BOARD_C128D, RES_EVENT_SAME, NULL,
      &board_type, set_board_type, NULL },
    { "MachineType", C128_MACHINE_INT, RES_EVENT_SAME, NULL,
      &machine_type, set_machine_type, NULL },
    { "CIA1Model", CIA_MODEL_6526A, RES_EVENT_SAME, NULL,
      &cia1_model, set_cia1_model, NULL },
    { "CIA2Model", CIA_MODEL_6526A, RES_EVENT_SAME, NULL,
      &cia2_model, set_cia2_model, NULL },
    { "SidStereoAddressStart", 0xde00, RES_EVENT_SAME, NULL,
      (int *)&sid_stereo_address_start, sid_set_sid_stereo_address, NULL },
    { "SidTripleAddressStart", 0xdf00, RES_EVENT_SAME, NULL,
      (int *)&sid_triple_address_start, sid_set_sid_triple_address, NULL },
    { "SidQuadAddressStart", 0xdf80, RES_EVENT_SAME, NULL,
      (int *)&sid_quad_address_start, sid_set_sid_quad_address, NULL },
    { "C128FullBanks", 0, RES_EVENT_NO, NULL,
      (int *)&c128_full_banks, set_c128_full_banks, NULL },
    RESOURCE_INT_LIST_END
};

void c128_resources_update_cia_models(int model)
{
    set_cia1_model(model, NULL);
    set_cia2_model(model, NULL);
}

int c128_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void c128_resources_shutdown(void)
{
    lib_free(chargen_int_rom_name);
    lib_free(chargen_de_rom_name);
    lib_free(chargen_fr_rom_name);
    lib_free(chargen_se_rom_name);
    lib_free(chargen_ch_rom_name);
    lib_free(chargen_no_rom_name);
    lib_free(basiclo_rom_name);
    lib_free(basichi_rom_name);
    lib_free(kernal_int_rom_name);
    lib_free(kernal_de_rom_name);
    lib_free(kernal_fi_rom_name);
    lib_free(kernal_fr_rom_name);
    lib_free(kernal_it_rom_name);
    lib_free(kernal_no_rom_name);
    lib_free(kernal_se_rom_name);
    lib_free(kernal_ch_rom_name);
    lib_free(basic64_rom_name);
    lib_free(kernal64_rom_name);
}
