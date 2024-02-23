/** \file   archdep_fdopen.c
 * \brief   Associate stream with file descriptor
 *
 * Wrapper for the POSIX fdopen(2) function.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - BSD
 *  - MacOS
 *  - Haiku
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

#include <stdio.h>

#if !defined(UNIX_COMPILE) && !defined(HAIKU_COMPILE) && !defined(WINDOWS_COMPILE)
# error "Unsupported OS!"
#endif

#include "archdep_fdopen.h"


/** \brief  Close file descriptor
 *
 * \param[in]   fd      file descriptor
 * \param[in]   mode    access mode
 *
 * \return  FILE pointer on success, NULL on failure
 */
FILE *archdep_fdopen(int fd, const char *mode)
{
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    return fdopen(fd, mode);
#elif defined(WINDOWS_COMPILE)
    return _fdopen(fd, mode);
#else
    return NULL;
#endif
}
