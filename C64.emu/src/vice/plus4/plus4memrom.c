/*
 * plus4memrom.c -- Plus4 ROM access.
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

#include "plus4mem.h"
#include "plus4memrom.h"
#include "types.h"

#ifdef USE_EMBEDDED
#include "plus4basic.h"
#include "plus4kernal.h"
#else
BYTE plus4memrom_basic_rom[PLUS4_BASIC_ROM_SIZE];
BYTE plus4memrom_kernal_rom[PLUS4_KERNAL_ROM_SIZE];
#endif

BYTE plus4memrom_kernal_trap_rom[PLUS4_KERNAL_ROM_SIZE];


BYTE plus4memrom_kernal_read(WORD addr)
{
    return plus4memrom_kernal_rom[addr & 0x3fff];
}

void plus4memrom_kernal_store(WORD addr, BYTE value)
{
    plus4memrom_kernal_rom[addr & 0x3fff] = value;
}

BYTE plus4memrom_basic_read(WORD addr)
{
    return plus4memrom_basic_rom[addr & 0x3fff];
}

void plus4memrom_basic_store(WORD addr, BYTE value)
{
    plus4memrom_basic_rom[addr & 0x3fff] = value;
}

BYTE plus4memrom_trap_read(WORD addr)
{
    switch (addr & 0xc000) {
        case 0xc000:
            return plus4memrom_kernal_trap_rom[addr & 0x3fff];
    }

    return 0;
}

void plus4memrom_trap_store(WORD addr, BYTE value)
{
    switch (addr & 0xc000) {
        case 0xc000:
            plus4memrom_kernal_trap_rom[addr & 0x3fff] = value;
            break;
    }
}

BYTE plus4memrom_extromlo1_read(WORD addr)
{
    return extromlo1[addr & 0x3fff];
}

BYTE plus4memrom_extromlo2_read(WORD addr)
{
    return extromlo2[addr & 0x3fff];
}

BYTE plus4memrom_extromlo3_read(WORD addr)
{
    return extromlo3[addr & 0x3fff];
}

BYTE plus4memrom_extromhi1_read(WORD addr)
{
    return extromhi1[addr & 0x3fff];
}

BYTE plus4memrom_extromhi2_read(WORD addr)
{
    return extromhi2[addr & 0x3fff];
}

BYTE plus4memrom_extromhi3_read(WORD addr)
{
    return extromhi3[addr & 0x3fff];
}

BYTE plus4memrom_rom_read(WORD addr)
{
    switch (addr & 0xc000) {
        case 0x8000:
            switch ((mem_config >> 1) & 3) {
                case 0:
                    return plus4memrom_basic_read(addr);
                case 1:
                    return plus4memrom_extromlo1_read(addr);
                case 2:
                    return plus4memrom_extromlo2_read(addr);
                case 3:
                    return plus4memrom_extromlo3_read(addr);
            }
        case 0xc000:
            if ((addr & 0xff00) == 0xfc00) {
                return plus4memrom_kernal_read(addr);
            } else {
                switch ((mem_config >> 3) & 3) {
                    case 0:
                        return plus4memrom_kernal_read(addr);
                    case 1:
                        return plus4memrom_extromhi1_read(addr);
                    case 2:
                        return plus4memrom_extromhi2_read(addr);
                    case 3:
                        return plus4memrom_extromhi3_read(addr);
                }
            }
    }

    return 0;
}

void plus4memrom_rom_store(WORD addr, BYTE value)
{
    switch (addr & 0xc000) {
        case 0x8000:
            plus4memrom_basic_rom[addr & 0x3fff] = value;
            break;
        case 0xc000:
            plus4memrom_kernal_rom[addr & 0x3fff] = value;
            break;
    }
}
