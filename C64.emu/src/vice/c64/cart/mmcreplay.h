/*
 * mmcreplay.h - Cartridge handling, MMCReplay cart.
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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

#ifndef VICE_MMCREPLAY_H
#define VICE_MMCREPLAY_H

#include "types.h"

#define MMCR_TYPE_AUTO  0
#define MMCR_TYPE_MMC   1
#define MMCR_TYPE_SD    2
#define MMCR_TYPE_SDHC  3

/* FIXME get rid of this */
#define MMCREPLAY_EEPROM_SIZE (1024)

extern BYTE mmcreplay_roml_read(WORD addr);
extern void mmcreplay_roml_store(WORD addr, BYTE value);
extern BYTE mmcreplay_romh_read(WORD addr);
extern void mmcreplay_romh_store(WORD addr, BYTE value);

extern BYTE mmcreplay_1000_7fff_read(WORD addr);
extern void mmcreplay_1000_7fff_store(WORD addr, BYTE value);
extern BYTE mmcreplay_a000_bfff_read(WORD addr);
extern void mmcreplay_a000_bfff_store(WORD addr, BYTE value);
extern BYTE mmcreplay_c000_cfff_read(WORD addr);
extern void mmcreplay_c000_cfff_store(WORD addr, BYTE value);

extern int mmcreplay_romh_phi1_read(WORD addr, BYTE *value);
extern int mmcreplay_romh_phi2_read(WORD addr, BYTE *value);

extern void mmcreplay_freeze(void);
extern int mmcreplay_freeze_allowed(void);

extern void mmcreplay_config_init(void);
extern void mmcreplay_reset(void);
extern void mmcreplay_config_setup(BYTE *rawcart);
extern int mmcreplay_bin_attach(const char *filename, BYTE *rawcart);
extern int mmcreplay_crt_attach(FILE *fd, BYTE *rawcart, const char *filename);
extern void mmcreplay_detach(void);
extern int mmcreplay_flush_image(void);
extern int mmcreplay_bin_save(const char *filename);
extern int mmcreplay_crt_save(const char *filename);

extern int mmcreplay_resources_init(void);
extern void mmcreplay_resources_shutdown(void);
extern int mmcreplay_cmdline_options_init(void);

extern int mmcreplay_cart_enabled(void);
extern int mmcr_clockport_enabled; /* FIXME */

struct snapshot_s;
extern int mmcreplay_snapshot_read_module(struct snapshot_s *s);
extern int mmcreplay_snapshot_write_module(struct snapshot_s *s);

#endif
