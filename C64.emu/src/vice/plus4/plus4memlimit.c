/*
 * plus4memlimit.c -- Builds the Plus4 memory limit table.
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

#include "mem.h"
#include "plus4memcsory256k.h"
#include "plus4memhannes256k.h"
#include "plus4memlimit.h"


#define NUM_SEGMENTS 6
#define NUM_CONFIGS 32


static const int mstart[NUM_SEGMENTS] = { 0x00, 0x40, 0x80, 0xc0, 0xfc, 0xfd };

static const int mend[NUM_SEGMENTS] = { 0x3f, 0x7f, 0xbf, 0xfb, 0xfc, 0xff };


static const int limit_tab[NUM_SEGMENTS][NUM_CONFIGS] = {
    /* 0000-3fff */
    { 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd,
      0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd,
      0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd,
      0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd, 0x3ffd },

    /* 4000-7fff */
    { 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
      0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
      0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd,
      0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd, 0x7ffd },

    /* 8000-bfff */
    { 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd,
      0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd,
      0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd,
      0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd, 0xbffd },

    /* c000-fbff */
    { 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd,
      0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd,
      0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd,
      0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd, 0xfcfd, 0xfbfd },

    /* fc00-fcff */
    { 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd,
      0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd,
      0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd,
      0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd, 0xfcfd },

    /* fd00-ffff */
    {      0,      0,      0,      0,      0,      0,      0,      0,
           0,      0,      0,      0,      0,      0,      0,      0,
           0,      0,      0,      0,      0,      0,      0,      0,
           0,      0,      0,      0,      0,      0,      0,      0 }
};


void mem_limit_init(int mem_read_limit_tab[NUM_CONFIGS][0x101])
{
    int i, j, k;

    for (i = 0; i < NUM_CONFIGS; i++) {
        for (j = 0; j < NUM_SEGMENTS; j++) {
            for (k = mstart[j]; k <= mend[j]; k++) {
                if (h256k_enabled && k < 0x10) {
                    mem_read_limit_tab[i][k] = 0x0ffd;
                }
                if (h256k_enabled && k >= 0x10) {
                    mem_read_limit_tab[i][k] = 0;
                }
                if (cs256k_enabled) {
                    mem_read_limit_tab[i][k] = 0;
                }
                if (!h256k_enabled && !cs256k_enabled) {
                    mem_read_limit_tab[i][k] = limit_tab[j][i];
                }
            }
        }
        mem_read_limit_tab[i][0x100] = 0;
    }
}
