/*
 * shortbus.h
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

#ifndef VICE_SHORTBUS_H
#define VICE_SHORTBUS_H

#include "types.h"

extern int shortbus_resources_init(void);
extern void shortbus_resources_shutdown(void);

extern int shortbus_cmdline_options_init(void);

extern void shortbus_unregister(void);
extern void shortbus_register(void);

extern void shortbus_reset(void);

extern int shortbus_write_snapshot_module(snapshot_t *s);
extern int shortbus_read_snapshot_module(snapshot_t *s);

#endif
