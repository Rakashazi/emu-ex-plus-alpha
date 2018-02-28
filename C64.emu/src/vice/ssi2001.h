/*
 * ssi2001.h - SSI2001 (ISA SID card) support.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_SSI2001_H
#define VICE_SSI2001_H

#include "types.h"

#include "sid-snapshot.h"

extern int ssi2001_open(void);
extern int ssi2001_close(void);
extern int ssi2001_read(WORD addr, int chipno);
extern void ssi2001_store(WORD addr, BYTE val, int chipno);
extern void ssi2001_set_machine_parameter(long cycles_per_sec);

extern int ssi2001_available(void);

extern int ssi2001_drv_open(void);
extern int ssi2001_drv_close(void);
extern int ssi2001_drv_read(WORD addr, int chipno);
extern void ssi2001_drv_store(WORD addr, BYTE val, int chipno);
extern int ssi2001_drv_available(void);

extern void ssi2001_state_read(int chipno, struct sid_ssi2001_snapshot_state_s *sid_state);
extern void ssi2001_state_write(int chipno, struct sid_ssi2001_snapshot_state_s *sid_state);

#endif
