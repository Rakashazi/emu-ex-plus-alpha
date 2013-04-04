/*
 * memieee.c - IEEE drive memory.
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

#include "drive-check.h"
#include "drivemem.h"
#include "driverom.h"
#include "drivetypes.h"
#include "memieee.h"
#include "riotd.h"
#include "types.h"
#include "via1d2031.h"
#include "viad.h"


static BYTE drive_read_ram(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0x7ff];
}

static void drive_store_ram(drive_context_t *drv, WORD address, BYTE value)
{
    drv->cpud->drive_ram[address & 0x7ff] = value;
}

static BYTE drive_read_zero(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0xff];
}

static void drive_store_zero(drive_context_t *drv, WORD address, BYTE value)
{
    drv->cpud->drive_ram[address & 0xff] = value;
}

/* SFD1001 specific memory.  */

static BYTE drive_read_1001_io(drive_context_t *drv, WORD address)
{
    if (address & 0x80) {
        return riot2_read(drv, address);
    }
    return riot1_read(drv, address);
}

static void drive_store_1001_io(drive_context_t *drv, WORD address, BYTE byte)
{
    if (address & 0x80) {
        riot2_store(drv, address, byte);
    } else {
        riot1_store(drv, address, byte);
    }
}

static BYTE drive_read_1001zero_ram(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[address & 0xff];
}

static void drive_store_1001zero_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->cpud->drive_ram[address & 0xff] = byte;
}

static BYTE drive_read_1001buffer_ram(drive_context_t *drv, WORD address)
{
    return drv->cpud->drive_ram[(((address >> 2) & 0x1c00)
                                 | (address & 0x03ff)) - 0x300];
}

static void drive_store_1001buffer_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->cpud->drive_ram[(((address >> 2) & 0x1c00) | (address & 0x03ff))
                         - 0x300] = byte;
}

void memieee_init(struct drive_context_s *drv, unsigned int type)
{
    unsigned int i, j;
    drivecpud_context_t *cpud;

    cpud = drv->cpud;

    if (type == DRIVE_TYPE_2031) {
        drv->cpu->pageone = cpud->drive_ram + 0x100;

        cpud->read_func_nowatch[0] = drive_read_zero;
        cpud->store_func_nowatch[0] = drive_store_zero;

        /* Setup drive RAM.  */
        for (j = 0; j < 0x80; j += 0x20) {
            for (i = 0x00 + j; i < 0x08 + j; i++) {
                cpud->read_func_nowatch[i] = drive_read_ram;
                cpud->store_func_nowatch[i] = drive_store_ram;
            }
        }

        /* Setup 2031 VIAs.  */
        for (i = 0x18; i < 0x1c; i++) {
            cpud->read_func_nowatch[i] = via1d2031_read;
            cpud->store_func_nowatch[i] = via1d2031_store;
        }
        for (i = 0x1c; i < 0x20; i++) {
            cpud->read_func_nowatch[i] = via2d_read;
            cpud->store_func_nowatch[i] = via2d_store;
        }
    }

    if (type == DRIVE_TYPE_2031 || type == DRIVE_TYPE_1001
        || type == DRIVE_TYPE_8050 || type == DRIVE_TYPE_8250) {
        for (i = 0xc0; i < 0x100; i++) {
            cpud->read_func_nowatch[i] = drive_read_rom;
        }
    }

    if (type == DRIVE_TYPE_2040) {
        for (i = 0x100 - (DRIVE_ROM2040_SIZE >> 8); i < 0x100; i++) {
            cpud->read_func_nowatch[i] = drive_read_rom;
        }
    }

    if (type == DRIVE_TYPE_3040 || type == DRIVE_TYPE_4040) {
        for (i = 0x100 - (DRIVE_ROM3040_SIZE >> 8); i < 0x100; i++) {
            cpud->read_func_nowatch[i] = drive_read_rom;
        }
    }

    if (drive_check_old(type)) {
        /* The 2040/3040/4040/1001/8050/8250 have 256 byte at $00xx,
           mirrored at $01xx, $04xx, $05xx, $08xx, $09xx, $0cxx, $0dxx.
           (From the 2 RIOT's 128 byte RAM each. The RIOT's I/O fill
           the gaps, x00-7f the first and x80-ff the second, at
           $02xx, $03xx, $06xx, $07xx, $0axx, $0bxx, $0exx, $0fxx).
           Then we have 4k of buffers, at $1000-13ff, 2000-23ff, 3000-33ff
           and 4000-43ff, each mirrored at $x400-$x7fff, $x800-$xbff,
           and $xc00-$xfff.

           Here we set zeropage, stack and buffer RAM as well as I/O */

        drv->cpu->pageone = cpud->drive_ram;

        for (i = 0; i < 0x10; i += 4) {
            cpud->read_func_nowatch[i] = drive_read_1001zero_ram;
            cpud->store_func_nowatch[i] = drive_store_1001zero_ram;
            cpud->read_func_nowatch[i + 1] = drive_read_1001zero_ram;
            cpud->store_func_nowatch[i + 1] = drive_store_1001zero_ram;
            cpud->read_func_nowatch[i + 2] = drive_read_1001_io;
            cpud->store_func_nowatch[i + 2] = drive_store_1001_io;
            cpud->read_func_nowatch[i + 3] = drive_read_1001_io;
            cpud->store_func_nowatch[i + 3] = drive_store_1001_io;
        }

        for (i = 0x10; i < 0x50; i++) {
            cpud->read_func_nowatch[i] = drive_read_1001buffer_ram;
            cpud->store_func_nowatch[i] = drive_store_1001buffer_ram;
        }
    }
}
