/*
 * c128memrom.c -- C128 ROM access.
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

#include "c128mem.h"
#include "c128memrom.h"
#include "types.h"
#include "z80mem.h"

BYTE c128memrom_basic_rom[C128_BASIC_ROM_SIZE + C128_EDITOR_ROM_SIZE];
BYTE c128memrom_kernal_rom[C128_KERNAL_ROM_SIZE];
BYTE c128memrom_kernal_trap_rom[C128_KERNAL_ROM_SIZE];

BYTE c128memrom_basic_read(WORD addr)
{
    return c128memrom_basic_rom[addr - 0x4000];
}

void c128memrom_basic_store(WORD addr, BYTE value)
{
    c128memrom_basic_rom[addr - 0x4000] = value;
}

BYTE c128memrom_kernal_read(WORD addr)
{
    return c128memrom_kernal_rom[addr & 0x1fff];
}

void c128memrom_kernal_store(WORD addr, BYTE value)
{
    c128memrom_kernal_rom[addr & 0x1fff] = value;
}

BYTE c128memrom_trap_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            return c128memrom_kernal_trap_rom[addr & 0x1fff];
    }

    return 0;
}

void c128memrom_trap_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            c128memrom_kernal_trap_rom[addr & 0x1fff] = value;
            break;
    }
}

BYTE c128memrom_rom_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0x0000:
            return bios_read(addr);
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
        case 0x8000:
        case 0x9000:
        case 0xa000:
        case 0xb000:
            return c128memrom_basic_read(addr);
        case 0xe000:
        case 0xf000:
            return c128memrom_kernal_read(addr);
    }

    return 0;
}

void c128memrom_rom_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0x0000:
            bios_store(addr, value);
            break;
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000:
        case 0x8000:
        case 0x9000:
        case 0xa000:
        case 0xb000:
            c128memrom_basic_store(addr, value);
            break;
        case 0xe000:
        case 0xf000:
            c128memrom_kernal_store(addr, value);
            break;
    }
}
