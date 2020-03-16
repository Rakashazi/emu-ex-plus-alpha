/*
 * shortbus_digimax.h
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

#ifndef VICE_SHORTBUS_DIGIMAX_H
#define VICE_SHORTBUS_DIGIMAX_H

#include "snapshot.h"
#include "types.h"

extern int shortbus_digimax_resources_init(void);
extern void shortbus_digimax_resources_shutdown(void);

extern int shortbus_digimax_cmdline_options_init(void);

extern void shortbus_digimax_unregister(void);
extern void shortbus_digimax_register(void);

extern void shortbus_digimax_reset(void);

extern void shortbus_digimax_sound_chip_init(void);

extern int shortbus_digimax_enabled(void);

extern int shortbus_digimax_write_snapshot_module(snapshot_t *s);
extern int shortbus_digimax_read_snapshot_module(snapshot_t *s);

#endif
