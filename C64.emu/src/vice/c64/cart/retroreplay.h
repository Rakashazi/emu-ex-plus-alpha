/*
 * retroreplay.h - Cartridge handling, Retro Replay cart.
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_RETROREPLAY_H
#define VICE_RETROREPLAY_H

#include <stdio.h>

#include "types.h"

struct snapshot_s;
struct export_s;

#define RR_REV_RETRO_REPLAY  0
#define RR_REV_NORDIC_REPLAY 1

extern BYTE retroreplay_roml_read(WORD addr);
extern void retroreplay_roml_store(WORD addr, BYTE value);
extern int retroreplay_roml_no_ultimax_store(WORD addr, BYTE value);
extern BYTE retroreplay_romh_read(WORD addr);
extern void retroreplay_romh_store(WORD addr, BYTE value);
extern int retroreplay_peek_mem(struct export_s *export, WORD addr, BYTE *value);
extern void retroreplay_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

extern void retroreplay_freeze(void);
extern int retroreplay_freeze_allowed(void);

extern void retroreplay_config_init(void);
extern void retroreplay_reset(void);
extern void retroreplay_config_setup(BYTE *rawcart);
extern int retroreplay_bin_attach(const char *filename, BYTE *rawcart);
extern int retroreplay_crt_attach(FILE *fd, BYTE *rawcart, const char *filename);
extern int retroreplay_bin_save(const char *filename);
extern int retroreplay_crt_save(const char *filename);
extern int retroreplay_flush_image(void);
extern void retroreplay_detach(void);

extern int retroreplay_cart_enabled(void);
extern int rr_clockport_enabled;

extern int retroreplay_cmdline_options_init(void);
extern int retroreplay_resources_init(void);
extern void retroreplay_resources_shutdown(void);

extern int retroreplay_snapshot_write_module(struct snapshot_s *s);
extern int retroreplay_snapshot_read_module(struct snapshot_s *s);

#endif
