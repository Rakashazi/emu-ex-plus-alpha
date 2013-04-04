/*
 * reu.h - REU emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Richard Hable <K3027E7@edvz.uni-linz.ac.at>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_REU_H
#define VICE_REU_H

#include "types.h"

struct snapshot_s;

extern void reu_init(void);
extern int reu_resources_init(void);
extern void reu_resources_shutdown(void);
extern int reu_cmdline_options_init(void);

typedef int reu_ba_check_callback_t (void);
typedef void reu_ba_steal_callback_t (void);

extern void reu_ba_register(reu_ba_check_callback_t *ba_check,
                            reu_ba_steal_callback_t *ba_steal,
                            int *ba_var, int ba_mask);

extern void reu_reset(void);
extern void reu_dma(int immed);
extern void reu_dma_start(void);
extern void reu_detach(void);
extern int reu_enable(void);
extern int reu_read_snapshot_module(struct snapshot_s *s);
extern int reu_write_snapshot_module(struct snapshot_s *s);

extern int reu_cart_enabled(void);
extern void reu_config_setup(BYTE *rawcart);
extern const char *reu_get_file_name(void);
extern int reu_bin_attach(const char *filename, BYTE *rawcart);
extern int reu_bin_save(const char *filename);
extern int reu_flush_image(void);

#endif
