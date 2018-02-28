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

#include "ciad.h"
#include "drive.h"
#include "drivemem.h"
#include "driverom.h"
#include "drivetypes.h"
#include "ds1216e.h"
#include "log.h"
#include "machine-drive.h"
#include "mem.h"
#include "monitor.h"
#include "riotd.h"
#include "tpid.h"
#include "types.h"
#include "via1d1541.h"
#include "via4000.h"
#include "viad.h"

static drive_read_func_t *read_tab_watch[0x101];
static drive_store_func_t *store_tab_watch[0x101];

/* ------------------------------------------------------------------------- */
/* Common memory access.  */

static BYTE drive_read_free(drive_context_t *drv, WORD address)
{
    return address >> 8;
}

static void drive_store_free(drive_context_t *drv, WORD address, BYTE value)
{
    return;
}

static BYTE drive_peek_free(drive_context_t *drv, WORD address)
{
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Watchpoint memory access.  */

static BYTE drive_zero_read_watch(drive_context_t *drv, WORD addr)
{
    addr &= 0xff;
    monitor_watch_push_load_addr(addr, drv->cpu->monspace);
    return drv->cpud->read_tab[0][0](drv, addr);
}

static void drive_zero_store_watch(drive_context_t *drv, WORD addr, BYTE value)
{
    addr &= 0xff;
    monitor_watch_push_store_addr(addr, drv->cpu->monspace);
    drv->cpud->store_tab[0][0](drv, addr, value);
}

static BYTE drive_read_watch(drive_context_t *drv, WORD address)
{
    monitor_watch_push_load_addr(address, drv->cpu->monspace);
    return drv->cpud->read_tab[0][address >> 8](drv, address);
}

static void drive_store_watch(drive_context_t *drv, WORD address, BYTE value)
{
    monitor_watch_push_store_addr(address, drv->cpu->monspace);
    drv->cpud->store_tab[0][address >> 8](drv, address, value);
}

void drivemem_toggle_watchpoints(int flag, void *context)
{
    drive_context_t *drv = (drive_context_t *)context;

    if (flag) {
        drv->cpud->read_func_ptr = read_tab_watch;
        drv->cpud->store_func_ptr = store_tab_watch;
    } else {
        drv->cpud->read_func_ptr = drv->cpud->read_tab[0];
        drv->cpud->store_func_ptr = drv->cpud->store_tab[0];
    }
}

/* ------------------------------------------------------------------------- */

void drivemem_set_func(drivecpud_context_t *cpud,
                       unsigned int start, unsigned int stop,
                       drive_read_func_t *read_func,
                       drive_store_func_t *store_func,
                       drive_peek_func_t *peek_func,
                       BYTE *base, DWORD limit)
{
    unsigned int i;

    if (read_func != NULL) {
        for (i = start; i < stop; i++) {
            cpud->read_tab[0][i] = read_func;
        }
        /* if no peek function is provided, use the read function instead */
        if (peek_func == NULL) {
            peek_func = read_func;
        }
    }
    if (store_func != NULL) {
        for (i = start; i < stop; i++) {
            cpud->store_tab[0][i] = store_func;
        }
    }
    if (peek_func != NULL) {
        for (i = start; i < stop; i++) {
            cpud->peek_tab[0][i] = peek_func;
        }
    }
    for (i = start; i < stop; i++) {
        cpud->read_base_tab[0][i] = base ? (base - (start << 8)) : NULL;
        cpud->read_limit_tab[0][i] = limit;
    }
}

/* ------------------------------------------------------------------------- */
/* This is the external interface for banked memory access.  */

BYTE drivemem_bank_read(int bank, WORD addr, void *context)
{
    drive_context_t *drv = (drive_context_t *)context;

    return drv->cpud->read_func_ptr[addr >> 8](drv, addr);
}

BYTE drivemem_bank_peek(int bank, WORD addr, void *context)
{
    drive_context_t *drv = (drive_context_t *)context;

    return drv->cpud->peek_func_ptr[addr >> 8](drv, addr);
}

void drivemem_bank_store(int bank, WORD addr, BYTE value, void *context)
{
    drive_context_t *drv = (drive_context_t *)context;

    drv->cpud->store_func_ptr[addr >> 8](drv, addr, value);
}

/* ------------------------------------------------------------------------- */

void drivemem_init(drive_context_t *drv, unsigned int type)
{
    int i;

    /* setup watchpoint tables */
    if (!read_tab_watch[0]) {
        read_tab_watch[0] = drive_zero_read_watch;
        store_tab_watch[0] = drive_zero_store_watch;
        for (i = 1; i < 0x101; i++) {
            read_tab_watch[i] = drive_read_watch;
            store_tab_watch[i] = drive_store_watch;
        }
    }

    drivemem_set_func(drv->cpud, 0x00, 0x101, drive_read_free, drive_store_free, drive_peek_free, NULL, 0);

    machine_drive_mem_init(drv, type);

    drv->cpud->read_tab[0][0x100] = drv->cpud->read_tab[0][0];
    drv->cpud->store_tab[0][0x100] = drv->cpud->store_tab[0][0];
    drv->cpud->peek_tab[0][0x100] = drv->cpud->peek_tab[0][0];

    drv->cpud->read_func_ptr = drv->cpud->read_tab[0];
    drv->cpud->store_func_ptr = drv->cpud->store_tab[0];
    drv->cpud->peek_func_ptr = drv->cpud->peek_tab[0];

    drv->cpud->read_base_tab_ptr = drv->cpud->read_base_tab[0];
    drv->cpud->read_limit_tab_ptr = drv->cpud->read_limit_tab[0];
}

mem_ioreg_list_t *drivemem_ioreg_list_get(void *context)
{
    unsigned int type;
    mem_ioreg_list_t *drivemem_ioreg_list = NULL;

    type = ((drive_context_t *)context)->drive->type;

    switch (type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_2031:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA1", 0x1800, 0x180f, via1d1541_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA2", 0x1c00, 0x1c0f, via2d_dump, context);
            break;
        case DRIVE_TYPE_1551:
            mon_ioreg_add_list(&drivemem_ioreg_list, "TPI", 0x4000, 0x4007, tpid_dump, context);
            break;
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA1", 0x1800, 0x180f, via1d1541_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA2", 0x1c00, 0x1c0f, via2d_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "WD1770", 0x2000, 0x2003, NULL, context); /* FIXME: register dump function */
            mon_ioreg_add_list(&drivemem_ioreg_list, "CIA", 0x4000, 0x400f, cia1571_dump, context);
            break;
        case DRIVE_TYPE_1581:
            mon_ioreg_add_list(&drivemem_ioreg_list, "CIA", 0x4000, 0x400f, cia1581_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "WD1770", 0x6000, 0x6003, NULL, context); /* FIXME: register dump function */
            break;
        case DRIVE_TYPE_2000:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA", 0x4000, 0x400f, via4000_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "DP8473", 0x4e00, 0x4e07, NULL, context); /* FIXME: register dump function */
            break;
        case DRIVE_TYPE_4000:
            mon_ioreg_add_list(&drivemem_ioreg_list, "VIA", 0x4000, 0x400f, via4000_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "PC8477", 0x4e00, 0x4e07, NULL, context); /* FIXME: register dump function */
            break;
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            mon_ioreg_add_list(&drivemem_ioreg_list, "RIOT1", 0x0200, 0x021f, riot1_dump, context);
            mon_ioreg_add_list(&drivemem_ioreg_list, "RIOT2", 0x0280, 0x029f, riot2_dump, context);
            break;
        default:
            log_error(LOG_ERR, "DRIVEMEM: Unknown drive type `%i'.", type);
            break;
    }
    return drivemem_ioreg_list;
}
