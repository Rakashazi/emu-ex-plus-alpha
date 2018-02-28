/*
 * c128memsnapshot.c -- C128 memory snapshot handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "c64cart.h"
#include "c128.h"
#include "c128-resources.h"
#include "c128mem.h"
#include "c128memrom.h"
#include "c128memsnapshot.h"
#include "c128mmu.h"
#include "c128rom.h"
#include "cartridge.h"
#include "log.h"
#include "mem.h"
#include "resources.h"
#include "snapshot.h"
#include "tpi.h"
#include "types.h"
#include "uiapi.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "c64acia.h"
#endif

static log_t c128_snapshot_log = LOG_ERR;

static char snap_rom_module_name[] = "C128ROM";
#define SNAP_ROM_MAJOR 0
#define SNAP_ROM_MINOR 0

static int mem_write_rom_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_rom_module_name, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_BA(m, c128memrom_kernal_rom, C128_KERNAL_ROM_SIZE) < 0
        || SMW_BA(m, c128memrom_basic_rom, C128_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, c128memrom_basic_rom + C128_BASIC_ROM_SIZE, C128_EDITOR_ROM_SIZE) < 0
        || SMW_BA(m, mem_chargen_rom, C128_CHARGEN_ROM_SIZE) < 0) {
        goto fail;
    }

    /* FIXME: save cartridge ROM (& RAM?) areas:
       first write out the configuration, i.e.
       - type of cartridge (banking scheme type)
       - state of cartridge (active/which bank, ...)
       then the ROM/RAM arrays:
       - cartridge ROM areas
       - cartridge RAM areas
    */

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

static int mem_read_rom_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int trapfl;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_rom_module_name, &major_version, &minor_version);

    /* This module is optional.  */
    if (m == NULL) {
        return 0;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    /* Do not accept higher versions than current */
    if (major_version > SNAP_ROM_MAJOR || minor_version > SNAP_ROM_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        log_error(c128_snapshot_log,
                  "MEM: Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
        goto fail;
    }

    if (0
        || SMR_BA(m, c128memrom_kernal_rom, C128_KERNAL_ROM_SIZE) < 0
        || SMR_BA(m, c128memrom_basic_rom, C128_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, c128memrom_basic_rom + C128_BASIC_ROM_SIZE, C128_EDITOR_ROM_SIZE) < 0
        || SMR_BA(m, mem_chargen_rom, C128_CHARGEN_ROM_SIZE) < 0) {
        goto fail;
    }

    log_warning(c128_snapshot_log, "Dumped Romset files and saved settings will "                "represent\nthe state before loading the snapshot!");

    memcpy(c128memrom_kernal_trap_rom, c128memrom_kernal_rom, C128_KERNAL_ROM_SIZE);

    c128rom_basic_checksum();
    c128rom_kernal_checksum();

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    /* to get all the checkmarks right */
    ui_update_menus();

    return 0;

fail:

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

static char snap_module_name[] = "C128MEM";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0

int c128_snapshot_write_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;
    WORD i;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* Assuming no side-effects.  */
    for (i = 0; i < 11; i++) {
        if (SMW_B(m, mmu_read(i)) < 0) {
            goto fail;
        }
    }

    if (0
        || SMW_BA(m, mem_ram, C128_RAM_SIZE) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (save_roms && mem_write_rom_snapshot_module(s) < 0) {
        goto fail;
    }

    if (cartridge_snapshot_write_modules(s) < 0) {
        goto fail;
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int c128_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    WORD i;
    BYTE byte;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept higher versions than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        log_error(c128_snapshot_log,
                  "MEM: Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    for (i = 0; i < 11; i++) {
        if (SMR_B(m, &byte) < 0) {
            goto fail;
        }
        mmu_store(i, byte);     /* Assuming no side-effects */
    }

    if (SMR_BA(m, mem_ram, C128_RAM_SIZE) < 0) {
        goto fail;
    }

    /* pla_config_changed(); */

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (mem_read_rom_snapshot_module(s) < 0) {
        goto fail;
    }

    if (cartridge_snapshot_read_modules(s) < 0) {
        goto fail;
    }

    ui_update_menus();

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
