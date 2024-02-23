/** \file   debug_vhk.c
 * \brief   Debug messages for the hotkeys API
 *
 * To enable printing of debug messages by the shared hotkeys code, define
 * `DEBUG_VHK` before including `vhkdebug.h`.
 * Format used for the strings is:
 * "[debug-vhk] <basename(__FILE__)>:<__LINE__>::<__func__> ...".
 * (The [ and ] in the line above are literal characters, not indicating an
 * optional component.)
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

#include "vice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "archdep.h"

#include "vhkdebug.h"


/** \brief  Get basename of a path
 *
 * Get basename of \a path, return pointer into \a path beyond the last
 * directory separator token, if present.
 *
 * \param[in]   path    file system path
 *
 * \return  basename
 */
const char *debug_vhk_basename(const char *path)
{
    if (path != NULL && *path != '\0') {
        const char *p = path + (ptrdiff_t)strlen(path) - 1;

        while (p >= path && *p != ARCHDEP_DIR_SEP_CHR) {
            p--;
        }
        if (p >= path) {
            path = p + 1;    /* skip separator we found */
        }
    }
    return path;
}
