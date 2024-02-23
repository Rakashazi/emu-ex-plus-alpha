/*
 * joyport_io_sim.h
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

#ifndef VICE_JOYPORT_IO_SIM_H
#define VICE_JOYPORT_IO_SIM_H

#include "types.h"

int joyport_io_sim_resources_init(void);

void joyport_io_sim_set_out_lines(uint8_t val, int port);
uint8_t joyport_io_sim_get_out_lines(int port);
uint8_t joyport_io_sim_get_in_lines(int port);

void joyport_io_sim_set_potx(uint8_t val, int port);
void joyport_io_sim_set_poty(uint8_t val, int port);

uint8_t joyport_io_sim_get_potx(int port);
uint8_t joyport_io_sim_get_poty(int port);

#endif
