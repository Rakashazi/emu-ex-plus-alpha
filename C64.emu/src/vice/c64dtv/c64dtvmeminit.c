/*
 * c64dtvmeminit.c -- Initialize C64 memory.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#include <stdio.h>

#include "c64cia.h"
#include "c64mem.h"
#include "c64dtvmem.h"
#include "c64dtvmeminit.h"
#include "c64memrom.h"
#include "debugcart.h"
#include "sid.h"
#include "vicii-mem.h"

#define NUM_SEGMENTS 7
#define NUM_CONFIGS 8

static const int mstart[NUM_SEGMENTS] = {
    0x00, 0x40, 0x80,
    0xa0, 0xc0, 0xd0, 0xe0
};

static const int mend[NUM_SEGMENTS] = {
    0x3f, 0x7f, 0x9f,
    0xbf, 0xcf, 0xdf, 0xff
};

static const DWORD limit_tab[NUM_SEGMENTS][NUM_CONFIGS] = {
    /* 0000-3fff */
    { 0x00023ffd, 0x00023ffd, 0x00023ffd, 0x00023ffd, 0x00023ffd, 0x00023ffd, 0x00023ffd, 0x00023ffd },

    /* 4000-7fff */
    { 0x40007ffd, 0x40007ffd, 0x40007ffd, 0x40007ffd, 0x40007ffd, 0x40007ffd, 0x40007ffd, 0x40007ffd },

    /* 8000-9fff */
    { 0x8000bffd, 0x8000bffd, 0x8000bffd, 0x80009ffd, 0x8000bffd, 0x8000bffd, 0x8000bffd, 0x80009ffd },

    /* a000-bfff */
    { 0x8000bffd, 0x8000bffd, 0x8000bffd, 0xa000bffd, 0x8000bffd, 0x8000bffd, 0x8000bffd, 0xa000bffd },

    /* c000-cfff */
    { 0xc000fffd, 0xc000cffd, 0xc000cffd, 0xc000cffd, 0xc000fffd, 0xc000cffd, 0xc000cffd, 0xc000cffd },

    /* d000-dfff */
    { 0xc000fffd, 0xd000dffd, 0xd000dffd, 0xd000dffd, 0xc000fffd,          0,          0,          0 },

    /* e000-ffff */
    { 0xc000fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd, 0xc000fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd }
};

/* IO is enabled at memory configs 5, 6, 7 */
const unsigned int c64dtvmeminit_io_config[8] = { 0, 0, 0, 0, 0, 1, 1, 1 };

static void sid_store_d700(WORD addr, BYTE val)
{
    sid_store(addr, val);
    debugcart_store(addr, val);
}

void c64dtvmeminit(unsigned int base)
{
    unsigned int i, j;

    /* Setup BASIC ROM at $A000-$BFFF (memory configs 3, 7).  */
    for (i = 0xa0; i <= 0xbf; i++) {
        mem_read_tab_set(base + 3, i, c64memrom_basic64_read);
        mem_read_tab_set(base + 7, i, c64memrom_basic64_read);
        mem_read_base_set(base + 3, i, NULL);
        mem_read_base_set(base + 7, i, NULL);
    }

    /* Setup I/O at $D000-$DFFF (memory configs 5, 6, 7).  */
    for (j = 0; j < 8; j++) {
        if (c64dtvmeminit_io_config[j] == 1) {
            for (i = 0xd0; i <= 0xd3; i++) {
                mem_read_tab_set(base + j, i, vicii_read);
                mem_set_write_hook(base + j, i, vicii_store);
            }
            for (i = 0xd4; i <= 0xd6; i++) {
                mem_read_tab_set(base + j, i, sid_read);
                mem_set_write_hook(base + j, i, sid_store);
            }
            mem_set_write_hook(base + j, i, sid_store_d700);
            for (i = 0xd8; i <= 0xdb; i++) {
                mem_read_tab_set(base + j, i, colorram_read);
                mem_set_write_hook(base + j, i, colorram_store);
            }

            mem_read_tab_set(base + j, 0xdc, cia1_read);
            mem_set_write_hook(base + j, 0xdc, cia1_store);
            mem_read_tab_set(base + j, 0xdd, cia2_read);
            mem_set_write_hook(base + j, 0xdd, cia2_store);

            mem_read_tab_set(base + j, 0xde, c64io1_read);
            mem_set_write_hook(base + j, 0xde, c64io1_store);
            mem_read_tab_set(base + j, 0xdf, c64io2_read);
            mem_set_write_hook(base + j, 0xdf, c64io2_store);

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
        mem_read_base_set(base + 2, i, NULL);
        mem_read_base_set(base + 3, i, NULL);
        mem_read_base_set(base + 6, i, NULL);
        mem_read_base_set(base + 7, i, NULL);
    }
}

void c64dtvmem_limit_init(DWORD mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                mem_read_limit_tab[i][k] = limit_tab[j][i];
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }
}
