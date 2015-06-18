/*
 * c64memlimit.c -- Builds the C64 memory limit table.
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

#include "c64memlimit.h"

#define NUM_SEGMENTS 7

static const int mstart[NUM_SEGMENTS] = {
    0x00, 0x10, 0x80,
    0xa0, 0xc0, 0xd0, 0xe0
};

static const int mend[NUM_SEGMENTS] = {
    0x0f, 0x7f, 0x9f,
    0xbf, 0xcf, 0xdf, 0xff
};

static const DWORD limit_tab[NUM_SEGMENTS][NUM_CONFIGS] = {
    /* 0000-0fff */
    { 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd, 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd,
      0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd, 0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd,
      0x00020ffd, 0x00020ffd, 0x00020ffd, 0x00020ffd, 0x00020ffd, 0x00020ffd, 0x00020ffd, 0x00020ffd,
      0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd, 0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd },

    /* 1000-7fff */
    { 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd, 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd,
      0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd, 0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd, 0x0002fffd, 0x0002cffd, 0x00029ffd, 0x00027ffd },

    /* 8000-9fff */
    { 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd, 0x0002fffd, 0x0002cffd, 0x0002cffd, 0x00029ffd,
      0x0002fffd, 0x0002cffd, 0x00029ffd,          0, 0x0002fffd, 0x0002cffd, 0x00029ffd,          0,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0x0002cffd, 0x00029ffd,          0, 0x0002fffd, 0x0002cffd, 0x00029ffd,          0 },

    /* a000-bfff */
    { 0x0002fffd, 0x0002cffd, 0x0002cffd, 0xa000bffd, 0x0002fffd, 0x0002cffd, 0x0002cffd, 0xa000bffd,
      0x0002fffd, 0x0002cffd,          0,          0, 0x0002fffd, 0x0002cffd,          0,          0,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0x0002cffd,          0,          0, 0x0002fffd, 0x0002cffd,          0,          0 },

    /* c000-cfff */
    { 0x0002fffd, 0x0002cffd, 0x0002cffd, 0xc000cffd, 0x0002fffd, 0x0002cffd, 0x0002cffd, 0xc000cffd,
      0x0002fffd, 0x0002cffd, 0xc000cffd, 0xc000cffd, 0x0002fffd, 0x0002cffd, 0xc000cffd, 0xc000cffd,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0x0002cffd, 0xc000cffd, 0xc000cffd, 0x0002fffd, 0x0002cffd, 0xc000cffd, 0xc000cffd },

    /* d000-dfff */
    { 0x0002fffd, 0xd000dffd, 0xd000dffd, 0xd000dffd, 0x0002fffd,          0,          0,          0,
      0x0002fffd, 0xd000dffd, 0xd000dffd, 0xd000dffd, 0x0002fffd,          0,          0,          0,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0xd000dffd, 0xd000dffd, 0xd000dffd, 0x0002fffd,          0,          0,          0 },

    /* e000-ffff */
    { 0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd, 0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd,
      0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd, 0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd,
               0,          0,          0,          0,          0,          0,          0,          0,
      0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd, 0x0002fffd, 0xe000fffd, 0xe000fffd, 0xe000fffd } };

void mem_limit_init(DWORD mem_read_limit_tab[NUM_CONFIGS][0x101])
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

void mem_limit_plus60k_init(DWORD mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                if (k < 0x10) {
                    mem_read_limit_tab[i][k] = 0x00020ffd;
                } else {
                    mem_read_limit_tab[i][k] = 0;
                }
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }
}

void mem_limit_256k_init(DWORD mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                mem_read_limit_tab[i][k] = 0;
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }
}

void mem_limit_max_init(DWORD mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                if (k < 0x8) {
                    mem_read_limit_tab[i][k] = 0x000207fd;
                } else {
                    mem_read_limit_tab[i][k] = 0;
                }
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }
}
