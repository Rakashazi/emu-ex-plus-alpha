/*
 * parsid.h - PARallel port SID support.
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

#ifndef VICE_PARSID_H
#define VICE_PARSID_H

#include "types.h"
#include "sid-snapshot.h"

/* control register bits */
#define parsid_STROBE   0x01
#define parsid_AUTOFEED 0x02
#define parsid_nINIT    0x04
#define parsid_SELECTIN 0x08
#define parsid_PCD      0x20

extern void parsid_reset(void);
extern int parsid_open(void);
extern int parsid_close(void);
extern int parsid_read(WORD addr, int chipno);
extern void parsid_store(WORD addr, BYTE outval, int chipno);
extern int parsid_available(void);

extern int parsid_drv_open(void);
extern void parsid_drv_out_ctr(BYTE parsid_ctrport, int chipno);
extern BYTE parsid_drv_in_ctr(int chipno);
extern int parsid_drv_close(void);
extern BYTE parsid_drv_in_data(int chipno);
extern void parsid_drv_out_data(BYTE addr, int chipno);
extern void parsid_drv_sleep(int amount);
extern int parsid_drv_available(void);

extern void parsid_state_read(int chipno, struct sid_parsid_snapshot_state_s *sid_state);
extern void parsid_state_write(int chipno, struct sid_parsid_snapshot_state_s *sid_state);

#endif
