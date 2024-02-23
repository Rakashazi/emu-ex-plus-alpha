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

void parsid_reset(void);
int parsid_open(void);
int parsid_close(void);
int parsid_read(uint16_t addr, int chipno);
void parsid_store(uint16_t addr, uint8_t outval, int chipno);
int parsid_available(void);

int parsid_drv_open(void);
void parsid_drv_out_ctr(uint8_t parsid_ctrport, int chipno);
uint8_t parsid_drv_in_ctr(int chipno);
int parsid_drv_close(void);
uint8_t parsid_drv_in_data(int chipno);
void parsid_drv_out_data(uint8_t addr, int chipno);
void parsid_drv_sleep(int amount);
int parsid_drv_available(void);

void parsid_state_read(int chipno, struct sid_parsid_snapshot_state_s *sid_state);
void parsid_state_write(int chipno, struct sid_parsid_snapshot_state_s *sid_state);

#endif
