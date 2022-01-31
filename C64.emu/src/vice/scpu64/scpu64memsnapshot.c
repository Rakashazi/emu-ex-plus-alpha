/*
 * scpu64memsnapshot.c - SCPU64 memory snapshot handling.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#include "scpu64-resources.h"
#include "c64cart.h"
#include "scpu64mem.h"
#include "scpu64memsnapshot.h"
#include "scpu64rom.h"
#include "scpu64cpu.h"
#include "cartridge.h"
#include "log.h"
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

#define SNAP_ROM_MAJOR 0
#define SNAP_ROM_MINOR 0

/* private stuff, but it's needed for the snapshot */
extern int mem_reg_soft_1mhz;
extern int mem_reg_sys_1mhz;
extern int mem_reg_hwenable;
extern int mem_reg_dosext;  
extern int mem_reg_ramlink; 
extern int mem_reg_optim; 
extern int mem_reg_bootmap;
extern int mem_reg_simm;  
extern int mem_pport;   
extern unsigned int mem_simm_ram_mask;
/* ------------------------ */

static log_t c64_snapshot_log = LOG_ERR;

static const char snap_rom_module_name[] = "C64ROM";

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

static int scpu64_snapshot_write_rom_module(snapshot_t *s)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_rom_module_name, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
    if (m == NULL) {
        return -1;
    }

    /* disable traps before saving the ROM */
    get_trapflags();
    clear_trapflags();

    if (SMW_BA(m, mem_chargen_rom, SCPU64_CHARGEN_ROM_SIZE) < 0
        || SMW_BA(m, scpu64rom_scpu64_rom, SCPU64_SCPU64_ROM_MAXSIZE) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }

    restore_trapflags();

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }

    restore_trapflags();

    return -1;
}

static int scpu64_snapshot_read_rom_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_rom_module_name, &major_version, &minor_version);
    if (m == NULL) {
        /* this module is optional */
        /* FIXME: reset all cartridge stuff to standard C64 behaviour */
        return 0;
    }

    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_ROM_MAJOR, SNAP_ROM_MINOR)) {
        log_error(c64_snapshot_log, "Snapshot module version (%d.%d) newer than %d.%d.", major_version, minor_version, SNAP_ROM_MAJOR, SNAP_ROM_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    /* disable traps before loading the ROM */
    get_trapflags();
    clear_trapflags();

    if (SMR_BA(m, mem_chargen_rom, SCPU64_CHARGEN_ROM_SIZE) < 0
        || SMR_BA(m, scpu64rom_scpu64_rom, SCPU64_SCPU64_ROM_MAXSIZE) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }

    /* enable traps again when necessary */
    restore_trapflags();

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    restore_trapflags();
    return -1;
}

#define SNAP_MAJOR 0
#define SNAP_MINOR 0
static const char snap_mem_module_name[] = "C64MEM";

int scpu64_snapshot_write_module(snapshot_t *s, int save_roms)
{
    snapshot_module_t *m;

    /* Main memory module.  */

    m = snapshot_module_create(s, snap_mem_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)mem_pport) < 0
        || SMW_B(m, (uint8_t)mem_reg_soft_1mhz) < 0
        || SMW_B(m, (uint8_t)mem_reg_sys_1mhz) < 0
        || SMW_B(m, (uint8_t)mem_reg_hwenable) < 0
        || SMW_B(m, (uint8_t)mem_reg_dosext) < 0
        || SMW_B(m, (uint8_t)mem_reg_ramlink) < 0
        || SMW_B(m, (uint8_t)mem_reg_optim) < 0
        || SMW_B(m, (uint8_t)mem_reg_bootmap) < 0
        || SMW_B(m, (uint8_t)mem_reg_simm) < 0
        || SMW_B(m, (uint8_t)export.exrom) < 0
        || SMW_B(m, (uint8_t)export.game) < 0
        || scpu64_snapshot_write_cpu_state(m)
        || SMW_DW(m, mem_simm_ram_mask) < 0
        || SMW_BA(m, mem_ram, SCPU64_RAM_SIZE) < 0
        || SMW_BA(m, mem_sram, SCPU64_SRAM_SIZE) < 0
        || SMW_BA(m, mem_simm_ram, mem_simm_ram_mask + 1) < 0) {
        goto fail;
    }

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (save_roms && scpu64_snapshot_write_rom_module(s) < 0) {
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

int scpu64_snapshot_read_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    snapshot_module_t *m;
    unsigned int simm_mask;

    /* Main memory module.  */

    m = snapshot_module_open(s, snap_mem_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    if (snapshot_version_is_bigger(major_version, minor_version, SNAP_MAJOR, SNAP_MINOR)) {
        log_error(c64_snapshot_log, "Snapshot module version (%d.%d) newer than %d.%d.", major_version, minor_version, SNAP_MAJOR, SNAP_MINOR);
        goto fail;
    }

    if (SMR_B_INT(m, &mem_pport) < 0
        || SMR_B_INT(m, &mem_reg_soft_1mhz) < 0
        || SMR_B_INT(m, &mem_reg_sys_1mhz) < 0
        || SMR_B_INT(m, &mem_reg_hwenable) < 0
        || SMR_B_INT(m, &mem_reg_dosext) < 0
        || SMR_B_INT(m, &mem_reg_ramlink) < 0
        || SMR_B_INT(m, &mem_reg_optim) < 0
        || SMR_B_INT(m, &mem_reg_bootmap) < 0
        || SMR_B_INT(m, &mem_reg_simm) < 0
        || SMR_B(m, &export.exrom) < 0
        || SMR_B(m, &export.game) < 0
        || scpu64_snapshot_read_cpu_state(m)
        || SMR_DW_UINT(m, &simm_mask) < 0
        || SMR_BA(m, mem_ram, SCPU64_RAM_SIZE) < 0
        || SMR_BA(m, mem_sram, SCPU64_SRAM_SIZE) < 0) {
        goto fail;
    }

    /* disable traps before loading the ROM */
    get_trapflags();
    clear_trapflags();

    memcpy(mem_trap_ram, mem_sram + 0x1e000, SCPU64_KERNAL_ROM_SIZE);

    /* enable traps again when necessary */
    restore_trapflags();

    switch (simm_mask) {
    case 0x0:
        resources_set_int("SIMMSize", 0);
        break;
    case 0xfffff:
        resources_set_int("SIMMSize", 1);
        break;
    case 0x3fffff:
        resources_set_int("SIMMSize", 4);
        break;
    case 0x7fffff:
        resources_set_int("SIMMSize", 8);
        break;
    case 0xffffff:
        resources_set_int("SIMMSize", 16);
        break;
    default:
        goto fail;
    }

    if (SMR_BA(m, mem_simm_ram, mem_simm_ram_mask + 1) < 0) {
        goto fail;
    }

    mem_set_mirroring(mem_reg_optim);
    mem_set_simm(mem_reg_simm);
    mem_pla_config_changed();

    if (snapshot_module_close(m) < 0) {
        goto fail;
    }
    m = NULL;

    if (scpu64_snapshot_read_rom_module(s) < 0) {
        goto fail;
    }

    if (cartridge_snapshot_read_modules(s) < 0) {
        goto fail;
    }

    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
