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

#include "sid-snapshot.h"
#include "types.h"

#define HS_MAXSID 2

extern int hardsid_open(void);
extern int hardsid_close(void);
extern void hardsid_reset(void);
extern int hardsid_read(WORD addr, int chipno);
extern void hardsid_store(WORD addr, BYTE val, int chipno);
extern void hardsid_set_machine_parameter(long cycles_per_sec);
extern int hardsid_available(void);
extern void hardsid_set_device(unsigned int chipno, unsigned int device);

extern int hardsid_drv_open(void);
extern int hardsid_drv_close(void);
extern void hardsid_drv_reset(void);
extern int hardsid_drv_read(WORD addr, int chipno);
extern void hardsid_drv_store(WORD addr, BYTE val, int chipno);
extern int hardsid_drv_available(void);
extern void hardsid_drv_set_device(unsigned int chipno, unsigned int device);

extern void hardsid_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
extern void hardsid_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);

extern void hardsid_drv_state_read(int chipno, struct sid_hs_snapshot_state_s *sid_state);
extern void hardsid_drv_state_write(int chipno, struct sid_hs_snapshot_state_s *sid_state);

#endif
