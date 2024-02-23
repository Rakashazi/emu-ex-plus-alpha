/** \file   archdep_path_is_relative.c
 * \brief   Determine if a path is relative
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "archdep_exit.h"
#include "log.h"

#include "archdep_path_is_relative.h"


/** \brief  Determine if \a path is a relative path
 *
 * \param[in]   path    pathname
 *
 * \return  non-0 if \a path is relative
 */
int archdep_path_is_relative(const char *path)
{
    if (path == NULL || *path == '\0') {
        return 1;   /* yup, */
    }

#if defined(UNIX_COMPILE) || defined(BEOS_COMPILE)
    return *path != '/';
#elif defined(WINDOWS_COMPILE)
    if (*path == '\\' || *path == '/') {
        return 0;
    }
    if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
            (path[2] == '\\' || path[2] == '/')) {
        return 0;
    }
    return 1;
#else
    log_error(LOG_ERR, "system not supported.");
    archdep_vice_exit(1);
#endif
}
