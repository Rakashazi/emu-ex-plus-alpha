/*
 * c64memrom.c -- C64 ROM access.
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

#include "c64mem.h"
#include "c64memrom.h"
#include "types.h"

#ifdef USE_EMBEDDED
#include "c64basic.h"
#include "c64kernal.h"
#else
BYTE c64memrom_basic64_rom[C64_BASIC_ROM_SIZE];
BYTE c64memrom_kernal64_rom[C64_KERNAL_ROM_SIZE];
#endif

BYTE c64memrom_kernal64_trap_rom[C64_KERNAL_ROM_SIZE];

BYTE c64memrom_kernal64_read(WORD addr)
{
    return c64memrom_kernal64_rom[addr & 0x1fff];
}

static void c64memrom_kernal64_store(WORD addr, BYTE value)
{
    c64memrom_kernal64_rom[addr & 0x1fff] = value;
}

BYTE c64memrom_basic64_read(WORD addr)
{
    return c64memrom_basic64_rom[addr & 0x1fff];
}

static void c64memrom_basic64_store(WORD addr, BYTE value)
{
    c64memrom_basic64_rom[addr & 0x1fff] = value;
}

BYTE c64memrom_trap_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            return c64memrom_kernal64_trap_rom[addr & 0x1fff];
    }

    return 0;
}

void c64memrom_trap_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            c64memrom_kernal64_trap_rom[addr & 0x1fff] = value;
            break;
    }
}

BYTE c64memrom_rom64_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0xa000:
        case 0xb000:
            return c64memrom_basic64_read(addr);
        case 0xd000:
            return chargen_read(addr);
        case 0xe000:
        case 0xf000:
            return c64memrom_kernal64_read(addr);
    }

    return 0;
}

void c64memrom_rom64_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0xa000:
        case 0xb000:
            c64memrom_basic64_store(addr, value);
            break;
        case 0xd000:
            chargen_store(addr, value);
            break;
        case 0xe000:
        case 0xf000:
            c64memrom_kernal64_store(addr, value);
            break;
    }
}
