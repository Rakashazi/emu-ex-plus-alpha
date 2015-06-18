/*
 * drivemem.h - Drive memory handling.
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

#ifndef VICE_DRIVEMEM_H
#define VICE_DRIVEMEM_H

#include "drivetypes.h"
#include "types.h"

struct drive_context_s;
struct drivecpud_context_s;
struct mem_ioreg_list_s;

extern void drivemem_toggle_watchpoints(int flag, void *context);
extern BYTE drivemem_bank_read(int bank, WORD addr, void *context);
extern BYTE drivemem_bank_peek(int bank, WORD addr, void *context);
extern void drivemem_bank_store(int bank, WORD addr, BYTE value, void *context);
extern void drivemem_init(struct drive_context_s *drv, unsigned int type);
extern void drivemem_set_func(struct drivecpud_context_s *cpud,
                              unsigned int start, unsigned int stop,
                              drive_read_func_t *read_func,
                              drive_store_func_t *store_func,
                              BYTE *base, DWORD limit);

extern struct mem_ioreg_list_s *drivemem_ioreg_list_get(void *context);

#endif
