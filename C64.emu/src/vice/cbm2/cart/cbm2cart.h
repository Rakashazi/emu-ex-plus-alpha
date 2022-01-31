/*
 * cbm2cart.h --CBM2 cartridge memory interface.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#ifndef VICE_CBM2CART_H
#define VICE_CBM2CART_H

#include "types.h"

/* Expansion port signals.  */

/*
    CSBANK1      P
    CSBANK2      R
    CSBANK3      S

    R/!W         M    Read/Write (write active low)
    phi2         N    Phase 2 system clock

    D7-D0     J- A    Data bus bit 7-0
    A12-A0   13- 1    Address bus bit 0-12
*/

/* WARNING: due to the way VICE is being built/linked, this struct has to be
            the same in C64/DTV/C128 and CBM2 (ie c64cart.h and cbm2cart.h) */
typedef struct {
    uint8_t exrom;          /* (C64) exrom signal, 0 - active */
    uint8_t game;           /* (C64) game signal, 0 - active */
    uint8_t ultimax_phi1;   /* (C64) flag for vic-ii, ultimax mode in phi1 phase */
    uint8_t ultimax_phi2;   /* (C64) flag for vic-ii, ultimax mode in phi2 phase */
} export_t;

/* this is referenced by the VICII emulation */
extern export_t export;

void cart_power_off(void);

uint8_t *ultimax_romh_phi1_ptr(uint16_t addr);
uint8_t *ultimax_romh_phi2_ptr(uint16_t addr);

#endif
