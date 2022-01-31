/*
 * c64dtvmemrom.c -- C64 DTV ROM access.
 *
 * Written by
 *  M.Kiesel <mayne@users.sourceforge.net>
 * Based on code by
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
#include "c64dtvmem.h"
#include "c64dtvflash.h"

/* These are read directly from flash in the DTV emulation and not used. */
uint8_t c64memrom_basic64_rom[C64_BASIC_ROM_SIZE];
uint8_t c64memrom_kernal64_rom[C64_KERNAL_ROM_SIZE];

uint8_t c64memrom_kernal64_trap_rom[C64_KERNAL_ROM_SIZE];

uint8_t c64memrom_kernal64_read(uint16_t addr)
{
    int mapping = c64dtvmem_memmapper[0];
    int paddr = ((mapping & 0x1f) << 16) + addr;
    if ((mapping >> 6) == 0) {
        return c64dtvflash_read(paddr);
    } else {
        return mem_ram[paddr];
    }
}

static void c64memrom_kernal64_store(uint16_t addr, uint8_t value)
{
    int mapping = c64dtvmem_memmapper[0];
    int paddr = ((mapping & 0x1f) << 16) + addr;
    if ((mapping >> 6) == 0) {
        c64dtvflash_store_direct(paddr, value);
    } else {
        mem_ram[paddr] = value;
    }
}

uint8_t c64memrom_basic64_read(uint16_t addr)
{
    int mapping = c64dtvmem_memmapper[1];
    int paddr = ((mapping & 0x1f) << 16) + addr;
    if ((mapping >> 6) == 0) {
        return c64dtvflash_read(paddr);
    } else {
        return mem_ram[paddr];
    }
}

/* static void c64memrom_basic64_store(uint16_t addr, uint8_t value)
{
}
*/

/* We don't use trap_rom in the DTV emulation. Traps are installed in */
/* flash/RAM directly and temporarily removed when accessing $d10x. */

uint8_t c64memrom_trap_read(uint16_t addr)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            return c64memrom_kernal64_read(addr);
            break;
    }
    return 0;
}

void c64memrom_trap_store(uint16_t addr, uint8_t value)
{
    switch (addr & 0xf000) {
        case 0xe000:
        case 0xf000:
            c64memrom_kernal64_store(addr, value);
            break;
    }
}

uint8_t c64memrom_rom64_read(uint16_t addr)
{
    switch (addr & 0xf000) {
        case 0xa000:
        case 0xb000:
        case 0xd000:
        case 0xe000:
        case 0xf000:
            return c64dtvflash_read(addr);
    }

    return 0;
}

void c64memrom_rom64_store(uint16_t addr, uint8_t value)
{
    return;
}
