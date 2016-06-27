/*
 * plus4memsnapshot.c - Plus4 memory snapshot handling.
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

#include "log.h"
#include "mem.h"
#include "plus4mem.h"
#include "plus4memrom.h"
#include "plus4memsnapshot.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "uiapi.h"

/* #define DEBUGSNAPSHOT */

#ifdef DEBUGSNAPSHOT
#define DBG(x) printf x
#else
#define DBG(x)
#endif

static log_t plus4_snapshot_log = LOG_ERR;

#define SNAP_MAJOR 1
#define SNAP_MINOR 0
static const char snap_mem_module_name[] = "PLUS4MEM";

#define SNAP_ROM_MAJOR 1
#define SNAP_ROM_MINOR 0
static const char snap_rom_module_name[] = "PLUS4ROM";

static int plus4_snapshot_write_rom_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_rom_module_name, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_BA(m, plus4memrom_kernal_rom, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMW_BA(m, plus4memrom_basic_rom, PLUS4_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, extromlo1, PLUS4_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, extromlo2, PLUS4_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, extromlo3, PLUS4_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, extromhi1, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMW_BA(m, extromhi2, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMW_BA(m, extromhi3, PLUS4_KERNAL_ROM_SIZE) < 0
        ) {
        goto fail;
    }

    ui_update_menus();

    if (snapshot_module_close(m) < 0) {
        goto fail2;
    }

    DBG(("rom snapshots written.\n"));
    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
fail2:
    DBG(("error writing rom snapshots.\n"));
    return -1;
}

static int plus4_snapshot_read_rom_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int trapfl;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_rom_module_name, &major_version, &minor_version);
    if (m == NULL) {
        /* this module is optional */
        /* FIXME: reset all cartridge stuff to standard behaviour */
        return 0;
    }

    if (major_version > SNAP_ROM_MAJOR || minor_version > SNAP_ROM_MINOR) {
        log_error(plus4_snapshot_log, "Snapshot module version (%d.%d) newer than %d.%d.", major_version, minor_version, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    if (SMR_BA(m, plus4memrom_kernal_rom, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMR_BA(m, plus4memrom_basic_rom, PLUS4_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, extromlo1, PLUS4_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, extromlo2, PLUS4_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, extromlo3, PLUS4_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, extromhi1, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMR_BA(m, extromhi2, PLUS4_KERNAL_ROM_SIZE) < 0
        || SMR_BA(m, extromhi3, PLUS4_KERNAL_ROM_SIZE) < 0
        ) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }

    memcpy(plus4memrom_kernal_trap_rom, plus4memrom_kernal_rom, PLUS4_KERNAL_ROM_SIZE);

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);
    DBG(("rom snapshots loaded.\n"));
    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    resources_set_int("VirtualDevices", trapfl);
    DBG(("error loading rom snapshots.\n"));
    return -1;
}

int plus4_snapshot_write_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_mem_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }
    if (SMW_B(m, pport.data) < 0
        || SMW_B(m, pport.dir) < 0
        || SMW_B(m, pport.data_out) < 0
#if 0
        || SMW_B(m, export.exrom) < 0
        || SMW_B(m, export.game) < 0
#endif
        || SMW_B(m, (BYTE)mem_config) < 0
        || SMW_BA(m, mem_ram, PLUS4_RAM_SIZE) < 0
        ) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (save_roms && plus4_snapshot_write_rom_module(s) < 0) {
        goto fail;
    }
#if 0
    if (cartridge_snapshot_write_modules(s) < 0) {
        goto fail;
    }
#endif
    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int plus4_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    BYTE config;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_mem_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(plus4_snapshot_log, "Snapshot module version (%d.%d) newer than %d.%d.", major_version, minor_version, SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_B(m, &pport.data) < 0
        || SMR_B(m, &pport.dir) < 0
        || SMR_B(m, &pport.data_out) < 0
#if 0
        || SMR_B(m, &export.exrom) < 0
        || SMR_B(m, &export.game) < 0
#endif
        || SMR_B(m, &config) < 0
        || SMR_BA(m, mem_ram, PLUS4_RAM_SIZE) < 0) {
        goto fail;
    }

    mem_config_ram_set(config);
    mem_config_rom_set(config);

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (plus4_snapshot_read_rom_module(s) < 0) {
        goto fail;
    }
#if 0
    if (cartridge_snapshot_read_modules(s) < 0) {
        goto fail;
    }
#endif
    ui_update_menus();

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
