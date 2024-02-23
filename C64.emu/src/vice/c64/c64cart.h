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
#define C64CART_IMAGE_LIMIT (C64CART_ROM_LIMIT + (16 * 1024 * 1024))

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

    bidirectional:

    D7-D0    14-21    Data bus bit 7-0 - unbuffered, 1 ls ttl load max
    A15-A0    F- Y    Address bus bit 0-15 - unbuffered, 1 ls ttl load max
*/

/* WARNING: due to the way VICE is being built/linked, this struct has to be
            the same in C64/DTV/C128 and CBM2 (ie c64cart.h and cbm2cart.h) */
typedef struct {
    uint8_t exrom;          /* exrom signal, 0 - active */
    uint8_t game;           /* game signal, 0 - active */
    uint8_t ultimax_phi1;   /* flag for vic-ii, ultimax mode in phi1 phase */
    uint8_t ultimax_phi2;   /* flag for vic-ii, ultimax mode in phi2 phase */
} export_t;

/* this is referenced by the VICII emulation */
extern export_t export;

/* expose public API symbols for those headers that provide them */
#define CARTRIDGE_INCLUDE_PUBLIC_API
#include "cart/expert.h"        /* provide defines for ExpertCartridgeMode resource */
#include "cart/retroreplay.h"   /* provide defines for RRrevision resource */
#include "cart/mmc64.h"         /* provide defines for MMC64_sd_type and MMC64_revision resources */
#include "cart/mmcreplay.h"     /* provide defines for MMCRSDType resource */
#ifdef HAVE_RAWNET
#include "cart/ethernetcart.h"  /* provide defines for ETHERNETCARTMode resource */
#endif
#undef CARTRIDGE_INCLUDE_PUBLIC_API

/* the following is used to hook up the c128 mode in x128 */
#include <stdio.h>
#include "cartridge.h"

struct c128cartridge_interface_s {
    int (*attach_crt)(int type, FILE *fd, const char *filename, uint8_t *rawcart);
    int (*bin_attach)(int type, const char *filename, uint8_t *rawcart);
    int (*bin_save)(int type, const char *filename);
    int (*save_secondary_image)(int type, const char *filename);
    int (*crt_save)(int type, const char *filename);
    int (*flush_image)(int type);
    int (*flush_secondary_image)(int type);
    void (*config_init)(int type);
    void (*config_setup)(int type, uint8_t *rawcart);
    void (*detach_image)(int type);
    void (*reset)(void);
    int (*freeze_allowed)(void);
    void (*freeze)(void);
    void (*powerup)(void);
    cartridge_info_t* (*get_info_list)(void);
    int (*can_flush_image)(int type);
    int (*can_flush_secondary_image)(int type);
    int (*can_save_image)(int type);
    int (*can_save_secondary_image)(int type);
};
typedef struct c128cartridge_interface_s c128cartridge_interface_t;

extern c128cartridge_interface_t *c128cartridge; /* lives in c64cart.c */

/* only x128 actually implements this function */
void c128cartridge_setup_interface(void);

#endif
