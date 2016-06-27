/*
 * c64dtvpla.c -- C64 DTV PLA handling.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
 *
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

#include "c64pla.h"
#include "mem.h"


/* Processor port.  */
pport_t pport;

void c64pla_config_changed(int tape_sense, int caps_sense, BYTE pullup)
{
    BYTE dir = pport.dir & 0x3f;
    BYTE data = pport.data & 0x0f;

    pport.data_out = (pport.data_out & ~dir)
                     | (data & dir);

    pport.data_read = (data | ~dir) & (pport.data_out | pullup) & 0x0f;

    pport.dir_read = dir;
}

void c64pla_pport_reset(void)
{
    pport.data = 0x3f;
    pport.data_out = 0x3f;
    pport.data_read = 0x3f;
    pport.dir = 0;
    pport.dir_read = 0;
}
