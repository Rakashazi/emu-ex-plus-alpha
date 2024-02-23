/** \file   archdep_list_drives.c
 * \brief   Get a list of available Windows drives
 * \author  (Unknown)
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
 *
 */

#include "vice.h"
#include "archdep_defs.h"
#include "archdep.h"
#include "lib.h"

#include "archdep_list_drives.h"

/** \fn     archdep_list_drives
 * \brief   Get a list of available Windows drives
 *
 * \return  list of strings, NULL-terminated
 *
 * \note    the list and its elements need to be freed with lib_free().
 */

#if defined(WINDOWS_COMPILE)

/* FIXME: is this needed* */
# ifdef SDL_CHOOSE_DRIVES

#  include <windows.h>

char **archdep_list_drives(void)
{
    uint32_t bits, mask;
    int drive_count = 1, i = 0;
    char **result, **p;

    bits = GetLogicalDrives();
    mask = 1;
    while (mask != 0) {
        if (bits & mask) {
            ++drive_count;
        }
        mask <<= 1;
    }
    result = lib_malloc(sizeof(char*) * drive_count);
    p = result;
    mask = 1;
    while (mask != 0) {
        if (bits & mask) {
            char buf[16];
            sprintf(buf, "%c:/", 'a' + i);
            *p++ = lib_strdup(buf);
        }
        mask <<= 1;
        ++i;
    }
    *p = NULL;

    return result;
}
# endif /* SDL_CHOOSE_DRIVES */

#endif  /* WINDOWS_COMPILE */
