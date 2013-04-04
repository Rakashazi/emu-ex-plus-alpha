/*
 * tcbm.h
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

#ifndef VICE_TCBM_H
#define VICE_TCBM_H

#include "types.h"

struct drive_context_s;
struct snapshot_s;

extern int tcbm_drive_resources_init(void);
extern void tcbm_drive_resources_shutdown(void);
extern int tcbm_drive_cmdline_options_init(void);
extern void tcbm_drive_init(struct drive_context_s *drv);
extern void tcbm_drive_shutdown(struct drive_context_s *drv);
extern void tcbm_drive_reset(struct drive_context_s *drv);
extern void tcbm_drive_mem_init(struct drive_context_s *drv, unsigned int type);
extern void tcbm_drive_setup_context(struct drive_context_s *drv);
extern void tcbm_drive_rom_load(void);
extern void tcbm_drive_rom_setup_image(unsigned int dnr);
extern int tcbm_drive_rom_read(unsigned int type, WORD addr, BYTE *data);
extern int tcbm_drive_rom_check_loaded(unsigned int type);
extern void tcbm_drive_rom_do_checksum(unsigned int dnr);
extern int tcbm_drive_snapshot_read(struct drive_context_s *ctxptr, struct snapshot_s *s);
extern int tcbm_drive_snapshot_write(struct drive_context_s *ctxptr, struct snapshot_s *s);

extern BYTE tpid_outputa[], tpid_outputb[], tpid_outputc[];

#endif
