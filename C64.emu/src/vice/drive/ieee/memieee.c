/*
 * memieee.c - IEEE drive memory.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Kajtar Zsolt <soci@c64.rulez.org>
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


static BYTE drive_read_rom(drive_context_t *drv, WORD address)
{
    return drv->drive->rom[address & 0x7fff];
}

static BYTE drive_read_2031ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[address & 0x7ff];
}

static void drive_store_2031ram(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram[address & 0x7ff] = value;
}

static BYTE drive_read_zero(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[address & 0xff];
}

static void drive_store_zero(drive_context_t *drv, WORD address, BYTE value)
{
    drv->drive->drive_ram[address & 0xff] = value;
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
    return drv->drive->drive_ram[address & 0xff];
}

static void drive_store_1001zero_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[address & 0xff] = byte;
}

static BYTE drive_read_1001buffer1_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x7ff) + 0x100];
}

static void drive_store_1001buffer1_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x7ff) + 0x100] = byte;
}

static BYTE drive_read_1001buffer2_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x7ff) + 0x900];
}

static void drive_store_1001buffer2_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x7ff) + 0x900] = byte;
}

static BYTE drive_read_2040buffer1_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x3ff) + 0x100];
}

static void drive_store_2040buffer1_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x3ff) + 0x100] = byte;
}

static BYTE drive_read_2040buffer2_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x3ff) + 0x500];
}

static void drive_store_2040buffer2_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x3ff) + 0x500] = byte;
}

static BYTE drive_read_2040buffer3_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x3ff) + 0x900];
}

static void drive_store_2040buffer3_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x3ff) + 0x900] = byte;
}

static BYTE drive_read_2040buffer4_ram(drive_context_t *drv, WORD address)
{
    return drv->drive->drive_ram[(address & 0x3ff) + 0xd00];
}

static void drive_store_2040buffer4_ram(drive_context_t *drv, WORD address, BYTE byte)
{
    drv->drive->drive_ram[(address & 0x3ff) + 0xd00] = byte;
}

void memieee_init(struct drive_context_s *drv, unsigned int type)
{
    drivecpud_context_t *cpud = drv->cpud;

    switch (type) {
    case DRIVE_TYPE_2031: 
        drv->cpu->pageone = drv->drive->drive_ram + 0x100;
        drivemem_set_func(cpud, 0x00, 0x01, drive_read_zero, drive_store_zero, drv->drive->drive_ram, 0x000007fd);
        drivemem_set_func(cpud, 0x01, 0x08, drive_read_2031ram, drive_store_2031ram, &drv->drive->drive_ram[0x0100], 0x000007fd);
        drivemem_set_func(cpud, 0x18, 0x1c, via1d2031_read, via1d2031_store, NULL, 0);
        drivemem_set_func(cpud, 0x1c, 0x20, via2d_read, via2d_store, NULL, 0);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, drv->drive->trap_rom, 0x8000bffd);
        return;
    case DRIVE_TYPE_1001:
        drv->cpu->pageone = drv->drive->drive_ram;
        drivemem_set_func(cpud, 0x00, 0x02, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x000000fd);
        drivemem_set_func(cpud, 0x02, 0x04, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x04, 0x06, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x040004fd);
        drivemem_set_func(cpud, 0x06, 0x08, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x08, 0x0a, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x080008fd);
        drivemem_set_func(cpud, 0x0a, 0x0c, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x0c, 0x0e, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x0c000cfd);
        drivemem_set_func(cpud, 0x0e, 0x10, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x10, 0x18, drive_read_1001buffer1_ram, drive_store_1001buffer1_ram, &drv->drive->drive_ram[0x0100], 0x100017fd);
        drivemem_set_func(cpud, 0x18, 0x20, drive_read_1001buffer1_ram, drive_store_1001buffer1_ram, &drv->drive->drive_ram[0x0100], 0x18001ffd);
        drivemem_set_func(cpud, 0x20, 0x28, drive_read_1001buffer1_ram, drive_store_1001buffer1_ram, &drv->drive->drive_ram[0x0100], 0x200027fd);
        drivemem_set_func(cpud, 0x28, 0x30, drive_read_1001buffer1_ram, drive_store_1001buffer1_ram, &drv->drive->drive_ram[0x0100], 0x28002ffd);
        drivemem_set_func(cpud, 0x30, 0x38, drive_read_1001buffer2_ram, drive_store_1001buffer2_ram, &drv->drive->drive_ram[0x0900], 0x300037fd);
        drivemem_set_func(cpud, 0x38, 0x40, drive_read_1001buffer2_ram, drive_store_1001buffer2_ram, &drv->drive->drive_ram[0x0900], 0x38003ffd);
        drivemem_set_func(cpud, 0x40, 0x48, drive_read_1001buffer2_ram, drive_store_1001buffer2_ram, &drv->drive->drive_ram[0x0900], 0x400047fd);
        drivemem_set_func(cpud, 0x48, 0x50, drive_read_1001buffer2_ram, drive_store_1001buffer2_ram, &drv->drive->drive_ram[0x0900], 0x48004ffd);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, drv->drive->trap_rom, 0x8000fffd);
        return;
    case DRIVE_TYPE_8050:
    case DRIVE_TYPE_8250:
        drv->cpu->pageone = drv->drive->drive_ram;
        drivemem_set_func(cpud, 0x00, 0x02, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x000000fd); 
        drivemem_set_func(cpud, 0x02, 0x04, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x04, 0x06, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x040004fd); 
        drivemem_set_func(cpud, 0x06, 0x08, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x08, 0x0a, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x080008fd); 
        drivemem_set_func(cpud, 0x0a, 0x0c, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x0c, 0x0e, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x0c000cfd); 
        drivemem_set_func(cpud, 0x0e, 0x10, drive_read_1001_io, drive_store_1001_io, NULL, 0);
        drivemem_set_func(cpud, 0x10, 0x14, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x100013fd);
        drivemem_set_func(cpud, 0x14, 0x18, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x140017fd);
        drivemem_set_func(cpud, 0x18, 0x1c, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x18001bfd);
        drivemem_set_func(cpud, 0x1c, 0x20, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x1c001ffd);
        drivemem_set_func(cpud, 0x20, 0x34, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x200023fd);
        drivemem_set_func(cpud, 0x24, 0x38, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x240027fd);
        drivemem_set_func(cpud, 0x28, 0x3c, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x28002bfd);
        drivemem_set_func(cpud, 0x2c, 0x40, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x2c002ffd);
        drivemem_set_func(cpud, 0x30, 0x34, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x300033fd);
        drivemem_set_func(cpud, 0x34, 0x38, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x340037fd);
        drivemem_set_func(cpud, 0x38, 0x3c, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x38003bfd);
        drivemem_set_func(cpud, 0x3c, 0x40, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x3c003ffd);
        drivemem_set_func(cpud, 0x40, 0x44, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x400043fd);
        drivemem_set_func(cpud, 0x44, 0x48, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x440047fd);
        drivemem_set_func(cpud, 0x48, 0x4c, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x48004bfd);
        drivemem_set_func(cpud, 0x4c, 0x50, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x4c004ffd);
        drivemem_set_func(cpud, 0x80, 0x100, drive_read_rom, NULL, drv->drive->trap_rom, 0x8000fffd);
        return;
    case DRIVE_TYPE_2040:
        drivemem_set_func(cpud, 0x60, 0x80, drive_read_rom, NULL, &drv->drive->trap_rom[0x2000], 0x60007ffd);
        drivemem_set_func(cpud, 0xe0, 0x100, drive_read_rom, NULL, &drv->drive->trap_rom[0x2000], 0xe000fffd);
        break;
    case DRIVE_TYPE_3040:
    case DRIVE_TYPE_4040:
        drivemem_set_func(cpud, 0x50, 0x80, drive_read_rom, NULL, &drv->drive->trap_rom[0x1000], 0x50007ffd);
        drivemem_set_func(cpud, 0xd0, 0x100, drive_read_rom, NULL, &drv->drive->trap_rom[0x1000], 0xd000fffd);
        break;
    default:
        return;
    }

    drv->cpu->pageone = drv->drive->drive_ram;
    drivemem_set_func(cpud, 0x00, 0x02, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x000000fd);  
    drivemem_set_func(cpud, 0x02, 0x04, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x04, 0x06, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x040004fd);  
    drivemem_set_func(cpud, 0x06, 0x08, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x08, 0x0a, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x080008fd);  
    drivemem_set_func(cpud, 0x0a, 0x0c, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x0c, 0x0e, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x0c000cfd);  
    drivemem_set_func(cpud, 0x0e, 0x10, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x10, 0x14, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x100013fd);
    drivemem_set_func(cpud, 0x14, 0x18, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x140017fd);
    drivemem_set_func(cpud, 0x18, 0x1c, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x18001bfd);
    drivemem_set_func(cpud, 0x1c, 0x20, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x1c001ffd);
    drivemem_set_func(cpud, 0x20, 0x34, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x200023fd);
    drivemem_set_func(cpud, 0x24, 0x38, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x240027fd);
    drivemem_set_func(cpud, 0x28, 0x3c, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x28002bfd);
    drivemem_set_func(cpud, 0x2c, 0x40, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0x2c002ffd);
    drivemem_set_func(cpud, 0x30, 0x34, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x300033fd);
    drivemem_set_func(cpud, 0x34, 0x38, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x340037fd);
    drivemem_set_func(cpud, 0x38, 0x3c, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x38003bfd);
    drivemem_set_func(cpud, 0x3c, 0x40, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0x3c003ffd);
    drivemem_set_func(cpud, 0x40, 0x44, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x400043fd);
    drivemem_set_func(cpud, 0x44, 0x48, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x440047fd);
    drivemem_set_func(cpud, 0x48, 0x4c, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x48004bfd);
    drivemem_set_func(cpud, 0x4c, 0x50, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0x4c004ffd);
    drivemem_set_func(cpud, 0x80, 0x82, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x000000fd);   
    drivemem_set_func(cpud, 0x82, 0x84, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x84, 0x86, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x040004fd);   
    drivemem_set_func(cpud, 0x86, 0x88, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x88, 0x8a, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x080008fd);   
    drivemem_set_func(cpud, 0x8a, 0x8c, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x8c, 0x8e, drive_read_1001zero_ram, drive_store_1001zero_ram, drv->drive->drive_ram, 0x0c000cfd);   
    drivemem_set_func(cpud, 0x8e, 0x90, drive_read_1001_io, drive_store_1001_io, NULL, 0);
    drivemem_set_func(cpud, 0x90, 0x94, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x900093fd);
    drivemem_set_func(cpud, 0x94, 0x98, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x940097fd);
    drivemem_set_func(cpud, 0x98, 0x9c, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x98009bfd);
    drivemem_set_func(cpud, 0x9c, 0xa0, drive_read_2040buffer1_ram, drive_store_2040buffer1_ram, &drv->drive->drive_ram[0x0100], 0x9c009ffd);
    drivemem_set_func(cpud, 0xa0, 0xb4, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0xa000a3fd);
    drivemem_set_func(cpud, 0xa4, 0xb8, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0xa400a7fd);
    drivemem_set_func(cpud, 0xa8, 0xbc, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0xa800abfd);
    drivemem_set_func(cpud, 0xac, 0xc0, drive_read_2040buffer2_ram, drive_store_2040buffer2_ram, &drv->drive->drive_ram[0x0500], 0xac00affd);
    drivemem_set_func(cpud, 0xb0, 0xb4, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0xb000b3fd);
    drivemem_set_func(cpud, 0xb4, 0xb8, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0xb400b7fd);
    drivemem_set_func(cpud, 0xb8, 0xbc, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0xb800bbfd);
    drivemem_set_func(cpud, 0xbc, 0xc0, drive_read_2040buffer3_ram, drive_store_2040buffer3_ram, &drv->drive->drive_ram[0x0900], 0xbc00bffd);
    drivemem_set_func(cpud, 0xc0, 0xc4, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0xc000c3fd);
    drivemem_set_func(cpud, 0xc4, 0xc8, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0xc400c7fd);
    drivemem_set_func(cpud, 0xc8, 0xcc, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0xc800cbfd);
    drivemem_set_func(cpud, 0xcc, 0xd0, drive_read_2040buffer4_ram, drive_store_2040buffer4_ram, &drv->drive->drive_ram[0x0d00], 0xcc00cffd);
}
