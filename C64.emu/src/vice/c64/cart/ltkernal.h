/*
 * ltkernal64.h - Cartridge handling, Final cart.
 *
 * Written by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#ifndef VICE_LTKERNAL_H
#define VICE_LTKERNAL_H

struct snapshot_s;

extern void ltkernal_freeze(void);
extern void ltkernal_config_init(void);
extern void ltkernal_config_setup(uint8_t *rawcart);
extern int ltkernal_bin_attach(const char *filename, uint8_t *rawcart);
extern void ltkernal_detach(void);
extern int ltkernal_crt_attach(FILE *fd, uint8_t *rawcart);

extern uint8_t ltkernal_roml_read(uint16_t addr);
extern uint8_t ltkernal_romh_read(uint16_t addr);
extern void ltkernal_roml_store(uint16_t addr, uint8_t value);
extern void ltkernal_romh_store(uint16_t addr, uint8_t value);
extern int ltkernal_peek_mem(export_t *export, uint16_t addr, uint8_t *value);

extern int ltkernal_cmdline_options_init(void);
extern int ltkernal_resources_init(void);
extern int ltkernal_resources_shutdown(void);

extern int ltkernal_snapshot_write_module(struct snapshot_s *s);
extern int ltkernal_snapshot_read_module(struct snapshot_s *s);

#endif
