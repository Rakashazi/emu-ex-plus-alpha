/*
 * c128rom.c
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

#include "c128.h"
#include "c128mem.h"
#include "c128memrom.h"
#include "c128rom.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "mem.h"
#include "log.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"
#include "z80mem.h"

static log_t c128rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
static int rom_loaded = 0;

#ifdef USE_EMBEDDED
#include "c128kernal.h"
#include "c128kernalde.h"
#include "c128kernalfi.h"
#include "c128kernalfr.h"
#include "c128kernalit.h"
#include "c128kernalno.h"
#include "c128kernalse.h"
#include "c128kernalch.h"

#include "c128chargde.h"
#include "c128chargen.h"
#include "c128chargfr.h"
#include "c128chargse.h"
#include "c128chargch.h"
#else
/* National Kernal ROM images. */
static BYTE kernal_int[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_de[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_fi[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_fr[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_it[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_no[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_se[C128_KERNAL_ROM_IMAGE_SIZE];
static BYTE kernal_ch[C128_KERNAL_ROM_IMAGE_SIZE];

/* National Chargen ROM images. */
static BYTE chargen_int[C128_CHARGEN_ROM_SIZE];
static BYTE chargen_de[C128_CHARGEN_ROM_SIZE];
static BYTE chargen_fr[C128_CHARGEN_ROM_SIZE];
static BYTE chargen_se[C128_CHARGEN_ROM_SIZE];
static BYTE chargen_ch[C128_CHARGEN_ROM_SIZE];
#endif

int c128rom_kernal_checksum(void)
{
    int i, id;
    WORD sum;

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < C128_KERNAL_ROM_SIZE; i++) {
        sum += c128memrom_kernal_rom[i];
    }

    id = c128memrom_rom_read(0xff80);

    log_message(c128rom_log, "Kernal rev #%d.", id);
    if (id == 1 && sum != C128_KERNAL_CHECKSUM_R01 && sum != C128_KERNAL_CHECKSUM_R01SWE && sum != C128_KERNAL_CHECKSUM_R01GER) {
        log_error(c128rom_log, "Warning: Kernal image may be corrupted. Sum: %d.", sum);
    }
    return 0;
}

int c128rom_load_kernal_int(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load international Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_int, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_de(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load German Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_de, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_fi(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Finnish Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_fi, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_fr(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load French Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_fr, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_it(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Italian Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_it, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_no(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Norwegian Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_no, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_se(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Swedish Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_se, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_kernal_ch(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Swiss Kernal ROM.  */
        if (sysfile_load(rom_name, kernal_ch, C128_KERNAL_ROM_IMAGE_SIZE, C128_KERNAL_ROM_IMAGE_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_kernal_setup(void)
{
    int trapfl, machine_type;
    BYTE *kernal = NULL;

    if (!rom_loaded) {
        return 0;
    }

    resources_get_int("MachineType", &machine_type);

    switch (machine_type) {
        case C128_MACHINE_INT:
            kernal = kernal_int;
            break;
        case C128_MACHINE_FINNISH:
            kernal = kernal_fi;
            break;
        case C128_MACHINE_FRENCH:
            kernal = kernal_fr;
            break;
        case C128_MACHINE_GERMAN:
            kernal = kernal_de;
            break;
        case C128_MACHINE_ITALIAN:
            kernal = kernal_it;
            break;
        case C128_MACHINE_NORWEGIAN:
            kernal = kernal_no;
            break;
        case C128_MACHINE_SWEDISH:
            kernal = kernal_se;
            break;
        case C128_MACHINE_SWISS:
            kernal = kernal_ch;
            break;
        default:
            log_error(c128rom_log, "Unknown machine type %i.", machine_type);
            return -1;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    memcpy(&c128memrom_basic_rom[C128_BASIC_ROM_SIZE], kernal, C128_EDITOR_ROM_SIZE);
    memcpy(z80bios_rom, &kernal[C128_EDITOR_ROM_SIZE], C128_Z80BIOS_ROM_SIZE);
    memcpy(c128memrom_kernal_rom, &kernal[C128_EDITOR_ROM_SIZE + C128_Z80BIOS_ROM_SIZE], C128_KERNAL_ROM_SIZE);
    memcpy(c128memrom_kernal_trap_rom, c128memrom_kernal_rom, C128_KERNAL_ROM_SIZE);

    c128rom_kernal_checksum();

    resources_set_int("VirtualDevices", trapfl);

    return 0;
}

int c128rom_basic_checksum(void)
{
    int i, id;
    WORD sum;

    /* Check Basic ROM.  */
    for (i = 0, sum = 0; i < C128_BASIC_ROM_SIZE; i++) {
        sum += c128memrom_basic_rom[i];
    }

    if (sum != C128_BASIC_CHECKSUM_85 && sum != C128_BASIC_CHECKSUM_86) {
        log_error(c128rom_log, "Warning: Unknown Basic image.  Sum: %d ($%04X).", sum, sum);
    }

    /* Check Editor ROM.  */
    for (i = C128_BASIC_ROM_SIZE, sum = 0; i < C128_BASIC_ROM_SIZE + C128_EDITOR_ROM_SIZE; i++) {
        sum += c128memrom_basic_rom[i];
    }

    id = c128memrom_rom_read(0xff80);
    if (id == 01 && sum != C128_EDITOR_CHECKSUM_R01 && sum != C128_EDITOR_CHECKSUM_R01SWE && sum != C128_EDITOR_CHECKSUM_R01GER) {
        log_error(c128rom_log, "Warning: EDITOR image may be corrupted. Sum: %d.", sum);
        log_error(c128rom_log, "Check your Basic ROM.");
    }
    return 0;
}

int c128rom_load_basiclo(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Basic ROM.  */
        if (sysfile_load(rom_name, c128memrom_basic_rom, C128_BASIC_ROM_IMAGELO_SIZE, C128_BASIC_ROM_IMAGELO_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load basic ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_basichi(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Basic ROM.  */
        if (sysfile_load(rom_name, &c128memrom_basic_rom[C128_BASIC_ROM_IMAGELO_SIZE], C128_BASIC_ROM_IMAGEHI_SIZE, C128_BASIC_ROM_IMAGEHI_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load basic ROM `%s'.", rom_name);
            return -1;
        }
    }
    return c128rom_basic_checksum();
}

int c128rom_chargen_setup(void)
{
    int machine_type;
    BYTE *chargen;

    if (!rom_loaded) {
        return 0;
    }

    resources_get_int("MachineType", &machine_type);

    switch (machine_type) {
        case C128_MACHINE_INT:
            chargen = chargen_int;
            break;
        case C128_MACHINE_FRENCH:
        case C128_MACHINE_ITALIAN:
            chargen = chargen_fr;
            break;
        case C128_MACHINE_GERMAN:
            chargen = chargen_de;
            break;
        case C128_MACHINE_FINNISH:
        case C128_MACHINE_NORWEGIAN:
        case C128_MACHINE_SWEDISH:
            chargen = chargen_se;
            break;
        case C128_MACHINE_SWISS:
            chargen = chargen_ch;
            break;
        default:
            log_error(c128rom_log, "Unknown machine type %i.", machine_type);
            return -1;
    }

    memcpy(mem_chargen_rom, chargen, C128_CHARGEN_ROM_SIZE);

    return 0;
}

int c128rom_load_chargen_int(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM.  */
        if (sysfile_load(rom_name, chargen_int, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_chargen_de(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM.  */
        if (sysfile_load(rom_name, chargen_de, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_chargen_fr(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM.  */
        if (sysfile_load(rom_name, chargen_fr, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_chargen_se(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM.  */
        if (sysfile_load(rom_name, chargen_se, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c128rom_load_chargen_ch(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM.  */
        if (sysfile_load(rom_name, chargen_ch, C128_CHARGEN_ROM_SIZE, C128_CHARGEN_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load character ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int c64rom_cartkernal_active = 0;

int c128rom_load_kernal64(const char *rom_name, BYTE *cartkernal)
{
    if (!rom_loaded) {
        return 0;
    }

    if (cartkernal == NULL) {
        if (c64rom_cartkernal_active == 1) {
            return -1;
        }

        if (!util_check_null_string(rom_name)) {
            /* Load C64 kernal ROM.  */
            if (sysfile_load(rom_name, c64memrom_kernal64_rom, C128_KERNAL64_ROM_SIZE, C128_KERNAL64_ROM_SIZE) < 0) {
                log_error(c128rom_log, "Couldn't load C64 kernal ROM `%s'.", rom_name);
                return -1;
            }
        }
    } else {
        memcpy(c64memrom_kernal64_rom, cartkernal, 0x2000);
        c64rom_cartkernal_active = 1;
    }
    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C128_KERNAL64_ROM_SIZE);
    return 0;
}

int c128rom_load_basic64(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load basic ROM.  */
        if (sysfile_load(rom_name, c64memrom_basic64_rom, C128_BASIC64_ROM_SIZE, C128_BASIC64_ROM_SIZE) < 0) {
            log_error(c128rom_log, "Couldn't load C64 basic ROM `%s'.", rom_name);
            return -1;
        }
    }
    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (c128rom_log == LOG_ERR) {
        c128rom_log = log_open("C128MEM");
    }

    mem_initialize_memory();

    rom_loaded = 1;

    if (resources_get_string("KernalIntName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_int(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalDEName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_de(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalFIName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_fi(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalFRName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_fr(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalITName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_it(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalNOName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_no(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalSEName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_se(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("KernalCHName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal_ch(rom_name) < 0) {
        return -1;
    }

    c128rom_kernal_setup();

    if (resources_get_string("BasicLoName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_basiclo(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("BasicHiName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_basichi(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenIntName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_chargen_int(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenDEName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_chargen_de(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenFRName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_chargen_fr(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenSEName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_chargen_se(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenCHName", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_chargen_ch(rom_name) < 0) {
        return -1;
    }

    c128rom_chargen_setup();

    if (resources_get_string("Kernal64Name", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_kernal64(rom_name, NULL) < 0) {
        return -1;
    }

    if (resources_get_string("Basic64Name", &rom_name) < 0) {
        return -1;
    }
    if (c128rom_load_basic64(rom_name) < 0) {
        return -1;
    }

    return 0;
}

int c64rom_load_kernal(const char *rom_name, BYTE *cartkernal)
{
    return c128rom_load_kernal64(rom_name, cartkernal);
}
