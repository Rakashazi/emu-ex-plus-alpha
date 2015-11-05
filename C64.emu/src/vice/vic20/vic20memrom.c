/*
 * vic20memrom.c -- VIC20 ROM access.
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

#include "vic20mem.h"
#include "vic20memrom.h"
#include "types.h"

#ifdef USE_EMBEDDED
#include "vic20basic.h"
#include "vic20kernal.h"
#else
BYTE vic20memrom_basic_rom[VIC20_BASIC_ROM_SIZE];
BYTE vic20memrom_kernal_rom[VIC20_KERNAL_ROM_SIZE];
#endif

BYTE vic20memrom_kernal_trap_rom[VIC20_KERNAL_ROM_SIZE];

BYTE vic20memrom_chargen_rom[VIC20_CHARGEN_ROM_SIZE];


BYTE vic20memrom_kernal_read(WORD addr)
{
    return vic20memrom_kernal_rom[addr & 0x1fff];
}

BYTE vic20memrom_basic_read(WORD addr)
{
    return vic20memrom_basic_rom[addr & 0x1fff];
}

BYTE vic20memrom_chargen_read(WORD addr)
{
    return vic20memrom_chargen_rom[addr & 0xfff];
}

BYTE vic20memrom_trap_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            return vic20memrom_kernal_trap_rom[addr & 0x1fff];
    }

    return 0;
}

void vic20memrom_trap_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            vic20memrom_kernal_trap_rom[addr & 0x1fff] = value;
            break;
    }
}

BYTE rom_read(WORD addr)
{
    switch (addr & 0xf000) {
        case 0x8000:
            return vic20memrom_chargen_read(addr);
        case 0xc000:
        case 0xd000:
            return vic20memrom_basic_read(addr);
        case 0xe000:
        case 0xf000:
            return vic20memrom_kernal_read(addr);
    }

    return 0;
}

void rom_store(WORD addr, BYTE value)
{
    switch (addr & 0xf000) {
        case 0x8000:
            vic20memrom_chargen_rom[addr & 0x0fff] = value;
            break;
        case 0xc000:
        case 0xd000:
            vic20memrom_basic_rom[addr & 0x1fff] = value;
            break;
        case 0xe000:
        case 0xf000:
            vic20memrom_kernal_rom[addr & 0x1fff] = value;
            break;
    }
}
