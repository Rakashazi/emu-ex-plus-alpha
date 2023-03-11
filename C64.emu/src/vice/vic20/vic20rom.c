/*
 * vic20rom.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "log.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"
#include "util.h"
#include "vic20mem.h"
#include "vic20memrom.h"
#include "vic20rom.h"


static log_t vic20rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
static int vicrom_loaded = 0;


int vic20rom_kernal_checksum(void)
{
    int i;
    uint16_t sum;

    /* Check Kernal ROM.  */
    for (i = 0, sum = 0; i < VIC20_KERNAL_ROM_SIZE; i++) {
        sum += vic20memrom_kernal_rom[i];
    }

    if ((sum != VIC20_KERNAL_REV2_CHECKSUM)
     && (sum != VIC20_KERNAL_REV6_CHECKSUM)
     && (sum != VIC20_KERNAL_REV7_CHECKSUM)) {
        log_warning(vic20rom_log,
                  "Unknown Kernal image.  Sum: %d ($%04X).",
                  sum, sum);
        return -1;
    }
    return 0;
}

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

int vic20rom_load_kernal(const char *rom_name)
{
    if (!vicrom_loaded) {
        return 0;
    }

    /* disable traps before loading the ROM */
    get_trapflags();
    clear_trapflags();

    /* Load Kernal ROM. */
    if (sysfile_load(rom_name,
                     machine_name,
                     vic20memrom_kernal_rom, VIC20_KERNAL_ROM_SIZE,
                     VIC20_KERNAL_ROM_SIZE) < 0) {
        log_error(vic20rom_log, "Couldn't load kernal ROM.");
        restore_trapflags();
        return -1;
    }

    vic20rom_kernal_checksum();
    memcpy(vic20memrom_kernal_trap_rom, vic20memrom_kernal_rom,
           VIC20_KERNAL_ROM_SIZE);

    restore_trapflags();

    return 0;
}

int vic20rom_basic_checksum(void)
{
    int i;
    uint16_t sum;

    /* Check Basic ROM. */
    for (i = 0, sum = 0; i < VIC20_BASIC_ROM_SIZE; i++) {
        sum += vic20memrom_basic_rom[i];
    }

    if (sum != VIC20_BASIC_CHECKSUM) {
        log_warning(vic20rom_log,
                  "Unknown Basic image.  Sum: %d ($%04X).",
                  sum, sum);
    }
    return 0;
}

int vic20rom_load_basic(const char *rom_name)
{
    if (!vicrom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load Basic ROM. */
        if (sysfile_load(rom_name,
                         machine_name,
                         vic20memrom_basic_rom, VIC20_BASIC_ROM_SIZE,
                         VIC20_BASIC_ROM_SIZE) < 0) {
            log_error(vic20rom_log, "Couldn't load basic ROM.");
            return -1;
        }
    }
    return vic20rom_basic_checksum();
}

int vic20rom_load_chargen(const char *rom_name)
{
    if (!vicrom_loaded) {
        return 0;
    }

    if (!util_check_null_string(rom_name)) {
        /* Load chargen ROM. */
        if (sysfile_load(rom_name,
                         machine_name,
                         vic20memrom_chargen_rom, VIC20_CHARGEN_ROM_SIZE,
                         VIC20_CHARGEN_ROM_SIZE) < 0) {
            log_error(vic20rom_log, "Couldn't load character ROM.");
            return -1;
        }
    }
    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (vic20rom_log == LOG_ERR) {
        vic20rom_log = log_open("VIC20MEM");
    }

    vicrom_loaded = 1;

    if (resources_get_string("KernalName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_kernal(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("BasicName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_basic(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("ChargenName", &rom_name) < 0) {
        return -1;
    }
    if (vic20rom_load_chargen(rom_name) < 0) {
        return -1;
    }

    return 0;
}
