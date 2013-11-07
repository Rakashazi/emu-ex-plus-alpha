/*
 * drivemem.c - Drive memory handling.
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
#include <stdlib.h>
#include <string.h>

#include "drive.h"
#include "drivemem.h"
#include "driverom.h"
#include "drivetypes.h"
#include "log.h"
#include "machine-drive.h"
#include "mem.h"
#include "monitor.h"
#include "types.h"
#include "ds1216e.h"


/* ------------------------------------------------------------------------- */
/* Common memory access.  */

BYTE drive_read_rom(drive_context_t *drv, WORD address)
{
    return drv->drive->rom[address & 0x7fff];
}

BYTE drive_read_rom_ds1216(drive_context_t *drv, WORD address)
{
    return ds1216e_read(drv->drive->ds1216, address, drv->drive->rom[address & 0x7fff]);
}

static BYTE drive_read_free(drive_context_t *drv, WORD address)
{
    return address >> 8;
}

static void drive_store_free(drive_context_t *drv, WORD address, BYTE value)
{
    return;
}

/* ------------------------------------------------------------------------- */
/* Watchpoint memory access.  */

static BYTE drive_zero_read_watch(drive_context_t *drv, WORD addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, drv->cpu->monspace);
    return drv->cpud->read_func_nowatch[0](drv, addr);
}

static void drive_zero_store_watch(drive_context_t *drv, WORD addr, BYTE value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, drv->cpu->monspace);
    drv->cpud->store_func_nowatch[0](drv, addr, value);
}

static BYTE drive_read_watch(drive_context_t *drv, WORD address)
{
    monitor_watch_push_load_addr(address, drv->cpu->monspace);
    return drv->cpud->read_func_nowatch[address >> 8](drv, address);
}

static void drive_store_watch(drive_context_t *drv, WORD address, BYTE value)
{
    monitor_watch_push_store_addr(address, drv->cpu->monspace);
    drv->cpud->store_func_nowatch[address >> 8](drv, address, value);
}

/* ------------------------------------------------------------------------- */

void drivemem_set_func(drivecpud_context_t *cpud,
                       unsigned int start, unsigned int stop,
                       drive_read_func_t *read_func,
                       drive_store_func_t *store_func)
{
    unsigned int i;

    if (read_func != NULL) {
        for (i = start; i < stop; i++) {
            cpud->read_func_nowatch[i] = read_func;
        }
    }
    if (store_func != NULL) {
        for (i = start; i < stop; i++) {
            cpud->store_func_nowatch[i] = store_func;
        }
    }
}

#ifdef _MSC_VER
#pragma optimize("",off)
#endif

void drivemem_init(drive_context_t *drv, unsigned int type)
{
    int i;

    /* setup watchpoint tables */
    drv->cpud->read_func_watch[0] = drive_zero_read_watch;
    drv->cpud->store_func_watch[0] = drive_zero_store_watch;
    for (i = 1; i < 0x101; i++) {
        drv->cpud->read_func_watch[i] = drive_read_watch;
        drv->cpud->store_func_watch[i] = drive_store_watch;
    }

    for (i = 0; i < 0x101; i++) {
        drv->cpud->read_func_nowatch[i] = drive_read_free;
        drv->cpud->store_func_nowatch[i] = drive_store_free;
    }

    machine_drive_mem_init(drv, type);

    drv->cpud->read_func_nowatch[0x100] = drv->cpud->read_func_nowatch[0];
    drv->cpud->store_func_nowatch[0x100] = drv->cpud->store_func_nowatch[0];

    memcpy(drv->cpud->read_func, drv->cpud->read_func_nowatch,
           sizeof(drive_read_func_t *) * 0x101);
    memcpy(drv->cpud->store_func, drv->cpud->store_func_nowatch,
           sizeof(drive_store_func_t *) * 0x101);

    switch (type) {
        case DRIVE_TYPE_NONE:
            break;
        case DRIVE_TYPE_2040:
            drv->drive->rom_start = 0xe000;
            break;
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
            drv->drive->rom_start = 0xd000;
            break;
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1551:
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            drv->drive->rom_start = 0xc000;
            break;
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
            drv->drive->rom_start = 0x8000;
            break;
        default:
            log_error(LOG_ERR, "DRIVEMEM: Unknown drive type `%i'.", type);
    }
}

#ifdef _MSC_VER
#pragma optimize("",on)
#endif

mem_ioreg_list_t *drivemem_ioreg_list_get(void *context)
{
    unsigned int type;
    mem_ioreg_list_t *drivemem_ioreg_list = NULL;

    type = ((drive_context_t *)context)->drive->type;

    switch (type) {
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA1", 0x1800, 0x180f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA2", 0x1c00, 0x1c0f, NULL);
            break;
        case DRIVE_TYPE_1551:
            mon_ioreg_add_list(&drivemem_ioreg_list, "TPI", 0x4000, 0x4007, NULL);
            break;
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA1", 0x1800, 0x180f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA2", 0x1c00, 0x1c0f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "WD1770", 0x2000, 0x2003, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "CIA", 0x4000, 0x400f, NULL);
            break;
        case DRIVE_TYPE_1581:
            mon_ioreg_add_list(&drivemem_ioreg_list, "CIA", 0x4000, 0x400f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "WD1770", 0x6000, 0x6003, NULL);
            break;
        case DRIVE_TYPE_2000:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA", 0x4000, 0x400f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "DP8473", 0x4e00, 0x4e07, NULL);
            break;
        case DRIVE_TYPE_4000:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA", 0x4000, 0x400f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "PC8477", 0x4e00, 0x4e07, NULL);
            break;
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            mon_ioreg_add_list(&drivemem_ioreg_list, "RIOT1", 0x0200, 0x021f, NULL);
            mon_ioreg_add_list(&drivemem_ioreg_list, "RIOT2", 0x0280, 0x029f, NULL);
            break;
        default:
            log_error(LOG_ERR, "DRIVEMEM: Unknown drive type `%i'.", type);
    }

    return drivemem_ioreg_list;
}
