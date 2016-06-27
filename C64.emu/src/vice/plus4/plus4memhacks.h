/*
 * plus4memhacks.h - Plus4 memory expansion hacks control.
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

#ifndef VICE_PLUS4MEMORYHACKS_H
#define VICE_PLUS4MEMORYHACKS_H

#include "types.h"

#define MEMORY_HACK_NONE     0
#define MEMORY_HACK_C256K    1
#define MEMORY_HACK_H256K    2
#define MEMORY_HACK_H1024K   3
#define MEMORY_HACK_H4096K   4

extern int plus4_memory_hacks_resources_init(void);
extern int plus4_memory_hacks_cmdline_options_init(void);

#endif
