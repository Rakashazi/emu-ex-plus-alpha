/*
 * isepic.h - ISEPIC cart emulation.
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

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_ISEPIC_H
#define VICE_ISEPIC_H

#include "types.h"

extern int isepic_cart_enabled(void);
extern int isepic_cart_active(void);
extern int isepic_freeze_allowed(void);
extern void isepic_freeze(void);
extern void isepic_reset(void);
extern void isepic_config_init(void);
extern void isepic_config_setup(uint8_t *rawcart);
extern void isepic_powerup(void);

extern int isepic_resources_init(void);
extern void isepic_resources_shutdown(void);
extern int isepic_cmdline_options_init(void);

extern uint8_t isepic_romh_read(uint16_t addr);
extern void isepic_romh_store(uint16_t addr, uint8_t byte);
extern uint8_t isepic_page_read(uint16_t addr);
extern void isepic_page_store(uint16_t addr, uint8_t byte);
extern int isepic_romh_phi1_read(uint16_t addr, uint8_t *value);
extern int isepic_romh_phi2_read(uint16_t addr, uint8_t *value);
extern int isepic_peek_mem(uint16_t addr, uint8_t *value);
extern void isepic_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

extern int isepic_bin_attach(const char *filename, uint8_t *rawcart);
extern int isepic_bin_save(const char *filename);
extern int isepic_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
extern int isepic_crt_save(const char *filename);
extern int isepic_flush_image(void);
extern void isepic_detach(void);
extern int isepic_enable(void);
int isepic_disable(void);

extern const char *isepic_get_file_name(void);

struct snapshot_s;
extern int isepic_snapshot_read_module(struct snapshot_s *s);
extern int isepic_snapshot_write_module(struct snapshot_s *s);

#endif
