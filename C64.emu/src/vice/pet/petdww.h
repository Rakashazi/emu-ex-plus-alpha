/*
 * petdww.h -- RAM + PIA chip emulation.
 *
 * Written by
 *  Olaf 'Rhialto' Seibert <rhialto@falu.nl>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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

#ifndef VICE_PETDWW_H
#define VICE_PETDWW_H

#include "types.h"
#include "mem.h"

/*
 * Signal values (for signaling edges on the control lines).
 * They're not connected but the piacore code wants them.
 */

#define PIA_SIG_CA1     0
#define PIA_SIG_CA2     1
#define PIA_SIG_CB1     2
#define PIA_SIG_CB2     3

#define PIA_SIG_FALL    0
#define PIA_SIG_RISE    1

/* ------------------------------------------------------------------------- */

struct snapshot_s;

extern int petdww_enabled;

extern int petdww_init_resources(void);
extern int petdww_init_cmdline_options(void);
extern int petdww_resources_init(void);
extern void petdww_resources_shutdown(void);
extern int petdww_cmdline_options_init(void);

extern void petdww_init(void);
extern void petdww_powerup(void);
extern void petdww_reset(void);
extern void petdww_shutdown(void);
extern void petdww_override_std_9toa(read_func_ptr_t *mem_read_tab, store_func_ptr_t *mem_write_tab, BYTE **mem_base_tab, int *mem_limit_tab);
extern void petdww_restore_std_9toa(read_func_ptr_t *mem_read_tab, store_func_ptr_t *mem_write_tab, BYTE **mem_base_tab, int *mem_limit_tab);
extern void petdww_signal(int line, int edge);
extern BYTE read_petdww_reg(WORD addr);
extern BYTE read_petdww_ec00_ram(WORD addr);
extern void store_petdww_reg(WORD addr, BYTE byte);
extern void store_petdww_ec00_ram(WORD addr, BYTE byte);

extern int petdww_snapshot_read_module(struct snapshot_s *);
extern int petdww_snapshot_write_module(struct snapshot_s *);

extern int petdww_mem_at_9000(void);
extern int petdwwpia_dump(void);

extern BYTE *petdww_crtc_get_active_bitmap(void);

#endif
