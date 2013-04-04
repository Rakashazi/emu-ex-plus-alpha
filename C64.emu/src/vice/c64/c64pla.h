/*
 * c64pla.h -- C64 PLA handling.
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

#ifndef VICE_C64PLA_H
#define VICE_C64PLA_H

#include "types.h"

struct pport_s {
    /* Value written to processor port.  */
    BYTE dir;
    BYTE data;

    /* Value read from processor port.  */
    BYTE dir_read;
    BYTE data_read;

    /* State of processor port pins.  */
    BYTE data_out;

    /* cycle that should invalidate the unused bits of the data port. */
    CLOCK data_set_clk_bit6;
    CLOCK data_set_clk_bit7;

    /* indicates if the unused bits of the data port are still
       valid or should be read as 0, 1 = unused bits valid,
       0 = unused bits should be 0 */
    BYTE data_set_bit6;
    BYTE data_set_bit7;

    /* indicated if the unused bits are in the process of falling off. */
    BYTE data_falloff_bit6;
    BYTE data_falloff_bit7;
};
typedef struct pport_s pport_t;

extern pport_t pport;

extern void c64pla_config_changed(int tape_sense, int caps_sense, BYTE pullup);
extern void c64pla_pport_reset(void);

#endif
