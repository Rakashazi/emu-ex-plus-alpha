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

#include "c64cart.h"

struct snapshot_s;

#define LTKIO_DE00   0
#define LTKIO_DF00   1

/** \brief  Lowest disk image number */
#define LTK_HD_MIN   0
/** \brief  Highest disk image number */
#define LTK_HD_MAX   6
/** \brief  Number of supported disk images */
#define LTK_HD_COUNT (LTK_HD_MAX - LTK_HD_MIN + 1)

/** \brief  Lowest port number */
#define LTK_PORT_MIN 0
/** \brief  Highest port number */
#define LTK_PORT_MAX 15


void ltkernal_freeze(void);
void ltkernal_powerup(void);
void ltkernal_config_init(void);
void ltkernal_config_setup(uint8_t *rawcart);
int ltkernal_bin_attach(const char *filename, uint8_t *rawcart);
void ltkernal_detach(void);
int ltkernal_crt_attach(FILE *fd, uint8_t *rawcart);

uint8_t ltkernal_roml_read(uint16_t addr);
uint8_t ltkernal_romh_read(uint16_t addr);
void ltkernal_roml_store(uint16_t addr, uint8_t value);
void ltkernal_romh_store(uint16_t addr, uint8_t value);
int ltkernal_peek_mem(export_t *export, uint16_t addr, uint8_t *value);
int ltkernal_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit);

int c128ltkernal_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit, int mem_config);
uint8_t c128ltkernal_roml_read(uint16_t addr, uint8_t *value);
uint8_t c128ltkernal_roml_store(uint16_t addr, uint8_t value);
uint8_t c128ltkernal_basic_hi_read(uint16_t addr, uint8_t *value);
uint8_t c128ltkernal_basic_hi_store(uint16_t addr, uint8_t value);
uint8_t c128ltkernal_hi_read(uint16_t addr, uint8_t *value);
uint8_t c128ltkernal_hi_store(uint16_t addr, uint8_t value);
uint8_t c128ltkernal_ram_read(uint16_t addr, uint8_t *value);
uint8_t c128ltkernal_ram_store(uint16_t addr, uint8_t value);
void c128ltkernal_switch_mode(int mode);

int ltkernal_cmdline_options_init(void);
int ltkernal_resources_init(void);
int ltkernal_resources_shutdown(void);

int ltkernal_snapshot_write_module(struct snapshot_s *s);
int ltkernal_snapshot_read_module(struct snapshot_s *s);

#endif
