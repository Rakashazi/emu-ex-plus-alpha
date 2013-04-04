/*
 * memiec.c - IEC drive memory.
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

#include "ciad.h"
#include "drivemem.h"
#include "drivetypes.h"
#include "lib.h"
#include "memiec.h"
#include "types.h"
#include "via1d1541.h"
#include "viad.h"
#include "wd1770.h"
#include "via4000.h"
#include "pc8477.h"

static BYTE drive_read_ram(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0x7ff];
}

static void drive_store_ram(drive_context_t *drv, WORD address, BYTE value)
{
    drv->cpud->drive_ram[address & 0x7ff] = value;
}

static BYTE drive_read_1581ram(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0x1fff];
}

static void drive_store_1581ram(drive_context_t *drv, WORD address, BYTE value)
{
    drv->cpud->drive_ram[address & 0x1fff] = value;
}

static BYTE drive_read_zero(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0xff];
}

static void drive_store_zero(drive_context_t *drv, WORD address, BYTE value)
{
    drv->cpud->drive_ram[address & 0xff] = value;
}

static BYTE drive_read_ram2(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram_expand2[address & 0x1fff];
}

static void drive_store_ram2(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram_expand2[address & 0x1fff] = value;
}

static BYTE drive_read_ram4(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram_expand4[address & 0x1fff];
}

static void drive_store_ram4(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram_expand4[address & 0x1fff] = value;
}

static BYTE drive_read_ram6(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram_expand6[address & 0x1fff];
}

static void drive_store_ram6(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram_expand6[address & 0x1fff] = value;
}

static BYTE drive_read_ram8(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram_expand8[address & 0x1fff];
}

static void drive_store_ram8(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram_expand8[address & 0x1fff] = value;
}

static BYTE drive_read_rama(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram_expanda[address & 0x1fff];
}

static void drive_store_rama(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram_expanda[address & 0x1fff] = value;
}

/* ------------------------------------------------------------------------- */

static void realloc_expram(BYTE **expram, int size)
{
    lib_free(*expram);
    *expram = lib_calloc(1, size);
}

void memiec_init(struct drive_context_s *drv, unsigned int type)
{
    unsigned int j;
    drivecpud_context_t *cpud;

    cpud = drv->cpud;

    if (type == DRIVE_TYPE_1541 || type == DRIVE_TYPE_1541II
        || type == DRIVE_TYPE_1570 || type == DRIVE_TYPE_1571
        || type == DRIVE_TYPE_1571CR || type == DRIVE_TYPE_1581
        || type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
        /* Setup drive RAM.  */
        switch (type) {
            case DRIVE_TYPE_1541:
            case DRIVE_TYPE_1541II:
                for (j = 0; j < 0x80; j += 0x20) {
                    drivemem_set_func(cpud, 0x00 + j, 0x08 + j,
                                      drive_read_ram, drive_store_ram);
                }
                break;
            case DRIVE_TYPE_1570:
            case DRIVE_TYPE_1571:
            case DRIVE_TYPE_1571CR:
                drivemem_set_func(cpud, 0x00, 0x10,
                                  drive_read_ram, drive_store_ram);
                break;
            case DRIVE_TYPE_1581:
                drivemem_set_func(cpud, 0x00, 0x20,
                                  drive_read_1581ram, drive_store_1581ram);
                break;
            case DRIVE_TYPE_2000:
            case DRIVE_TYPE_4000:
                drivemem_set_func(cpud, 0x00, 0x20,
                                  drive_read_1581ram, drive_store_1581ram);
                realloc_expram(&drv->drive->drive_ram_expand2, 0x2000);
                drivemem_set_func(cpud, 0x20, 0x40,
                                  drive_read_ram2, drive_store_ram2);
                realloc_expram(&drv->drive->drive_ram_expand4, 0x2000);
                drivemem_set_func(cpud, 0x50, 0x60,
                                  drive_read_ram4, drive_store_ram4);
                realloc_expram(&drv->drive->drive_ram_expand6, 0x2000);
                drivemem_set_func(cpud, 0x60, 0x80,
                                  drive_read_ram6, drive_store_ram6);
                break;
        }

        drv->cpu->pageone = cpud->drive_ram + 0x100;

        cpud->read_func_nowatch[0] = drive_read_zero;
        cpud->store_func_nowatch[0] = drive_store_zero;

        /* Setup drive ROM.  */
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL);

        /* for performance reasons it's only this page */
        if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
            drivemem_set_func(cpud, 0xf0, 0xf1, drive_read_rom_ds1216, NULL);
        }
    }

    /* Setup 1541, 1541-II VIAs.  */
    if (type == DRIVE_TYPE_1541 || type == DRIVE_TYPE_1541II) {
        for (j = 0; j < 0x80; j += 0x20) {
            drivemem_set_func(cpud, 0x18 + j, 0x1c + j,
                              via1d1541_read, via1d1541_store);
            drivemem_set_func(cpud, 0x1c + j, 0x20 + j,
                              via2d_read, via2d_store);
        }
    }

    /* Setup 1571 VIA1, VIA2, WD1770 and CIA.  */
    if (type == DRIVE_TYPE_1570 || type == DRIVE_TYPE_1571
        || type == DRIVE_TYPE_1571CR) {
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store);
        drivemem_set_func(cpud, 0x20, 0x30, wd1770d_read, wd1770d_store);
        drivemem_set_func(cpud, 0x40, 0x80, cia1571_read, cia1571_store);
    }

    /* Setup 1581 CIA.  */
    if (type == DRIVE_TYPE_1581) {
        drivemem_set_func(cpud, 0x40, 0x60, cia1581_read, cia1581_store);
        drivemem_set_func(cpud, 0x60, 0x80, wd1770d_read, wd1770d_store);
    }

    /* Setup 4000 VIA and dp8473/pc8477 */
    if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
        drivemem_set_func(cpud, 0x40, 0x4c, via4000_read, via4000_store);
        drivemem_set_func(cpud, 0x4e, 0x50, pc8477d_read, pc8477d_store);
    }

    if (!rom_loaded) {
        return;
    }

    /* Setup RAM expansions */
    if (type == DRIVE_TYPE_1541 || type == DRIVE_TYPE_1541II) {
        if (drv->drive->drive_ram2_enabled) {
            realloc_expram(&drv->drive->drive_ram_expand2, 0x2000);
            drivemem_set_func(cpud, 0x20, 0x40,
                              drive_read_ram2, drive_store_ram2);
        }
        if (drv->drive->drive_ram4_enabled) {
            realloc_expram(&drv->drive->drive_ram_expand4, 0x2000);
            drivemem_set_func(cpud, 0x40, 0x60,
                              drive_read_ram4, drive_store_ram4);
        }
    }

    if (type == DRIVE_TYPE_1570 || type == DRIVE_TYPE_1571
        || type == DRIVE_TYPE_1571CR) {
        if (drv->drive->drive_ram4_enabled) {
            realloc_expram(&drv->drive->drive_ram_expand4, 0x2000);
            drivemem_set_func(cpud, 0x48, 0x60,
                              drive_read_ram4, drive_store_ram4);
        }
    }

    if (type == DRIVE_TYPE_1541 || type == DRIVE_TYPE_1541II
        || type == DRIVE_TYPE_1570 || type == DRIVE_TYPE_1571
        || type == DRIVE_TYPE_1571CR) {
        if (drv->drive->drive_ram6_enabled) {
            realloc_expram(&drv->drive->drive_ram_expand6, 0x2000);
            drivemem_set_func(cpud, 0x60, 0x80,
                              drive_read_ram6, drive_store_ram6);
        }
    }

    if (type == DRIVE_TYPE_1541 || type == DRIVE_TYPE_1541II) {
        if (drv->drive->drive_ram8_enabled) {
            realloc_expram(&drv->drive->drive_ram_expand8, 0x2000);
            drivemem_set_func(cpud, 0x80, 0xa0,
                              drive_read_ram8, drive_store_ram8);
        }
        if (drv->drive->drive_rama_enabled) {
            realloc_expram(&drv->drive->drive_ram_expanda, 0x2000);
            drivemem_set_func(cpud, 0xa0, 0xc0,
                              drive_read_rama, drive_store_rama);
        }
    }
}
