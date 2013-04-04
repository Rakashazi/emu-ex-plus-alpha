/*
 * userport_digimax.c - Digimax DAC userport module emulation.
 *
 * Written by
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
#include <stdlib.h>
#include <string.h>

#include "digimax.h"
#include "userport_digimax.h"

/* DIGIMAX userport address latch */
static BYTE digimax_userport_address;

/* DIGIMAX userport direction latches */
static BYTE digimax_userport_direction_A;
static BYTE digimax_userport_direction_B;

/* ---------------------------------------------------------------------*/

/*
    PA2  low, /PA3  low: DAC #0 (left)
    PA2 high, /PA3  low: DAC #1 (right)
    PA2  low, /PA3 high: DAC #2 (left)
    PA2 high, /PA3 high: DAC #3 (right).
*/

static void digimax_userport_sound_store(BYTE value)
{
    WORD addr = 0;

    switch ((digimax_userport_address & digimax_userport_direction_A) & 0xc) {
        case 0x0:
            addr = 2;
            break;
        case 0x4:
            addr = 3;
            break;
        case 0x8:
            addr = 0;
            break;
        case 0xc:
            addr = 1;
            break;
    }

    digimax_sound_store(addr, (BYTE)(value & digimax_userport_direction_B));
}

void digimax_userport_store(WORD addr, BYTE value)
{
    switch (addr & 0x1f) {
        case 0:
            digimax_userport_address = value;
            break;
        case 1:
            if (digimax_cart_enabled() && digimax_is_userport()) {
                digimax_userport_sound_store(value);
            }
            break;
        case 2:
            digimax_userport_direction_A = value;
            break;
        case 3:
            digimax_userport_direction_B = value;
            break;
    }
}
