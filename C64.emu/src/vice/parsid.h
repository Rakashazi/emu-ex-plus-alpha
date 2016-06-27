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

extern int parsid_check_port(int port);
extern void parsid_reset(void);
extern int parsid_open(int port);
extern int parsid_close(void);
extern int parsid_read(WORD addr, int chipno);
extern void parsid_store(WORD addr, BYTE outval, int chipno);

extern int parsid_drv_check_port(int port);
extern void parsid_drv_out_ctr(WORD parsid_ctrport);
extern BYTE parsid_drv_in_ctr(void);
extern int parsid_drv_init(void);
extern int parsid_drv_close(void);
extern BYTE parsid_drv_in_data(void);
extern void parsid_drv_out_data(BYTE addr);
extern void parsid_drv_sleep(int amount);

extern void parsid_state_read(int chipno, struct sid_parsid_snapshot_state_s *sid_state);
extern void parsid_state_write(int chipno, struct sid_parsid_snapshot_state_s *sid_state);

#endif
