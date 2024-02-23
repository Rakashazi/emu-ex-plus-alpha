/*
 * c64rom.c
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

/* #define DEBUG_C64ROM */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "c64-resources.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "sha1.h"
#include "sysfile.h"
#include "types.h"

#ifdef DEBUG_C64ROM
#define LOG(x)  log_debug x
#else
#define LOG(x)
#endif

static log_t c64rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
static int rom_loaded = 0;

int c64rom_isloaded(void)
{
    return rom_loaded;
}

struct kernal_s {
    int id;         /* the value located at 0xff80 */
    int chksum;
    char *sha1hash;
    int rev;
};

/* NOTE: also update the table in c64-resources.c */
static struct kernal_s kernal_match[] = {
    { C64_KERNAL_ID_R01,    C64_KERNAL_CHECKSUM_R01,    C64_KERNAL_HASH_R01,    C64_KERNAL_REV1 },
    { C64_KERNAL_ID_R02,    C64_KERNAL_CHECKSUM_R02,    C64_KERNAL_HASH_R02,    C64_KERNAL_REV2 },
    { C64_KERNAL_ID_R03,    C64_KERNAL_CHECKSUM_R03,    C64_KERNAL_HASH_R03,    C64_KERNAL_REV3 },
/*  { C64_KERNAL_ID_R03swe, C64_KERNAL_CHECKSUM_R03swe, C64_KERNAL_HASH_R03swe, C64_KERNAL_REV3swe }, */
    { C64_KERNAL_ID_JAP,    C64_KERNAL_CHECKSUM_JAP,    C64_KERNAL_HASH_JAP,    C64_KERNAL_JAP },
    { C64_KERNAL_ID_R43,    C64_KERNAL_CHECKSUM_R43,    C64_KERNAL_HASH_R43,    C64_KERNAL_SX64 },
    { C64_KERNAL_ID_GS64,   C64_KERNAL_CHECKSUM_GS64,   C64_KERNAL_HASH_GS64,   C64_KERNAL_GS64 },
    { C64_KERNAL_ID_R64,    C64_KERNAL_CHECKSUM_R64,    C64_KERNAL_HASH_R64,    C64_KERNAL_4064 },
    { 0,                    0,                          NULL,                   C64_KERNAL_UNKNOWN }
};

int c64rom_get_kernal_chksum_id(uint16_t *sumout, int *idout, char *hash)
{
    int i;
    uint16_t sum;               /* ROM checksum */
    int id;                     /* ROM identification number */
    char sha1hash[41];

    /* special case to support MAX machine, if kernal is all zeros, return C64_KERNAL_NONE */
    for (i = 0, sum = 0; i < C64_KERNAL_ROM_SIZE; i++) {
        sum |= c64memrom_kernal64_rom[i];
    }
    if (sum == 0) {
        *sumout = 0; *idout = 0;
        LOG(("c64rom_get_kernal_chksum_id: C64_KERNAL_NONE"));
        return C64_KERNAL_NONE;
    }

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < C64_KERNAL_ROM_SIZE; i++) {
        sum += c64memrom_kernal64_rom[i];
    }
    /* get ID from Kernal ROM */
    id = c64memrom_rom64_read(0xff80);

    LOG(("c64rom_get_kernal_chksum_id chksum: %d id: %d", sum, id));

    if (sumout) {
        *sumout = sum;
    }
    if (idout) {
        *idout = id;
    }

    SHA1String(sha1hash, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
    LOG(("c64rom_get_kernal_chksum_id sha1: %s", sha1hash));

    if (hash) {
        strncpy(hash, sha1hash, 41);
    }

    /* find kernal that matches the SHA1 hash */
    i= 0; do {
        if (strncmp(kernal_match[i].sha1hash, sha1hash, 40) == 0) {
            LOG(("c64rom_get_kernal_chksum_id: rev:%d", kernal_match[i].rev));
            return kernal_match[i].rev;
        }
        i++;
    } while (kernal_match[i].rev != C64_KERNAL_UNKNOWN);

    LOG(("c64rom_get_kernal_chksum_id: C64_KERNAL_NONE"));
    return C64_KERNAL_UNKNOWN; /* unknown */
}

/* FIXME: this function has a misleading name, only called from snapshot stuff atm
          it was used to patch the kernal before, but not anymore
*/
int c64rom_print_kernal_info(void)
{
    uint16_t sum;               /* ROM checksum */
    int id;                     /* ROM identification number */
    int rev;                    /* detected revision */
    char hash[41];

    rev = c64rom_get_kernal_chksum_id(&sum, &id, hash);
    if (rev == C64_KERNAL_UNKNOWN) {
        log_warning(c64rom_log, "Unknown Kernal image.  ID: %d ($%02X) Sum: %d ($%04X) SHA1: %s.",
                    id, (unsigned int)id, sum, sum, hash);
        return rev;
    } else if (rev == C64_KERNAL_NONE) {
        /* MAX machine doesn't have a kernal */
        log_message(c64rom_log, "No Kernal image. No hash calculated.");
    } else {
        log_message(c64rom_log, "Kernal rev #%d ($%02X) Sum: %d ($%04X) SHA1: %s.",
                    id, (unsigned int)id, sum, sum, hash);
    }

    return rev;
}

int c64rom_cartkernal_active = 0;

#define NUM_TRAP_DEVICES 9  /* FIXME: is there a better constant ? */
static int trapfl[NUM_TRAP_DEVICES];
static int trapdevices[NUM_TRAP_DEVICES + 1] = { 1, 4, 5, 6, 7, 8, 9, 10, 11, -1 };

static void get_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_get_int_sprintf("VirtualDevice%d", &trapfl[i], trapdevices[i]);
    }
}

static void clear_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("VirtualDevice%d", 0, trapdevices[i]);
    }
}

static void restore_trapflags(void)
{
    int i;
    for(i = 0; trapdevices[i] != -1; i++) {
        resources_set_int_sprintf("VirtualDevice%d", trapfl[i], trapdevices[i]);
    }
}

/* the extra parameter cartkernal is used to replace the kernal
   with a cartridge kernal rom image, if it is NULL normal kernal
   is used */
int c64rom_load_kernal(const char *rom_name, uint8_t *cartkernal)
{
    int rev;

    LOG(("c64rom_load_kernal rom_name:%s", rom_name));

    if (!rom_loaded) {
        return 0;
    }

    /* disable traps before loading the ROM */
    if (machine_class != VICE_MACHINE_VSID) {
        get_trapflags();
        clear_trapflags();
    }

    /* Load Kernal ROM.  */
    if (cartkernal == NULL) {
        if (c64rom_cartkernal_active == 1) {
            if (machine_class != VICE_MACHINE_VSID) {
                restore_trapflags();
            }
            return -1;
        }

        if (strcmp(rom_name, C64_KERNAL_NONE_NAME) == 0) {
            /* special case handling for "no kernal" (MAX machine) */
            memset(c64memrom_kernal64_rom, 0, C64_KERNAL_ROM_SIZE);
        } else {
            if (sysfile_load(rom_name, machine_name, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE) < 0) {
                log_error(c64rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
                if (machine_class != VICE_MACHINE_VSID) {
                    restore_trapflags();
                }
                return -1;
            }
        }
    } else {
        memcpy(c64memrom_kernal64_rom, cartkernal, 0x2000);
        c64rom_cartkernal_active = 1;
    }

    rev = c64rom_print_kernal_info();
    if (machine_class != VICE_MACHINE_C64DTV) {
        resources_set_int("KernalRev", rev);
    }

    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);

    if (machine_class != VICE_MACHINE_VSID) {
        restore_trapflags();
    }

    return 0;
}

int c64rom_print_basic_info(void)
{
    int i;
    uint16_t sum;   /* ROM checksum */
    int rev = 0;    /* ROM revision */

    /* Check Basic ROM.  */

    for (i = 0, sum = 0; i < C64_BASIC_ROM_SIZE; i++) {
        sum += c64memrom_basic64_rom[i];
    }

    if (sum != C64_BASIC_CHECKSUM) {
        log_warning(c64rom_log, "Unknown Basic image.  Sum: %d ($%04X).", sum, sum);
    }

    return rev;
}

int c64rom_load_basic(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    /* Load Basic ROM.  */
    if (sysfile_load(rom_name, machine_name, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE) < 0) {
        log_error(c64rom_log, "Couldn't load basic ROM `%s'.", rom_name);
        return -1;
    }
    c64rom_print_basic_info();
    return 0;
}

int c64rom_load_chargen(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    /* Load chargen ROM.  */

    if (sysfile_load(rom_name, machine_name, mem_chargen_rom, C64_CHARGEN_ROM_SIZE, C64_CHARGEN_ROM_SIZE) < 0) {
        log_error(c64rom_log, "Couldn't load character ROM `%s'.", rom_name);
        return -1;
    }

    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (c64rom_log == LOG_ERR) {
        c64rom_log = log_open("C64MEM");
    }

    rom_loaded = 1;

    if (resources_get_string("KernalName", &rom_name) < 0) {
        return -1;
    }
    if (c64rom_load_kernal(rom_name, NULL) < 0) {
        return -1;
    }

    if (resources_get_string("BasicName", &rom_name) < 0) {
        return -1;
    }
    if (c64rom_load_basic(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenName", &rom_name) < 0) {
        return -1;
    }
    if (c64rom_load_chargen(rom_name) < 0) {
        return -1;
    }

    return 0;
}
