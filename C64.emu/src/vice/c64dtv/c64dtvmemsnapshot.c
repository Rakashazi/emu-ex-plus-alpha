/*
 * c64dtvmemsnapshot.c - C64DTV memory snapshot handling.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Based on code by
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

#include "c64dtv-resources.h"
#include "c64cart.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64memsnapshot.h"
#include "c64pla.h"
#include "c64rom.h"
#include "log.h"
#include "mem.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "uiapi.h"

#include "c64dtvmem.h"
#include "c64dtvflash.h"
#include "c64dtvmemsnapshot.h"
#include "hummeradc.h"

#define SNAP_ROM_MAJOR 0
#define SNAP_ROM_MINOR 0

static log_t c64_snapshot_log = LOG_ERR;

static const char snap_rom_module_name[] = "C64ROM";


static int c64dtv_snapshot_write_rom_module(snapshot_t *s)
{
    snapshot_module_t *m;
    int trapfl;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_rom_module_name,
                               SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* disable traps before saving the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    if (SMW_BA(m, c64dtvflash_mem, 0x200000) < 0 /* hack */
        || SMW_B(m, c64dtvflash_state) < 0
        || SMW_BA(m, c64dtvflash_mem_lock, 39) < 0) {
        goto fail;
    }

    ui_update_menus();

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }

    resources_set_int("VirtualDevices", trapfl);

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }

    resources_set_int("VirtualDevices", trapfl);

    return -1;
}

static int c64dtv_snapshot_read_rom_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int trapfl;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_rom_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        /* this module is optional */
        /* FIXME: reset all cartridge stuff to standard C64 behaviour */
        return 0;
    }

    if (major_version > SNAP_ROM_MAJOR || minor_version > SNAP_ROM_MINOR) {
        log_error(c64_snapshot_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    /* disable traps before loading the ROM */
    resources_get_int("VirtualDevices", &trapfl);
    resources_set_int("VirtualDevices", 0);

    if (SMR_BA(m, c64dtvflash_mem, 0x200000) < 0 /* hack */
        || SMR_B(m, &c64dtvflash_state) < 0
        || SMR_BA(m, c64dtvflash_mem_lock, 39) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }

    /* enable traps again when necessary */

    resources_set_int("VirtualDevices", trapfl);

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    resources_set_int("VirtualDevices", trapfl);
    return -1;
}


#define SNAP_MAJOR 0
#define SNAP_MINOR 0
static const char snap_mem_module_name[] = "C64MEM";

int c64dtv_snapshot_write_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;

    /* Main memory module.  */
    m = snapshot_module_create(s, snap_mem_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, pport.data) < 0
        || SMW_B(m, pport.dir) < 0
        || SMW_BA(m, mem_ram, 0x200000) < 0 /* hack */
        || SMW_B(m, c64dtvmem_memmapper[0]) < 0
        || SMW_B(m, c64dtvmem_memmapper[1]) < 0
        || SMW_B(m, pport.data_out) < 0
        || SMW_B(m, pport.data_read) < 0
        || SMW_B(m, pport.dir_read) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (save_roms && c64dtv_snapshot_write_rom_module(s) < 0) {
        goto fail;
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int c64dtv_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_mem_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(c64_snapshot_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_B(m, &pport.data) < 0
        || SMR_B(m, &pport.dir) < 0
        || SMR_BA(m, mem_ram, 0x200000) < 0 /* hack */
        || SMR_B(m, &c64dtvmem_memmapper[0]) < 0
        || SMR_B(m, &c64dtvmem_memmapper[1]) < 0) {
        goto fail;
    }

    /* new since 1.15.x */
    SMR_B(m, &pport.data_out);
    SMR_B(m, &pport.data_read);
    SMR_B(m, &pport.dir_read);

    mem_pla_config_changed();

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (c64dtv_snapshot_read_rom_module(s) < 0) {
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

#define SNAP_MAJOR 0
#define SNAP_MINOR 0
static const char snap_misc_module_name[] = "C64DTVMISC";

int c64dtvmisc_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Misc. module.  */
    m = snapshot_module_create(s, snap_misc_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, hummeradc_value) < 0
        || SMW_B(m, hummeradc_channel) < 0
        || SMW_B(m, hummeradc_control) < 0
        || SMW_B(m, hummeradc_chanattr) < 0
        || SMW_B(m, hummeradc_chanwakeup) < 0
        || SMW_B(m, hummeradc_prev) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int c64dtvmisc_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;

    /* Misc. module.  */
    m = snapshot_module_open(s, snap_misc_module_name,
                             &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        log_error(c64_snapshot_log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  major_version, minor_version,
                  SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_B(m, &hummeradc_value) < 0
        || SMR_B(m, &hummeradc_channel) < 0
        || SMR_B(m, &hummeradc_control) < 0
        || SMR_B(m, &hummeradc_chanattr) < 0
        || SMR_B(m, &hummeradc_chanwakeup) < 0
        || SMR_B(m, &hummeradc_prev) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
