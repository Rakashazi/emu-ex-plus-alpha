/*
 * c128meminit.c -- Initialize C128 memory.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#include "c128mem.h"
#include "c128meminit.h"
#include "c128memrom.h"
#include "c128mmu.h"
#include "c128cart.h"
#include "c64cia.h"
#include "cartio.h"
#include "functionrom.h"
#include "sid.h"
#include "vdc-mem.h"
#include "vicii-mem.h"

#define NUM_CONFIGS64  32
#define NUM_CONFIGS128 256
#define NUM_CONFIGS (NUM_CONFIGS64+NUM_CONFIGS128)

/*
 The tables are now built on the value in register 0 of the MMU.

 Take the MMU value:

  bit 7  : Expansion (A17 when in 256K mode)
  bit 6  : A16
  bit 5-4: ROM HIGH (00=ROM,01=INT,10=EXT,11=RAM)
  bit 3-2: ROM MID (00=ROM,01=INT,10=EXT,11=RAM)
  bit 1  : ROM LO (0=ROM,1=RAM)
  bit 0  : I/O (0=I/O,1=CHARGEN)

  This config starts at entry 32 (NUM_CONFIG64). This was done as C64 carts
   assume base of c64 configurations are at 0. There is now better
   accommodations for 256K configurations.
*/

void c128meminit(int base)
{
    unsigned int i, j;

    for (j = 0; j < NUM_CONFIGS128; j++) {
        for (i = 0; i <= 0x100; i++) {
            mem_read_base_set(j + base, i, NULL);
            mem_read_limit_set(j + base, i, 0);
        }

        /* 0x0000 - 0x00ff */
        mem_read_tab_set(j + base, 0, zero_read);
        mem_set_write_hook(j + base, 0, zero_store);

        /* 0x0100 - 0x01ff */
        mem_read_tab_set(j + base, 1, one_read);
        mem_set_write_hook(j + base, 1, one_store);

        /* 0x0200 - 0x3fff */
        for (i = 0x02; i <= 0x3f; i++) {
            if ((j & 0xc0) == 0) {
                /* bank 0 */
                mem_read_tab_set(j + base, i, ram_read);
                mem_set_write_hook(j + base, i, ram_store);
                mem_read_base_set(j + base, i, NULL /* mem_ram */);
/*                mem_read_limit_set(j + base, i, 0x00023fff - 2); */
            } else {
                /* other banks, ie. shared RAM */
                mem_read_tab_set(j + base, i, lo_read);
                mem_set_write_hook(j + base, i, lo_store);
            }
        }

        /* 0x4000 - 0x7fff */
        for (i = 0x40; i <= 0x7f; i++) {
            if ((j & 0x2) == 0) {
                mem_read_tab_set(j + base, i, basic_lo_read);
                mem_set_write_hook(j + base, i, basic_lo_store);
                mem_read_base_set(j + base, i, c128memrom_basic_rom - 0x4000);
                mem_read_limit_set(j + base, i, 0x40007fff - 2);
            } else {
                mem_read_tab_set(j + base, i, ram_read);
                mem_set_write_hook(j + base, i, ram_store);
/*                mem_read_base_set(j + base, i, mem_ram + 0x10000 * (j >> 6) ); */
/*                mem_read_limit_set(j + base, i, 0x40007fff - 2); */
            }
        }

        /* 0x8000 - 0xbfff */
        for (i = 0x80; i <= 0xbf; i++) {
            switch ((j >> 2) & 3) {
            case 0:
                mem_read_tab_set(j + base, i, basic_hi_read);
                mem_set_write_hook(j + base, i, basic_hi_store);
                mem_read_base_set(j + base, i, c128memrom_basic_rom - 0x4000);
                mem_read_limit_set(j + base, i, 0x8000bfff - 2);
                break;
            case 1:
                mem_read_tab_set(j + base, i, internal_function_rom_read);
                mem_set_write_hook(j + base, i, internal_function_rom_store);
                break;
            case 2:
                mem_read_tab_set(j + base, i, external_function_rom_read);
                mem_set_write_hook(j + base, i, external_function_rom_store);
                break;
            case 3:
                mem_read_tab_set(j + base, i, ram_read);
                mem_set_write_hook(j + base, i, ram_store);
/*                mem_read_base_set(j + base, i, mem_ram + 0x10000 * (j >> 6) ); */
/*                mem_read_limit_set(j + base, i, 0x8000bfff - 2); */
                break;
            default:
                break;
            }
        }

        /* 0xc000 - 0xcfff */
        for (i = 0xc0; i <= 0xcf; i++) {
            switch ((j >> 4) & 3) {
            case 0:
                mem_read_tab_set(j + base, i, editor_read);
                mem_set_write_hook(j + base, i, editor_store);
                mem_read_base_set(j + base, i, c128memrom_basic_rom - 0x4000);
                mem_read_limit_set(j + base, i, 0xc000cfff - 2);
                break;
            case 1:
                mem_read_tab_set(j + base, i, internal_function_rom_read);
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_set_write_hook(j + base, i, internal_function_rom_store);
                } else {
                    /* other banks, ie. shared RAM */
                    mem_set_write_hook(j + base, i, internal_function_top_shared_store);
                }
                break;
            case 2:
                mem_read_tab_set(j + base, i, external_function_rom_read);
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_set_write_hook(j + base, i, external_function_rom_store);
                } else {
                    /* other banks, ie. shared RAM */
                    mem_set_write_hook(j + base, i, external_function_top_shared_store);
                }
                break;
            case 3:
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_read_tab_set(j + base, i, ram_read);
                    mem_set_write_hook(j + base, i, ram_store);
                    mem_read_base_set(j + base, i, mem_ram);
                    mem_read_limit_set(j + base, i, 0xc000cfff - 2);
                } else {
                    /* other banks, ie. shared RAM */
                    mem_read_tab_set(j + base, i, top_shared_read);
                    mem_set_write_hook(j + base, i, top_shared_store);
                }
                break;
            default:
                break;
            }
        }

        /* 0xd000 - 0xdfff */
        /* non-I/O ie. ROM and CHARGEN */
        if ((j & 1)) {
            for (i = 0xd0; i <= 0xdf; i++) {
                switch ((j >> 4) & 3) {
                case 0:
                    mem_read_tab_set(j + base, i, chargen_read);
                    mem_set_write_hook(j + base, i, hi_store);
                    break;
                case 1:
                    if ((j & 0xc0) == 0) {
                        /* bank 0 */
                        mem_read_tab_set(j + base, i, internal_function_rom_read);
                        mem_set_write_hook(j + base, i, internal_function_rom_store);
                    } else {
                        /* other banks, ie. shared RAM */
                        mem_read_tab_set(j + base, i, internal_function_rom_read);
                        mem_set_write_hook(j + base, i, internal_function_top_shared_store);
                    }
                    break;
                case 2:
                    if ((j & 0xc0) == 0) {
                        /* bank 0 */
                        mem_read_tab_set(j + base, i, external_function_rom_read);
                        mem_set_write_hook(j + base, i, external_function_rom_store);
                    } else {
                        /* other banks, ie. shared RAM */
                        mem_read_tab_set(j + base, i, external_function_rom_read);
                        mem_set_write_hook(j + base, i, external_function_top_shared_store);
                    }
                    break;
                case 3:
                    if ((j & 0xc0) == 0) {
                        /* bank 0 */
                        mem_read_tab_set(j + base, i, ram_read);
                        mem_set_write_hook(j + base, i, ram_store);
                        mem_read_base_set(j + base, i, mem_ram);
                        mem_read_limit_set(j + base, i, 0xd000dfff - 2);
                    } else {
                        /* other banks, ie. shared RAM */
                        mem_read_tab_set(j + base, i, top_shared_read);
                        mem_set_write_hook(j + base, i, top_shared_store);
                    }
                    break;
                default:
                    break;
                }
            }
        } else {
        /* I/O */
            mem_read_tab_set(j + base, 0xd0, c128_c64io_d000_read);
            mem_set_write_hook(j + base, 0xd0, c128_c64io_d000_store);
            mem_read_tab_set(j + base, 0xd1, c128_c64io_d100_read);
            mem_set_write_hook(j + base, 0xd1, c128_c64io_d100_store);
            mem_read_tab_set(j + base, 0xd2, c128_c64io_d200_read);
            mem_set_write_hook(j + base, 0xd2, c128_c64io_d200_store);
            mem_read_tab_set(j + base, 0xd3, c128_c64io_d300_read);
            mem_set_write_hook(j + base, 0xd3, c128_c64io_d300_store);

            mem_read_tab_set(j + base, 0xd4, c128_c64io_d400_read);
            mem_set_write_hook(j + base, 0xd4, c128_c64io_d400_store);
            mem_read_tab_set(j + base, 0xd5, c128_mmu_read);
            mem_set_write_hook(j + base, 0xd5, c128_mmu_store);
            mem_read_tab_set(j + base, 0xd6, c128_c64io_d600_read);
            mem_set_write_hook(j + base, 0xd6, c128_c64io_d600_store);
            mem_read_tab_set(j + base, 0xd7, c128_c64io_d700_read);
            mem_set_write_hook(j + base, 0xd7, c128_c64io_d700_store);

            mem_read_tab_set(j + base, 0xd8, c128_colorram_read);
            mem_read_tab_set(j + base, 0xd9, c128_colorram_read);
            mem_read_tab_set(j + base, 0xda, c128_colorram_read);
            mem_read_tab_set(j + base, 0xdb, c128_colorram_read);
            mem_set_write_hook(j + base, 0xd8, c128_colorram_store);
            mem_set_write_hook(j + base, 0xd9, c128_colorram_store);
            mem_set_write_hook(j + base, 0xda, c128_colorram_store);
            mem_set_write_hook(j + base, 0xdb, c128_colorram_store);

            mem_read_tab_set(j + base, 0xdc, c128_cia1_read);
            mem_set_write_hook(j + base, 0xdc, c128_cia1_store);
            mem_read_tab_set(j + base, 0xdd, c128_cia2_read);
            mem_set_write_hook(j + base, 0xdd, c128_cia2_store);

            mem_read_tab_set(j + base, 0xde, c128_c64io_de00_read);
            mem_set_write_hook(j + base, 0xde, c128_c64io_de00_store);
            mem_read_tab_set(j + base, 0xdf, c128_c64io_df00_read);
            mem_set_write_hook(j + base, 0xdf, c128_c64io_df00_store);
        }

        /* 0xe000 - 0xfeff */
        for (i = 0xe0; i <= 0xfe; i++) {
            switch ((j >> 4) & 3) {
            case 0:
                mem_read_tab_set(j + base, i, hi_read);
                mem_set_write_hook(j + base, i, hi_store);
                mem_read_base_set(j + base, i, c128memrom_kernal_trap_rom - 0xe000);
                mem_read_limit_set(j + base, i, 0xe000feff - 2);
                break;
            case 1:
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_read_tab_set(j + base, i, internal_function_rom_read);
                    mem_set_write_hook(j + base, i, internal_function_rom_store);
                } else {
                    /* other banks, ie. shared RAM */
                    mem_read_tab_set(j + base, i, internal_function_rom_read);
                    mem_set_write_hook(j + base, i, internal_function_top_shared_store);
                }
                break;
            case 2:
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_read_tab_set(j + base, i, external_function_rom_read);
                    mem_set_write_hook(j + base, i, external_function_rom_store);
                } else {
                    /* other banks, ie. shared RAM */
                    mem_read_tab_set(j + base, i, external_function_rom_read);
                    mem_set_write_hook(j + base, i, external_function_top_shared_store);
                }
                break;
            case 3:
                if ((j & 0xc0) == 0) {
                    /* bank 0 */
                    mem_read_tab_set(j + base, i, ram_read);
                    mem_set_write_hook(j + base, i, ram_store);
/*                    mem_read_base_set(j + base, i, mem_ram); */
/*                    mem_read_limit_set(j + base, i, 0xe000feff - 2); */
                } else {
                    /* other banks, ie. shared RAM */
                    mem_read_tab_set(j + base, i, top_shared_read);
                    mem_set_write_hook(j + base, i, top_shared_store);
                }
                break;
            default:
                break;
            }
        }

        /* 0xff00 - 0xffff */
        mem_read_tab_set(j + base, 0xff, mmu_ffxx_read);
        mem_set_write_hook(j + base, 0xff, mmu_ffxx_store);

        /* 0x10000 - 0x100ff */
        mem_read_tab_set(j + base, 0x100, zero_read);
        mem_set_write_hook(j + base, 0x100, zero_store);
        mem_read_base_set(j + base, 0x100, NULL);
    }
}
