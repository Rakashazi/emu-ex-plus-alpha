/** \file   vhkdebug.h
 * \brief   Debug messages for the hotkeys API - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
 */

#ifndef VICE_HOTKEYS_DEBUG_VHK_H
#define VICE_HOTKEYS_DEBUG_VHK_H

#include <stdio.h>

const char *debug_vhk_basename(const char *path);

/** \def debug_vhk
 * \brief   Print debugging message on stdout
 *
 * Print debugging message on stdout when `DEBUG_VHK` is defined. Works just
 * like printf() except that a newline is always added. The message is prefixed
 * with "[debug-vhk] <basename(__FILE__)>:<__LINE__>::<__func__>(): ".
 */

#ifdef DEBUG_VHK

# define debug_vhk(...) \
    printf("[debug-vhk] %s:%d::%s(): ", \
           debug_vhk_basename(__FILE__), __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n");
#else
# define debug_vhk(...)
#endif

#endif
