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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "c64-resources.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64rom.h"
#include "log.h"
#include "mem.h"
#include "patchrom.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"

static log_t c64rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
static int rom_loaded = 0;

int c64rom_get_kernal_checksum(void)
{
    int i;
    WORD sum;                   /* ROM checksum */
    int id;                     /* ROM identification number */

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < C64_KERNAL_ROM_SIZE; i++) {
        sum += c64memrom_kernal64_rom[i];
    }

    id = c64memrom_rom64_read(0xff80);

    log_message(c64rom_log, "Kernal rev #%d.", id);

    if ((id == 0 && sum != C64_KERNAL_CHECKSUM_R00)
        || (id == 3 && sum != C64_KERNAL_CHECKSUM_R03 && sum != C64_KERNAL_CHECKSUM_R03swe)
        || (id == 0x43 && sum != C64_KERNAL_CHECKSUM_R43)
        || (id == 0x64 && sum != C64_KERNAL_CHECKSUM_R64)) {
        log_warning(c64rom_log, "Warning: Unknown Kernal image.  Sum: %d ($%04X).", sum, sum);
    } else if (kernal_revision != NULL) {
        if (patch_rom(kernal_revision) < 0) {
            return -1;
        }
    }
    return 0;
}

int c64rom_cartkernal_active = 0;

/* the extra parameter cartkernal is used to replace the kernal
   with a cartridge kernal rom image, if it is NULL normal kernal
   is used */
int c64rom_load_kernal(const char *rom_name, BYTE *cartkernal)
{
    int trapfl;

    if (!rom_loaded) {
        return 0;
    }

    /* Make sure serial code assumes there are no traps installed.  */
    /* serial_remove_traps(); */
    /* we also need the TAPE traps!!! therefore -> */
    /* disable traps before saving the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 1);

    /* Load Kernal ROM.  */
    if (cartkernal == NULL) {
        if (c64rom_cartkernal_active == 1) {
            return -1;
        }

        if (sysfile_load(rom_name, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE, C64_KERNAL_ROM_SIZE) < 0) {
            log_error(c64rom_log, "Couldn't load kernal ROM `%s'.", rom_name);
            resources_set_int("VirtualDevices", trapfl);
            return -1;
        }
    } else {
        memcpy(c64memrom_kernal64_rom, cartkernal, 0x2000);
        c64rom_cartkernal_active = 1;
    }
    c64rom_get_kernal_checksum();
    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);

    resources_set_int("VirtualDevices", trapfl);

    return 0;
}

int c64rom_get_basic_checksum(void)
{
    int i;
    WORD sum;

    /* Check Basic ROM.  */

    for (i = 0, sum = 0; i < C64_BASIC_ROM_SIZE; i++) {
        sum += c64memrom_basic64_rom[i];
    }

    if (sum != C64_BASIC_CHECKSUM) {
        log_warning(c64rom_log, "Warning: Unknown Basic image.  Sum: %d ($%04X).", sum, sum);
    }

    return 0;
}

int c64rom_load_basic(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    /* Load Basic ROM.  */
    if (sysfile_load(rom_name, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE, C64_BASIC_ROM_SIZE) < 0) {
        log_error(c64rom_log, "Couldn't load basic ROM `%s'.", rom_name);
        return -1;
    }
    return c64rom_get_basic_checksum();
}

int c64rom_load_chargen(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    /* Load chargen ROM.  */

    if (sysfile_load(rom_name, mem_chargen_rom, C64_CHARGEN_ROM_SIZE, C64_CHARGEN_ROM_SIZE) < 0) {
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
