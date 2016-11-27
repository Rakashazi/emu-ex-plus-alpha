/*
 * mon_memmap.h - The VICE built-in monitor, memmap/cpuhistory functions.
 *
 * Written by
 *  Hannu Nuotio <nojoopa@users.sourceforge.net>
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

#ifndef VICE_MON_MEMMAP_H
#define VICE_MON_MEMMAP_H

#include "montypes.h"
#include "types.h"

extern void mon_memmap_init(void);
extern void mon_memmap_shutdown(void);

extern void mon_cpuhistory(int count);

extern void mon_memmap_zap(void);
extern void mon_memmap_show(int mask, MON_ADDR start_addr, MON_ADDR end_addr);
extern void mon_memmap_save(const char* filename, int format);

#endif
