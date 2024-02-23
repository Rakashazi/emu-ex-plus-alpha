/** \file   archdep_access.c
 * \brief   Test access mode of a file - header
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
 *
 */

#include "vice.h"
#include "archdep_defs.h"

#include <errno.h>
/* Appropriate headers for access(2)/_access() are included in archdep_access.h */

#include "archdep_access.h"


/** \brief  Test access of \a pathname against \a mode
 *
 * \param[in]   pathname    path to file or directory to test
 * \param[in]   mode        access modes bitmask
 *
 * \return  0 on success, -1 on failure
 */
int archdep_access(const char *pathname, int mode)
{
    int access_mode = 0;    /* this is the same as F_OK */

    if (mode & ARCHDEP_ACCESS_R_OK) {
        access_mode |= ARCHDEP_R_OK;
    }
    if (mode & ARCHDEP_ACCESS_W_OK) {
        access_mode |= ARCHDEP_W_OK;
    }
    if (mode & ARCHDEP_ACCESS_X_OK) {
        access_mode |= ARCHDEP_X_OK;
    }

#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    return access(pathname, access_mode);
#elif defined(WINDOWS_COMPILE)
    return _access(pathname, access_mode);
#else
    errno = EINVAL;
    return -1;  /* fail */
#endif
}
