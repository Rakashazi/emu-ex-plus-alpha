/*
 * ramlink.h - Cartridge handling, CMD Ramlink
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

#ifndef VICE_RAMLINK_H
#define VICE_RAMLINK_H

struct snapshot_s;

extern void ramlink_freeze(void);
extern void ramlink_config_init(void);
extern void ramlink_config_setup(uint8_t *rawcart);
extern int ramlink_bin_attach(const char *filename, uint8_t *rawcart);
extern void ramlink_detach(void);
extern int ramlink_crt_attach(FILE *fd, uint8_t *rawcart);
extern int ramlink_flush_image(void);
extern const char *ramlink_get_file_name(void);
extern int ramlink_cart_enabled(void);
extern int ramlink_bin_save(const char *filename);

extern uint8_t ramlink_roml_read(uint16_t addr);
extern uint8_t ramlink_romh_read(uint16_t addr);
extern void ramlink_roml_store(uint16_t addr, uint8_t value);
extern void ramlink_romh_store(uint16_t addr, uint8_t value);
extern uint8_t ramlink_a000_bfff_read(uint16_t addr);
extern int ramlink_peek_mem(export_t *export, uint16_t addr, uint8_t *value);

extern int ramlink_cmdline_options_init(void);
extern int ramlink_resources_init(void);
extern int ramlink_resources_shutdown(void);

extern int ramlink_snapshot_write_module(struct snapshot_s *s);
extern int ramlink_snapshot_read_module(struct snapshot_s *s);

#endif
