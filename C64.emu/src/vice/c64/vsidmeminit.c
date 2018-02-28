/*
 * vsidmeminit.c -- Initialize C64 memory for the VSID emulator.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "c64cart.h"
#include "c64cartmem.h"
#include "c64cia.h"
#include "c64mem.h"
#include "c64meminit.h"
#include "c64memrom.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "sid-resources.h"
#include "vsid-debugcart.h"
#include "vicii-mem.h"
#include "vicii-phi1.h"

/*

 missing: BA,CAS(not needed),RW(through tables),AEC

 bit 2 - loram
 bit 1 - hiram
 bit 0 - charen

         8000      a000      d000      e000

 0 0x00
 1 0x01                      chr
 2 0x02                      chr       ker
 3 0x03            bas       chr       ker
 4 0x04
 5 0x05                      io
 6 0x06                      io        ker
 7 0x07            bas       io        ker

*/

/* IO is enabled at memory configs 5, 6, 7 and mirrors since game/exrom will never be changed.  */
const unsigned int c64meminit_io_config[32] = {
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

BYTE vsid_io_read(WORD addr)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        return sid_read(addr);
    }

    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        return sid_read(addr);
    }

    return vicii_read_phi1();
}

void vsid_io_store(WORD addr, BYTE val)
{
    if (sid_stereo >= 1
        && addr >= sid_stereo_address_start
        && addr < sid_stereo_address_end) {
        sid_store(addr, val);
    }

    if (sid_stereo >= 2
        && addr >= sid_triple_address_start
        && addr < sid_triple_address_end) {
        sid_store(addr, val);
    }
}

static void sid_store_d700(WORD addr, BYTE val)
{
    sid_store(addr, val);
    debugcart_store(addr, val);
}

void c64meminit(unsigned int base)
{
    unsigned int i, j;

    /* Setup BASIC ROM at $A000-$BFFF (memory configs 3 and 7).  */
    for (i = 0xa0; i <= 0xbf; i++) {
        mem_read_tab_set(base + 3, i, c64memrom_basic64_read);
        mem_read_tab_set(base + 7, i, c64memrom_basic64_read);
        mem_read_base_set(base + 3, i, c64memrom_basic64_rom - 0xa000);
        mem_read_base_set(base + 7, i, c64memrom_basic64_rom - 0xa000);
    }

    /* Setup I/O at $D000-$DFFF (memory configs 5, 6, 7).  */
    for (j = 0; j < 8; j++) {
        if (c64meminit_io_config[j] == 1) {
            mem_read_tab_set(base + j, 0xd0, vicii_read);
            mem_set_write_hook(base + j, 0xd0, vicii_store);
            mem_read_tab_set(base + j, 0xd1, vicii_read);
            mem_set_write_hook(base + j, 0xd1, vicii_store);
            mem_read_tab_set(base + j, 0xd2, vicii_read);
            mem_set_write_hook(base + j, 0xd2, vicii_store);
            mem_read_tab_set(base + j, 0xd3, vicii_read);
            mem_set_write_hook(base + j, 0xd3, vicii_store);
            mem_read_tab_set(base + j, 0xd4, sid_read);
            mem_set_write_hook(base + j, 0xd4, sid_store);
            mem_read_tab_set(base + j, 0xd5, sid_read);
            mem_set_write_hook(base + j, 0xd5, sid_store);
            mem_read_tab_set(base + j, 0xd6, sid_read);
            mem_set_write_hook(base + j, 0xd6, sid_store);
            mem_read_tab_set(base + j, 0xd7, sid_read);
            mem_set_write_hook(base + j, 0xd7, sid_store_d700);
            for (i = 0xd8; i <= 0xdb; i++) {
                mem_read_tab_set(base + j, i, colorram_read);
                mem_set_write_hook(base + j, i, colorram_store);
            }

            mem_read_tab_set(base + j, 0xdc, cia1_read);
            mem_set_write_hook(base + j, 0xdc, cia1_store);
            mem_read_tab_set(base + j, 0xdd, cia2_read);
            mem_set_write_hook(base + j, 0xdd, cia2_store);

            mem_read_tab_set(base + j, 0xde, vsid_io_read);
            mem_set_write_hook(base + j, 0xde, vsid_io_store);
            mem_read_tab_set(base + j, 0xdf, vsid_io_read);
            mem_set_write_hook(base + j, 0xdf, vsid_io_store);

            for (i = 0xd0; i <= 0xdf; i++) {
                mem_read_base_set(base + j, i, NULL);
            }
        }
    }

    /* Setup Kernal ROM at $E000-$FFFF (memory configs 2, 3, 6, 7).  */
    for (i = 0xe0; i <= 0xff; i++) {
        mem_read_tab_set(base + 2, i, c64memrom_kernal64_read);
        mem_read_tab_set(base + 3, i, c64memrom_kernal64_read);
        mem_read_tab_set(base + 6, i, c64memrom_kernal64_read);
        mem_read_tab_set(base + 7, i, c64memrom_kernal64_read);
        mem_read_base_set(base + 2, i, c64memrom_kernal64_rom - 0xe000);
        mem_read_base_set(base + 3, i, c64memrom_kernal64_rom - 0xe000);
        mem_read_base_set(base + 6, i, c64memrom_kernal64_rom - 0xe000);
        mem_read_base_set(base + 7, i, c64memrom_kernal64_rom - 0xe000);
    }
}
