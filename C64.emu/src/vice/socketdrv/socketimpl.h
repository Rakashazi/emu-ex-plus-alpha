/*
 * socketimpl.h - Socket-specific stuff.
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

#ifndef VICE_SOCKETIMPL_H
#define VICE_SOCKETIMPL_H

#include "vice.h"

#ifdef AMIGA_SUPPORT
#include "socket-amiga-impl.h"
#endif

#ifdef BEOS_COMPILE
#include "socket-beos-impl.h"
#endif

#ifdef UNIX_COMPILE
#include "socket-unix-impl.h"
#endif

#ifdef WIN32_COMPILE
#include "socket-win32-impl.h"
#endif

#endif
