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

enum {
    MMCR_TYPE_AUTO = 0,
    MMCR_TYPE_MMC,
    MMCR_TYPE_SD,
    MMCR_TYPE_SDHC
};

/* FIXME get rid of this */
#define MMCREPLAY_EEPROM_SIZE (1024)

uint8_t mmcreplay_roml_read(uint16_t addr);
void mmcreplay_roml_store(uint16_t addr, uint8_t value);
uint8_t mmcreplay_romh_read(uint16_t addr);
void mmcreplay_romh_store(uint16_t addr, uint8_t value);

uint8_t mmcreplay_1000_7fff_read(uint16_t addr);
void mmcreplay_1000_7fff_store(uint16_t addr, uint8_t value);
uint8_t mmcreplay_a000_bfff_read(uint16_t addr);
void mmcreplay_a000_bfff_store(uint16_t addr, uint8_t value);
uint8_t mmcreplay_c000_cfff_read(uint16_t addr);
void mmcreplay_c000_cfff_store(uint16_t addr, uint8_t value);

uint8_t mmcreplay_c128_read(uint16_t addr, uint8_t *value);
void mmcreplay_c128_switch_mode(int mode);

int mmcreplay_romh_phi1_read(uint16_t addr, uint8_t *value);
int mmcreplay_romh_phi2_read(uint16_t addr, uint8_t *value);

void mmcreplay_freeze(void);
int mmcreplay_freeze_allowed(void);

void mmcreplay_config_init(void);
void mmcreplay_reset(void);
void mmcreplay_config_setup(uint8_t *rawcart);
int mmcreplay_bin_attach(const char *filename, uint8_t *rawcart);
int mmcreplay_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename);
void mmcreplay_detach(void);
int mmcreplay_flush_image(void);
int mmcreplay_bin_save(const char *filename);
int mmcreplay_crt_save(const char *filename);
void mmcreplay_powerup(void);

int mmcreplay_save_eeprom(const char *filename);
int mmcreplay_flush_eeprom(void);
int mmcreplay_can_flush_eeprom(void);

int mmcreplay_resources_init(void);
void mmcreplay_resources_shutdown(void);
int mmcreplay_cmdline_options_init(void);

int mmcreplay_cart_enabled(void);
extern int mmcr_clockport_enabled; /* FIXME */

struct snapshot_s;

int mmcreplay_snapshot_read_module(struct snapshot_s *s);
int mmcreplay_snapshot_write_module(struct snapshot_s *s);

#endif

