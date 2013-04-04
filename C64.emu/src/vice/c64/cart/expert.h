/*
 * expert.h - Cartridge handling, Expert cart.
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

#ifdef CARTRIDGE_INCLUDE_PRIVATE_API

#ifndef VICE_EXPERT_H
#define VICE_EXPERT_H

#include <stdio.h>

#include "types.h"

extern BYTE expert_roml_read(WORD addr);
extern void expert_roml_store(WORD addr, BYTE value);
extern void expert_raml_store(WORD addr, BYTE value);
extern BYTE expert_romh_read(WORD addr);
extern int expert_romh_phi1_read(WORD addr, BYTE *value);
extern int expert_romh_phi2_read(WORD addr, BYTE *value);
extern int expert_peek_mem(WORD addr, BYTE *value);

extern void expert_reset(void);
extern void expert_freeze(void);

extern void expert_config_init(void);
extern void expert_config_setup(BYTE *rawcart);
extern int expert_bin_attach(const char *filename, BYTE *rawcart);
extern int expert_bin_save(const char *filename);
extern int expert_crt_attach(FILE *fd, BYTE *rawcart, const char *filename);
extern int expert_crt_save(const char *filename);
extern int expert_flush_image(void);

extern void expert_detach(void);
extern int expert_enable(void);

extern int expert_freeze_allowed(void);
extern int expert_cart_enabled(void);
extern void expert_mmu_translate(unsigned int addr, BYTE **base, int *start, int *limit);

struct snapshot_s;
extern int expert_snapshot_write_module(struct snapshot_s *s);
extern int expert_snapshot_read_module(struct snapshot_s *s);

extern int expert_resources_init(void);
extern void expert_resources_shutdown(void);
extern int expert_cmdline_options_init(void);

extern const char *expert_get_file_name(void);

#endif /* VICE_EXPERT_H */
#endif /* CARTRIDGE_INCLUDE_PRIVATE_API */

#ifndef VICE_EXPERT_PUBLIC_H
#define VICE_EXPERT_PUBLIC_H

/* Expert cartridge has three modes: */
#define EXPERT_MODE_OFF 0
#define EXPERT_MODE_PRG 1
#define EXPERT_MODE_ON 2
#define EXPERT_MODE_DEFAULT EXPERT_MODE_PRG

#endif /* VICE_EXPERT_PUBLIC_H */

