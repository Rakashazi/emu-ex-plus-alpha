/*
 * c64memsnapshot.c - C64 memory snapshot handling.
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

#include "c64-resources.h"
#include "c64cart.h"
#include "c64mem.h"
#include "c64memrom.h"
#include "c64memsnapshot.h"
#include "c64pla.h"
#include "c64rom.h"
#include "cartridge.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "resources.h"
#include "reu.h"
#include "georam.h"
#include "snapshot.h"
#include "types.h"
#include "uiapi.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "c64acia.h"
#endif

/* ---------------------------------------------------------------------*/

/* C64ROM snapshot module format:

   type  | name    | description
   -----------------------------
   ARRAY | KERNAL  | 8192 BYTES of KERNAL ROM data
   ARRAY | BASIC   | 8192 BYTES of BASIC ROM data
   ARRAY | CHARGEN | 4096 BYTES of CHARGEN ROM data
 */

static const char snap_rom_module_name[] = "C64ROM";
#define SNAP_ROM_MAJOR 0
#define SNAP_ROM_MINOR 0

/* static log_t c64_snapshot_log = LOG_ERR; */

static int c64_snapshot_write_rom_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_rom_module_name, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_BA(m, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE) < 0
        || SMW_BA(m, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE) < 0
        || SMW_BA(m, mem_chargen_rom, C64_CHARGEN_ROM_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    ui_update_menus();

    return snapshot_module_close(m);
}

static int c64_snapshot_read_rom_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int trapfl;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_rom_module_name, &major_version, &minor_version);

    if (m == NULL) {
        /* this module is optional */
        /* FIXME: reset all cartridge stuff to standard C64 behaviour */
        return 0;
    }

    /* get old value */
    resources_get_int("VirtualDevices", &trapfl);

    /* Do not accept versions higher than current */
    if (major_version > SNAP_ROM_MAJOR || minor_version > SNAP_ROM_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* disable traps before loading the ROM */
    resources_set_int("VirtualDevices", 0);

    if (0
        || SMR_BA(m, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE) < 0
        || SMR_BA(m, c64memrom_basic64_rom, C64_BASIC_ROM_SIZE) < 0
        || SMR_BA(m, mem_chargen_rom, C64_CHARGEN_ROM_SIZE) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        resources_set_int("VirtualDevices", trapfl);
        return -1;
    }

    memcpy(c64memrom_kernal64_trap_rom, c64memrom_kernal64_rom, C64_KERNAL_ROM_SIZE);
    c64rom_get_kernal_checksum();
    c64rom_get_basic_checksum();

    /* enable traps again when necessary */
    resources_set_int("VirtualDevices", trapfl);

    return 0;

fail:
    snapshot_module_close(m);
    /* restore old value */
    resources_set_int("VirtualDevices", trapfl);
    return -1;
}

/* ---------------------------------------------------------------------*/

/* C64MEM snapshot module format:

   type  | name                | version | description
   ---------------------------------------------------
   BYTE  | pport data          |   0.0+  | CPU port data register
   BYTE  | pport dir           |   0.0+  | CPU port direction register
   BYTE  | EXROM               |   0.0+  | EXROM line state
   BYTE  | GAME                |   0.0+  | GAME line state
   ARRAY | RAM                 |   0.0+  | 65536 BYTES of RAM data
   BYTE  | pport data out      |   0.0+  | CPU port data out lines state
   BYTE  | pport data read     |   0.0+  | CPU port data in lines state
   BYTE  | pport dir read      |   0.0+  | CPU port direction in lines state
   DWORD | pport bit6 clock    |   0.1   | CPU port bit 6 falloff clock
   DWORD | pport bit7 clock    |   0.1   | CPU port bit 7 falloff clock
   BYTE  | pport bit 6         |   0.1   | CPU port bit 6 state
   BYTE  | pport bit 7         |   0.1   | CPU port bit 7 state
   BYTE  | pport bit 6 falloff |   0.1   | CPU port bit 6 discharge flag
   BYTE  | pport bit 7 falloff |   0.1   | CPU port bit 7 discharge flag
 */

static const char snap_mem_module_name[] = "C64MEM";
#define SNAP_MAJOR 0
#define SNAP_MINOR 1

int c64_snapshot_write_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_mem_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, pport.data) < 0
        || SMW_B(m, pport.dir) < 0
        || SMW_B(m, export.exrom) < 0
        || SMW_B(m, export.game) < 0
        || SMW_BA(m, mem_ram, C64_RAM_SIZE) < 0
        || SMW_B(m, pport.data_out) < 0
        || SMW_B(m, pport.data_read) < 0
        || SMW_B(m, pport.dir_read) < 0
        || SMW_DW(m, (DWORD)pport.data_set_clk_bit6) < 0
        || SMW_DW(m, (DWORD)pport.data_set_clk_bit7) < 0
        || SMW_B(m, pport.data_set_bit6) < 0
        || SMW_B(m, pport.data_set_bit7) < 0
        || SMW_B(m, pport.data_falloff_bit6) < 0
        || SMW_B(m, pport.data_falloff_bit7) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    if (save_roms && c64_snapshot_write_rom_module(s) < 0) {
        return -1;
    }

    return cartridge_snapshot_write_modules(s);
}

int c64_snapshot_read_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int tmp_bit6, tmp_bit7;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_mem_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B(m, &pport.data) < 0
        || SMR_B(m, &pport.dir) < 0
        || SMR_B(m, &export.exrom) < 0
        || SMR_B(m, &export.game) < 0
        || SMR_BA(m, mem_ram, C64_RAM_SIZE) < 0
        || SMR_B(m, &pport.data_out) < 0
        || SMR_B(m, &pport.data_read) < 0
        || SMR_B(m, &pport.dir_read) < 0) {
        goto fail;
    }

    /* new since 0.1 */
    if (SNAPVAL(major_version, minor_version, 0, 1)) {
        if (0
            || SMR_DW_INT(m, &tmp_bit6) < 0
            || SMR_DW_INT(m, &tmp_bit7) < 0
            || SMR_B(m, &pport.data_set_bit6) < 0
            || SMR_B(m, &pport.data_set_bit7) < 0
            || SMR_B(m, &pport.data_falloff_bit6) < 0
            || SMR_B(m, &pport.data_falloff_bit7) < 0) {
            goto fail;
        }
        pport.data_set_clk_bit6 = (CLOCK)tmp_bit6;
        pport.data_set_clk_bit7 = (CLOCK)tmp_bit7;
    } else {
        pport.data_set_bit6 = 0;
        pport.data_set_bit7 = 0;
        pport.data_falloff_bit6 = 0;
        pport.data_falloff_bit7 = 0;
        pport.data_set_clk_bit6 = 0;
        pport.data_set_clk_bit7 = 0;
    }

    mem_pla_config_changed();

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    if (c64_snapshot_read_rom_module(s) < 0) {
        return -1;
    }

    if (cartridge_snapshot_read_modules(s) < 0) {
        return -1;
    }

    ui_update_menus();

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
