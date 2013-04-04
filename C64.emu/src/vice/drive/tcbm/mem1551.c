/*
 * mem1551.c - 1551 memory.
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

#include "drivemem.h"
#include "drivetypes.h"
#include "glue1551.h"
#include "mem1551.h"
#include "tpid.h"
#include "types.h"


static BYTE drive_read_ram(drive_context_t *drv, WORD address)
{
    /* FIXME: This breaks the 1541 RAM mirror!  */
    return drv->cpud->drive_ram[address & 0x1fff];
}

static void drive_store_ram(drive_context_t *drv, WORD address, BYTE value)
{
    /* FIXME: This breaks the 1541 RAM mirror!  */
    drv->cpud->drive_ram[address & 0x1fff] = value;
}

static BYTE drive_read_zero(drive_context_t *drv, WORD address)
{
    switch (address & 0xff) {
        case 0:
            return glue1551_port0_read(drv);
        case 1:
            return glue1551_port1_read(drv);
    }

    return drv->cpud->drive_ram[address & 0xff];
}

static void drive_store_zero(drive_context_t *drv, WORD address, BYTE value)
{
    switch (address & 0xff) {
        case 0:
            glue1551_port0_store(drv, value);
            return;
        case 1:
            glue1551_port1_store(drv, value);
            return;
    }

    drv->cpud->drive_ram[address & 0xff] = value;
}

void mem1551_init(struct drive_context_s *drv, unsigned int type)
{
    unsigned int i;

    if (type == DRIVE_TYPE_1551) {
        drv->cpu->pageone = drv->cpud->drive_ram + 0x100;

        /* Setup drive RAM.  */
        for (i = 0x01; i < 0x08; i++) {
            drv->cpud->read_func_nowatch[i] = drive_read_ram;
            drv->cpud->store_func_nowatch[i] = drive_store_ram;
        }
        for (i = 0xc0; i < 0x100; i++) {
            drv->cpud->read_func_nowatch[i] = drive_read_rom;
        }

        drv->cpud->read_func_nowatch[0] = drive_read_zero;
        drv->cpud->store_func_nowatch[0] = drive_store_zero;

        /* Setup 1551 TPI.  */
        for (i = 0x40; i < 0x7f; i++) {
            drv->cpud->read_func_nowatch[i] = tpid_read;
            drv->cpud->store_func_nowatch[i] = tpid_store;
        }
    }
}
