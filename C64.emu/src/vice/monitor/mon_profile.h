/*
 * mon_profiler.h -- Monitor Interface for CPU profiler
 *
 * Written by
 *  Oskar Linde <oskar.linde@gmail.com>
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

#ifndef VICE_MON_PROFILE_H
#define VICE_MON_PROFILE_H
#include "montypes.h"

/* monitor commands */
void mon_profile(void);
void mon_profile_action(ACTION action); /* on|off|toggle */
void mon_profile_flat(int num);
void mon_profile_graph(int context_id, int depth);
void mon_profile_func(MON_ADDR function);
void mon_profile_disass(MON_ADDR function);
void mon_profile_clear(MON_ADDR function);
void mon_profile_disass_context(int context_id);

#endif /* VICE_MON_PROFILE_H */
