/*
 * plus4rom.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown@axelero.hu>
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
#include "plus4mem.h"
#include "plus4memrom.h"
#include "plus4rom.h"
#include "plus4cart.h"
#include "resources.h"
#include "sysfile.h"
#include "types.h"


static log_t plus4rom_log = LOG_ERR;

/* Flag: nonzero if the Kernal and BASIC ROMs have been loaded.  */
int plus4_rom_loaded = 0;

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

int plus4rom_load_kernal(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* disable traps before loading the ROM */
    get_trapflags();
    clear_trapflags();

    /* Load Kernal ROM.  */
    if (sysfile_load(rom_name, machine_name, plus4memrom_kernal_rom,
                     PLUS4_KERNAL_ROM_SIZE, PLUS4_KERNAL_ROM_SIZE) < 0) {
        log_error(plus4rom_log, "Couldn't load kernal ROM `%s'.",
                  rom_name);
        restore_trapflags();
        return -1;
    }
    memcpy(plus4memrom_kernal_trap_rom, plus4memrom_kernal_rom,
           PLUS4_KERNAL_ROM_SIZE);

    restore_trapflags();

    return 0;
}

int plus4rom_load_basic(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load Basic ROM.  */
    if (sysfile_load(rom_name, machine_name, plus4memrom_basic_rom,
                     PLUS4_BASIC_ROM_SIZE, PLUS4_BASIC_ROM_SIZE) < 0) {
        log_error(plus4rom_log,
                  "Couldn't load basic ROM `%s'.",
                  rom_name);
        return -1;
    }
    return 0;
}

int mem_load(void)
{
    const char *rom_name = NULL;

    if (plus4rom_log == LOG_ERR) {
        plus4rom_log = log_open("PLUS4MEM");
    }

    plus4_rom_loaded = 1;

    if (resources_get_string("KernalName", &rom_name) < 0) {
        return -1;
    }
    if (plus4rom_load_kernal(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("BasicName", &rom_name) < 0) {
        return -1;
    }
    if (plus4rom_load_basic(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("FunctionLowName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_func_lo(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("FunctionHighName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_func_hi(rom_name) < 0) {
        return -1;
    }
#if 0
    if (resources_get_string("c1loName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_c1lo(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("c1hiName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_c1hi(rom_name) < 0) {
        return -1;
    }
#endif
    if (resources_get_string("c2loName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_c2lo(rom_name) < 0) {
        return -1;
    }

    if (resources_get_string("c2hiName", &rom_name) < 0) {
        return -1;
    }
    if (plus4cart_load_c2hi(rom_name) < 0) {
        return -1;
    }

    return 0;
}
