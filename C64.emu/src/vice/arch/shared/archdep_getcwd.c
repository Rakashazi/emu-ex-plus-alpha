/** \file   archdep_getcwd.c
 * \brief   Get current working directory
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
#include "archdep_defs.h"

#include <stddef.h>
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
# include <unistd.h>
#elif defined(WINDOWS_COMPILE)
# include <direct.h>
#else
# error "Unsupported OS!"
#endif

#include "archdep_getcwd.h"


/** \brief  Get current working directory
 *
 * Store current working directory in \a buf.
 *
 * \param[in]   buf     buffer to store current working directory
 * \param[in]   size    size of \a buf
 *
 * \return  pointer to \a buf on success, `NULL` on failure
 *
 * \see     getcwd(3)
 */
char *archdep_getcwd(char *buf, size_t size)
{
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    return getcwd(buf, size);
#elif defined(WINDOWS_COMPILE)
    return _getcwd(buf, (int)size);
#else
    return NULL;
#endif
}
