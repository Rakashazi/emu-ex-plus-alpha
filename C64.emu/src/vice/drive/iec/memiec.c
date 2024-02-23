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
#include "cmdhd.h"

static uint8_t drive_read_rom(diskunit_context_t *drv, uint16_t address)
{
    return drv->cpu->cpu_last_data = drv->rom[address & 0x7fff];
}

static uint8_t drive_read_rom_ds1216(diskunit_context_t *drv, uint16_t address)
{
    return drv->cpu->cpu_last_data = ds1216e_read(drv->ds1216, address, drv->rom[address & 0x7fff]);
}

static uint8_t drive_read_ram(diskunit_context_t *drv, uint16_t address)
{
    return drv->cpu->cpu_last_data = drv->drive_ram[address];
}

static void drive_store_ram(diskunit_context_t *drv, uint16_t address, uint8_t value)
{
    drv->cpu->cpu_last_data = value;
    drv->drive_ram[address] = value;
}

static uint8_t drive_read_1541ram(diskunit_context_t *drv, uint16_t address)
{
    return drv->cpu->cpu_last_data = drv->drive_ram[address & 0x7ff];
}

static void drive_store_1541ram(diskunit_context_t *drv, uint16_t address, uint8_t value)
{
    drv->cpu->cpu_last_data = value;
    drv->drive_ram[address & 0x7ff] = value;
}

static uint8_t drive_read_zero(diskunit_context_t *drv, uint16_t address)
{
    return drv->cpu->cpu_last_data = drv->drive_ram[address & 0xffu];
}

static void drive_store_zero(diskunit_context_t *drv, uint16_t address, uint8_t value)
{
    drv->cpu->cpu_last_data = value;
    drv->drive_ram[address & 0xffu] = value;
}

/* ------------------------------------------------------------------------- */

void memiec_init(struct diskunit_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    switch (type) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_1541ram, drive_store_1541ram, NULL, &drv->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, via2d_peek, NULL, 0);
        if (drv->drive_ram2_enabled) {
            drivemem_set_func(cpud, 0x20, 0x40, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x2000], 0x20003ffd);
        } else {
            drivemem_set_func(cpud, 0x20, 0x28, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive_ram, 0x200027fd);
            drivemem_set_func(cpud, 0x38, 0x3c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x3c, 0x40, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive_ram4_enabled) {
            drivemem_set_func(cpud, 0x40, 0x60, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x4000], 0x40005ffd);
        } else {
            drivemem_set_func(cpud, 0x40, 0x48, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive_ram, 0x400047fd);
            drivemem_set_func(cpud, 0x58, 0x5c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x5c, 0x60, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive_ram6_enabled) {
            drivemem_set_func(cpud, 0x60, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x6000], 0x60007ffd);
        } else {
            drivemem_set_func(cpud, 0x60, 0x68, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive_ram, 0x600067fd);
            drivemem_set_func(cpud, 0x78, 0x7c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
            drivemem_set_func(cpud, 0x7c, 0x80, via2d_read, via2d_store, via2d_peek, NULL, 0);
        }
        if (drv->drive_ram8_enabled) {
            drivemem_set_func(cpud, 0x80, 0xa0, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x8000], 0x80009ffd);
        } else {
            drivemem_set_func(cpud, 0x80, 0xa0, drive_read_rom, NULL, NULL, drv->trap_rom, 0x80009ffd);
        }
        if (drv->drive_rama_enabled) {
            drivemem_set_func(cpud, 0xa0, 0xc0, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0xa000], 0xa000bffd);
        } else {
            drivemem_set_func(cpud, 0xa0, 0xc0, drive_read_rom, NULL, NULL, &drv->trap_rom[0x2000], 0xa000bffd);
        }
        drivemem_set_func(cpud, 0xc0, 0x100, drive_read_rom, NULL, NULL, &drv->trap_rom[0x4000], 0xc000fffd);
        break;
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_1571:
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_1541ram, drive_store_1541ram, NULL, &drv->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x08, 0x10, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive_ram, 0x08000ffd);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, via2d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x20, 0x30, wd1770d_read, wd1770d_store, wd1770d_peek, NULL, 0);
        if (drv->drive_ram4_enabled) {
            drivemem_set_func(cpud, 0x40, 0x48, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
            drivemem_set_func(cpud, 0x48, 0x60, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x4000], 0x48005ffd);
        } else {
            drivemem_set_func(cpud, 0x40, 0x60, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
        }
        if (drv->drive_ram6_enabled) {
            drivemem_set_func(cpud, 0x60, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x6000], 0x60007ffd);
        } else {
            drivemem_set_func(cpud, 0x60, 0x80, cia1571_read, cia1571_store, cia1571_peek, NULL, 0);
        }
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->trap_rom, 0x8000fffd);
        break;
    case DRIVE_TYPE_1571CR:
        /* The mos5710 IC in the 1571CR drive implements:

           - a partial CIA
           - additional MFM registers at $4010
           - address decoder

                Address Decoder:

               A 15 14 13 12   10    4    3
            RAM   0  0  0  0    x    x    x     0xxx
            VIA1  0  0  0  1    0    x    x     10xx 11xx 12xx 13xx 18xx 19xx 1axx 1bxx
            VIA2  0  0  0  1    1    x    x     14xx 15xx 16xx 17xx 1cxx 1dxx 1exx 1fxx
            FDC   0  0  1  0    x    0    0     2x00-2x07 2x20-2x27 2x40-2x47 2x60-2x67 2x80-2x87 2xa0-2xa7 2xc0-2xc7 2xe0-2xe7
            CIA   0  1  0  0    x    0    x     4x0x 4x2x 4x4x 4x6x 4x8x 4xax 4xcx 4xex
            FDC2  0  1  0  0    0    1    x     401x 403x 405x 407x 409x 40bx 40dx 40fx
                                                411x 413x 415x 417x 419x 41bx 41dx 41fx
                                                421x 423x 425x 427x 429x 42bx 42dx 42fx
                                                431x 433x 435x 437x 439x 43bx 43dx 43fx
                                                481x 483x 485x 487x 489x 48bx 48dx 48fx
                                                491x 493x 495x 497x 499x 49bx 49dx 49fx
                                                4a1x 4a3x 4a5x 4a7x 4a9x 4abx 4adx 4afx
                                                4b1x 4b3x 4b5x 4b7x 4b9x 4bbx 4bdx 4bfx
            RAM   0  1  1  x    x    x    x     6xxx 7xxx
         */
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_1541ram, drive_store_1541ram, NULL, &drv->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x08, 0x10, drive_read_1541ram, drive_store_1541ram, NULL, drv->drive_ram, 0x08000ffd);
        drivemem_set_func(cpud, 0x10, 0x14, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x14, 0x18, via2d_read, via2d_store, via2d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d1541_read, via1d1541_store, via1d1541_peek, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, via2d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x20, 0x30, wd1770d_read, wd1770d_store, wd1770d_peek, NULL, 0);
        /* FIXME: the following is very incorrect */
        if (drv->drive_ram4_enabled) {
            drivemem_set_func(cpud, 0x40, 0x48, mos5710_read, mos5710_store, mos5710_peek, NULL, 0);
            drivemem_set_func(cpud, 0x48, 0x60, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x4000], 0x48005ffd);
        } else {
            drivemem_set_func(cpud, 0x40, 0x60, mos5710_read, mos5710_store, mos5710_peek, NULL, 0);
        }
        if (drv->drive_ram6_enabled) {
            drivemem_set_func(cpud, 0x60, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x6000], 0x60007ffd);
        } else {
            drivemem_set_func(cpud, 0x60, 0x80, mos5710_read, mos5710_store, mos5710_peek, NULL, 0);
        }
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->trap_rom, 0x8000fffd);
        break;
    case DRIVE_TYPE_1581:
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x00001ffd);
        drivemem_set_func(cpud, 0x01, 0x20, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x0100], 0x00001ffd);
        drivemem_set_func(cpud, 0x40, 0x60, cia1581_read, cia1581_store, cia1581_peek, NULL, 0);
        drivemem_set_func(cpud, 0x60, 0x80, wd1770d_read, wd1770d_store, wd1770d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->trap_rom, 0x8000fffd);
        break;
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x00003ffd);
        drivemem_set_func(cpud, 0x01, 0x40, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x0100], 0x00003ffd);
        drivemem_set_func(cpud, 0x40, 0x4c, via4000_read, via4000_store, via4000_peek, NULL, 0);
        drivemem_set_func(cpud, 0x4e, 0x50, pc8477d_read, pc8477d_store, pc8477d_peek, NULL, 0);
        drivemem_set_func(cpud, 0x50, 0x80, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x5000], 0x50007ffd);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, NULL, drv->trap_rom, 0x8000fffd);
        /* for performance reasons it's only this page */
        drivemem_set_func(cpud, 0xf0, 0xf1, drive_read_rom_ds1216, NULL, NULL, &drv->trap_rom[0x7000], 0x8000fffd);
        break;
    case DRIVE_TYPE_CMDHD:
        drv->cpu->pageone = drv->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, NULL, drv->drive_ram, 0x00003ffd);
        drivemem_set_func(cpud, 0x01, 0x40, drive_read_ram, drive_store_ram, NULL, &drv->drive_ram[0x0100], 0x00003ffd);
        /* CMDHD uses a lot of weird registers to mamage the memory above 0x4000
        so the granularity here doesn't work. We just group it all together */
        drivemem_set_func(cpud, 0x40, 0x100, cmdhd_read, cmdhd_store, NULL, NULL, 0x0000fffd);
    default:
        return;
    }
}
