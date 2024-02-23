/*
 * hardsid.h
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_HARDSID_H
#define VICE_HARDSID_H

#ifdef HAVE_HARDSID

#include "sid-snapshot.h"
#include "types.h"

#define HS_MAXSID 2

int hardsid_open(void);
int hardsid_close(void);
void hardsid_reset(void);
int hardsid_read(uint16_t addr, int chipno);
void hardsid_store(uint16_t addr, uint8_t val, int chipno);
void hardsid_set_machine_parameter(long cycles_per_sec);
void hardsid_set_device(unsigned int chipno, unsigned int device);

int hardsid_drv_open(void);
int hardsid_drv_close(void);
void hardsid_drv_reset(void);
int hardsid_drv_read(uint16_t addr, int chipno);
void hardsid_drv_store(uint16_t addr, uint8_t val, int chipno);
int hardsid_drv_available(void);
void hardsid_drv_set_device(unsigned int chipno, unsigned int device);

void hardsid_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hardsid_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);

void hardsid_drv_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
void hardsid_drv_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);
#endif

int hardsid_available(void);

#endif
