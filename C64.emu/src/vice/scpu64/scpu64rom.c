/*
 * scpu64rom.c
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

#include "scpu64-resources.h"
#include "scpu64mem.h"
#include "scpu64rom.h"
#include "log.h"
#include "mem.h"
#include "machine.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"

static log_t scpu64rom_log = LOG_ERR;

uint8_t scpu64rom_scpu64_rom[SCPU64_SCPU64_ROM_MAXSIZE];

/* added to resolve linking issues with LTK. LTK will disable itself if
   xscpu64 attempts to attach to it. */
uint8_t *c64memrom_basic64_rom = NULL;
uint8_t *c64memrom_kernal64_rom = NULL;
uint8_t *c64memrom_kernal64_trap_rom = NULL;

/* Flag: nonzero if the ROMs have been loaded.  */
static int rom_loaded = 0;

int scpu64rom_load_chargen(const char *rom_name)
{
    if (!rom_loaded) {
        return 0;
    }

    /* Load chargen ROM.  */

    if (sysfile_load(rom_name, machine_name, mem_chargen_rom, SCPU64_CHARGEN_ROM_SIZE, SCPU64_CHARGEN_ROM_SIZE) < 0) {
        log_error(scpu64rom_log, "Couldn't load character ROM `%s'.", rom_name);
        return -1;
    }

    return 0;
}

int scpu64rom_load_scpu64(const char *rom_name)
{
    int size, i;

    if (!rom_loaded) {
        return 0;
    }

    /* Load SCPU64 ROM.  */
    size = sysfile_load(rom_name, machine_name, scpu64rom_scpu64_rom, SCPU64_SCPU64_ROM_MINSIZE, SCPU64_SCPU64_ROM_MAXSIZE);
    if (size < 0 || (size & (size - 1))) {
        log_error(scpu64rom_log, "Couldn't load SCPU64 ROM `%s'.", rom_name);
        return -1;
    }
    for (i = 0 ;i < SCPU64_SCPU64_ROM_MAXSIZE - size; i += size) {
        memcpy(scpu64rom_scpu64_rom + i, scpu64rom_scpu64_rom + SCPU64_SCPU64_ROM_MAXSIZE - size, size);
    }
    /* FIXME: copy kernal into the trap ram, else the checkbytes can not be found */
    memcpy(mem_trap_ram, scpu64rom_scpu64_rom + 0x4100, 0x2000);
    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (scpu64rom_log == LOG_ERR) {
        scpu64rom_log = log_open("SCPU64MEM");
    }

    rom_loaded = 1;

    if (resources_get_string("ChargenName", &rom_name) < 0) {
        return -1;
    }
    if (scpu64rom_load_chargen(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("SCPU64Name", &rom_name) < 0) {
        return -1;
    }
    if (scpu64rom_load_scpu64(rom_name) < 0) {
        return -1;
    }

    return 0;
}
