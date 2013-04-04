/*
 * c64cart.h -- C64 cartridge memory interface.
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

#ifndef VICE_C64CART_H
#define VICE_C64CART_H

#include "types.h"

/* Cartridge ROM limit = 1MB (EasyFlash) */
#define C64CART_ROM_LIMIT (1024 * 1024)
/* Cartridge RAM limit = 32kB (IDE64, ...) */
#define C64CART_RAM_LIMIT (32 * 1024)
/* maximum size of a full "all inclusive" cartridge image (16MB for REU) */
#ifndef DINGOO_NATIVE
#define C64CART_IMAGE_LIMIT (C64CART_ROM_LIMIT + (16 * 1024 * 1024))
#else
#define C64CART_IMAGE_LIMIT (C64CART_ROM_LIMIT + 1024 * 1024)
#endif

/* Expansion port signals.  */

/*
    inputs:

    !RESET       C    6502 RESET pin(active low) buff'ed ttl out/unbuff'ed in
    !IRQ         4    Interrupt Request line to 6502 (active low)
    !NMI         D    6502 Non Maskable Interrupt (active low) buff'ed ttl out, unbuff'ed in
    !GAME        8    active low ls ttl input
    !EXROM       9    active low ls ttl input
    !DMA        13    Direct memory access request line (active low input) ls ttl input

    outputs:

     DOT CLOCK   6    8.18 MHz video dot clock
     phi2        E    Phase 2 system clock
     BA         12    Bus available signal from the VIC-II chip unbuffered 1 Is load max.
     R/!W        5    Read/Write (write active low)
    !I/O1        7    I/O block 1 @ $DE00-$DEFF (active low) unbuffered I/O
    !I/O2       10    I/O block 2 @ $DF00-$DFFF (active low) buff'ed ls ttl output
    !ROML       11    8K decoded RAM/ROM block @ $8000 (active low) buffered ls ttl output
    !ROMH        B    8K decoded RAM/ROM block @ $E000 buffered

    D7-D0    14-21    Data bus bit 7-0 - unbuffered, 1 ls ttl load max
    A15-A0    F- Y    Address bus bit 0-15 - unbuffered, 1 ls ttl load max
*/

typedef struct {
    BYTE exrom; /* exrom signal, 0 - active */
    BYTE game;  /* game signal, 0 - active */
    BYTE ultimax_phi1; /* flag for vic-ii, ultimax mode in phi1 phase */
    BYTE ultimax_phi2; /* flag for vic-ii, ultimax mode in phi2 phase */
} export_t;

extern export_t export;

#define CARTRIDGE_INCLUDE_PUBLIC_API
#include "cart/expert.h"
#undef CARTRIDGE_INCLUDE_PUBLIC_API

#endif
