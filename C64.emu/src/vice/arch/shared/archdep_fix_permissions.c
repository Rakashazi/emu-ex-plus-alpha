/** \file   archdep_fix_permissions.c
 * \brief   Update permissions of a file to R+W
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE)
# include <sys/stat.h>
# include <sys/types.h>
#endif
#ifdef WINDOWS_COMPILE
# include <io.h>
# include <windows.h>
#endif

#include "archdep_fix_permissions.h"


/** \brief  Update permissions of \a name to R+W
 *
 * \param[in]   name    pathname
 *
 * \return  non-0 on success
 *
 * \note    does nothing on system other than Unix or Windows
 */
int archdep_fix_permissions(const char *name)
{
#ifdef WINDOWS_COMPILE
    return _chmod(name, _S_IREAD|_S_IWRITE);
#elif defined(UNIX_COMPILE)
    mode_t mask = umask(0);
    umask(mask);
    return chmod(name, mask ^ 0666); /* this is really octal here! */
#elif defined(BEOS_COMPILE)
    /* there's got to be some beos-ish stuff to change permissions, at least
     * with Haiku */
    return 0;
#endif
    /* OS/2 etc */
    return 0;
}
