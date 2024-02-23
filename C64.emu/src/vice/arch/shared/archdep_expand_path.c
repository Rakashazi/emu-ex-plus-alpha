/** \file   archdep_expand_path.c
 * \brief   Expand a path into an absolute path
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

#include "archdep_getcwd.h"
#include "archdep_home_path.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_expand_path.h"


/** \brief  Generate heap-allocated full pathname of \a orig_name
 *
 * Returns the absolute path of \a orig_name.
 *
 * Expands '~' to the user's home path on Unix.
 * If the prefix in \a orig_name is not '\' and not '/' and not '~/' (Unix)
 * the file is assumed to reside in the current working directory, whatever
 * that may be.
 *
 * \param[out]  return_path pointer to expanded path destination
 * \param[in]   orig_name   original path
 *
 * \return  0
 */
int archdep_expand_path(char **return_path, const char *orig_name)
{
#ifdef UNIX_COMPILE
    if (*orig_name == '/') {
        *return_path = lib_strdup(orig_name);
    } else if ((orig_name[0] == '~') && (orig_name[1] == '/')) {
        *return_path = util_concat(archdep_home_path(), orig_name + 1, NULL);
    } else {
        char buffer[ARCHDEP_PATH_MAX];

        if (archdep_getcwd(buffer, sizeof(buffer)) == NULL) {
            *return_path = NULL;
            return -1;
        }
        *return_path = util_concat(buffer, "/", orig_name, NULL);
    }
    return 0;

#elif defined(WINDOWS_COMPILE)
    /* taken from the old WinVICE port (src/arch/win32/archdep.c): */
    *return_path = lib_strdup(orig_name);
#elif defined(BEOS_COMPILE)
    /* taken from src/arch/sdl/archdep_beos.c: */

    /* XXX: Haiku uses a Unix-like approach, so we could use the Unix codepath
     *      for it. But since non of the VICE devs use BeOS/Haiku, there's
     *      little point in doing so.
     */
    *return_path = lib_strdup(orig_name);
#else
    /* fallback */
    log_error(LOG_ERR, "unsupported OS: just returning input.");
    *return_path = lib_strdup(orig_name);
#endif
    return 0;
}
