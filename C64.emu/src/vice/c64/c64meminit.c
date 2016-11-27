/*
 * c64meminit.c -- Initialize C64 memory.
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
#include "cartio.h"
#include "machine.h"
#include "resources.h"
#include "sid.h"
#include "vicii-mem.h"

/*

 missing: BA,CAS(not needed),RW(through tables),AEC

 bit 4 - !game
 bit 3 - !exrom
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

 8 0x08
 9 0x09                      chr
10 0x0a            romh      chr       ker
11 0x0b  roml      bas/romh  chr       ker      8k game
12 0x0c
13 0x0d                      io
14 0x0e            romh      io        ker
15 0x0f  roml      bas/romh  io        ker      8k game

16 0x10  roml      -         io        romh     ultimax
17 0x11  roml      -         io        romh     ultimax
18 0x12  roml      -         io        romh     ultimax
19 0x13  roml      -         io        romh     ultimax
20 0x14  roml      -         io        romh     ultimax
21 0x15  roml      -         io        romh     ultimax
22 0x16  roml      -         io        romh     ultimax
23 0x17  roml      -         io        romh     ultimax

24 0x18
25 0x19
26 0x1a            romh      chr       ker
27 0x1b  roml      romh      chr       ker      16k game
28 0x1c
29 0x1d                      io
30 0x1e            romh      io        ker
31 0x1f  roml      romh      io        ker      16k game

*/

/* IO is enabled at memory configs 5, 6, 7 and Ultimax.  */
const unsigned int c64meminit_io_config[32] = {
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    0, 0, 0, 0, 0, 1, 1, 1
};

/* ROML is enabled at memory configs 11, 15, 27, 31 and Ultimax.  */
static const unsigned int c64meminit_roml_config[32] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 1, 0, 0, 0, 1
};

/* ROMH is enabled at memory configs 10, 11, 14, 15, 26, 27, 30, 31
   and Ultimax.  */
static const unsigned int c64meminit_romh_config[32] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0, 1, 1
};

/* ROMH is mapped to $A000-$BFFF at memory configs 10, 11, 14, 15, 26,
   27, 30, 31.  If Ultimax is enabled it is mapped to $E000-$FFFF.  */
static const unsigned int c64meminit_romh_mapping[32] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0xe0, 0xe0, 0xe0, 0xe0,
    0xe0, 0xe0, 0xe0, 0xe0,
    0x00, 0x00, 0xa0, 0xa0,
    0x00, 0x00, 0xa0, 0xa0
};

void c64meminit(unsigned int base)
{
    unsigned int i, j;
    int board = 0;

    if (machine_class != VICE_MACHINE_C128) {
        resources_get_int("BoardType", &board);
    }

    if (board != 1) {
        /* Setup BASIC ROM at $A000-$BFFF (memory configs 3, 7, 11, 15).  */
        for (i = 0xa0; i <= 0xbf; i++) {
            mem_read_tab_set(base + 3, i, c64memrom_basic64_read);
            mem_read_tab_set(base + 7, i, c64memrom_basic64_read);
            mem_read_tab_set(base + 11, i, c64memrom_basic64_read);
            mem_read_tab_set(base + 15, i, c64memrom_basic64_read);
            mem_read_base_set(base + 3, i, c64memrom_basic64_rom - 0xa000);
            mem_read_base_set(base + 7, i, c64memrom_basic64_rom - 0xa000);
            mem_read_base_set(base + 11, i, c64memrom_basic64_rom - 0xa000);
            mem_read_base_set(base + 15, i, c64memrom_basic64_rom - 0xa000);
        }
    }

    /* Setup I/O at $D000-$DFFF (memory configs 5, 6, 7).  */
    for (j = 0; j < 32; j++) {
        if (c64meminit_io_config[j] == 1) {
            mem_read_tab_set(base + j, 0xd0, c64io_d000_read);
            mem_set_write_hook(base + j, 0xd0, c64io_d000_store);
            mem_read_tab_set(base + j, 0xd1, c64io_d100_read);
            mem_set_write_hook(base + j, 0xd1, c64io_d100_store);
            mem_read_tab_set(base + j, 0xd2, c64io_d200_read);
            mem_set_write_hook(base + j, 0xd2, c64io_d200_store);
            mem_read_tab_set(base + j, 0xd3, c64io_d300_read);
            mem_set_write_hook(base + j, 0xd3, c64io_d300_store);
            mem_read_tab_set(base + j, 0xd4, c64io_d400_read);
            mem_set_write_hook(base + j, 0xd4, c64io_d400_store);
            mem_read_tab_set(base + j, 0xd5, c64io_d500_read);
            mem_set_write_hook(base + j, 0xd5, c64io_d500_store);
            mem_read_tab_set(base + j, 0xd6, c64io_d600_read);
            mem_set_write_hook(base + j, 0xd6, c64io_d600_store);
            mem_read_tab_set(base + j, 0xd7, c64io_d700_read);
            mem_set_write_hook(base + j, 0xd7, c64io_d700_store);
            for (i = 0xd8; i <= 0xdb; i++) {
                mem_read_tab_set(base + j, i, colorram_read);
                mem_set_write_hook(base + j, i, colorram_store);
            }

            mem_read_tab_set(base + j, 0xdc, cia1_read);
            mem_set_write_hook(base + j, 0xdc, cia1_store);
            if (board != 1) {
                mem_read_tab_set(base + j, 0xdd, cia2_read);
                mem_set_write_hook(base + j, 0xdd, cia2_store);
            }

            mem_read_tab_set(base + j, 0xde, c64io_de00_read);
            mem_set_write_hook(base + j, 0xde, c64io_de00_store);
            mem_read_tab_set(base + j, 0xdf, c64io_df00_read);
            mem_set_write_hook(base + j, 0xdf, c64io_df00_store);

            for (i = 0xd0; i <= 0xdf; i++) {
                mem_read_base_set(base + j, i, NULL);
            }
        }
        if (c64meminit_io_config[j] == 2) {
            for (i = 0xd0; i <= 0xdf; i++) {
                mem_read_tab_set(base + j, i, ultimax_d000_dfff_read);
                mem_set_write_hook(base + j, i, ultimax_d000_dfff_store);
                mem_read_base_set(base + j, i, NULL);
            }
        }
    }

    if (board != 1) {
        /* Setup Kernal ROM at $E000-$FFFF (memory configs 2, 3, 6, 7, 10,
        11, 14, 15, 26, 27, 30, 31).  */
        for (i = 0xe0; i <= 0xff; i++) {
            mem_read_tab_set(base + 2, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 3, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 6, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 7, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 10, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 11, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 14, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 15, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 26, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 27, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 30, i, c64memrom_kernal64_read);
            mem_read_tab_set(base + 31, i, c64memrom_kernal64_read);
            mem_read_base_set(base + 2, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 3, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 6, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 7, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 10, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 11, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 14, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 15, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 26, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 27, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 30, i, c64memrom_kernal64_trap_rom - 0xe000);
            mem_read_base_set(base + 31, i, c64memrom_kernal64_trap_rom - 0xe000);
        }
    }

    /* Setup ROML at $8000-$9FFF.  */
    for (j = 0; j < 32; j++) {
        if (c64meminit_roml_config[j]) {
            for (i = 0x80; i <= 0x9f; i++) {
                mem_read_tab_set(base + j, i, roml_read);
                mem_read_base_set(base + j, i, NULL);
                mem_set_write_hook(base + j, i, roml_no_ultimax_store);
            }
        }
    }

    /* Setup write Hook for when ROML is NOT selected at $8000-$9FFF in 8K Game, 16K Game */
    for (j = 8; j < 16; j++) {
        if (!c64meminit_roml_config[j]) {
            for (i = 0x80; i <= 0x9f; i++) {
                mem_set_write_hook(base + j, i, raml_no_ultimax_store);
            }
        }
    }
    for (j = 24; j < 32; j++) {
        if (!c64meminit_roml_config[j]) {
            for (i = 0x80; i <= 0x9f; i++) {
                mem_set_write_hook(base + j, i, raml_no_ultimax_store);
            }
        }
    }
    /* Setup write Hook for when ROML is NOT selected at $8000-$9FFF when cart is off */
    for (j = 0; j < 8; j++) {
        if (!c64meminit_roml_config[j]) {
            for (i = 0x80; i <= 0x9f; i++) {
                mem_set_write_hook(base + j, i, raml_no_ultimax_store);
            }
        }
    }
    /* Setup ROMH at $A000-$BFFF */
    for (j = 24; j < 32; j++) {
        if (c64meminit_romh_config[j]) {
            for (i = c64meminit_romh_mapping[j]; i <= c64meminit_romh_mapping[j] + 0x1f; i++) {
                mem_read_tab_set(base + j, i, romh_read);
                mem_read_base_set(base + j, i, NULL);
                mem_set_write_hook(base + j, i, romh_no_ultimax_store);
            }
        }
    }
    /* Setup write Hook for when ROMH is NOT selected at $A000-$BFFF in 16K Game */
    for (j = 24; j < 32; j++) {
        if (!c64meminit_romh_config[j]) {
            for (i = 0xa0; i <= 0xbf; i++) {
                mem_set_write_hook(base + j, i, ramh_no_ultimax_store);
            }
        }
    }
    /* Setup write Hook for when ROMH is NOT selected at $a000-$bFFF when cart is off */
    for (j = 0; j < 8; j++) {
        if (!c64meminit_romh_config[j]) {
            for (i = 0xa0; i <= 0xbf; i++) {
                mem_set_write_hook(base + j, i, ramh_no_ultimax_store);
            }
        }
    }
    /* Setup ROMH at $E000-$FFFF (ultimax)  */
    for (j = 16; j < 24; j++) {
        if (c64meminit_romh_config[j]) {
            for (i = c64meminit_romh_mapping[j]; i <= c64meminit_romh_mapping[j] + 0x1f; i++) {
                if (j & 2) {
                    mem_read_tab_set(base + j, i, ultimax_romh_read_hirom);
                } else {
                    mem_read_tab_set(base + j, i, romh_read);
                }
                mem_read_base_set(base + j, i, NULL);
            }
        }
    }


    /* Setup Ultimax configuration.  */
    for (j = 16; j < 24; j++) {
        for (i = 0x10; i <= 0x7f; i++) {
            mem_read_tab_set(base + j, i, ultimax_1000_7fff_read);
            mem_set_write_hook(base + j, i, ultimax_1000_7fff_store);
            mem_read_base_set(base + j, i, NULL);
        }
        for (i = 0x80; i <= 0x9f; i++) {
            mem_set_write_hook(base + j, i, roml_store);
        }
        for (i = 0xa0; i <= 0xbf; i++) {
            mem_read_tab_set(base + j, i, ultimax_a000_bfff_read);
            mem_set_write_hook(base + j, i, ultimax_a000_bfff_store);
            mem_read_base_set(base + j, i, NULL);
        }
        for (i = 0xc0; i <= 0xcf; i++) {
            mem_read_tab_set(base + j, i, ultimax_c000_cfff_read);
            mem_set_write_hook(base + j, i, ultimax_c000_cfff_store);
            mem_read_base_set(base + j, i, NULL);
        }
        for (i = 0xe0; i <= 0xff; i++) {
            mem_set_write_hook(base + j, i, romh_store);
        }
    }
}
