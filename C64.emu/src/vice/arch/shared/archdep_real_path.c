/** \file   archdep_real_path.c
 * \brief   Normalize path names
 * \author  Michael C. Martin <mcmartin@gmail.com>
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

#ifndef WINDOWS_COMPILE
#include <limits.h>
#else
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "archdep_real_path.h"

/** \brief  Resolve \a pathname to its canonicalized absolute form
 *
 * \param[in]   pathname             pathname to normalize. This file must
 *                                   exist.
 * \param[out]  resolved_pathname    buffer for result (must be at least
 *                                   PATH_MAX/_MAX_PATH long)
 *
 * \return      resolved_pathname on success, NULL on failure. On failure,
 *              contents of resolved_pathname are undefined.
 */
char *archdep_real_path(const char *pathname, char *resolved_pathname)
{
#ifdef WINDOWS_COMPILE
    DWORD size = GetFullPathNameA(pathname, _MAX_PATH, resolved_pathname, NULL);
    if (size == 0 || size >= _MAX_PATH) {
        return NULL;
    }
    size = GetShortPathNameA(resolved_pathname, resolved_pathname, _MAX_PATH);
    if (size == 0 || size >= _MAX_PATH) {
        return NULL;
    }
    size = GetLongPathNameA(resolved_pathname, resolved_pathname, _MAX_PATH);
    if (size == 0 || size >= _MAX_PATH) {
        return NULL;
    }
    return resolved_pathname;
#else
    return realpath(pathname, resolved_pathname);
#endif
}

/** \brief  Compare \a path1 to \a path2 in their canonicalized forms
 *
 * \param[in]  path1   The first path to compare
 * \param[in]  path2   The second path to compare
 *
 * \return     nonzero if path1 and path2 both refer to the same canonicalized
 *             path; zero otherwise. If path1 or path2 do not actually exist
 *             the results are unspecified.
 */
int archdep_real_path_equal(const char *path1, const char *path2)
{
#ifdef WINDOWS_COMPILE
    char path1_norm[_MAX_PATH], path2_norm[_MAX_PATH];
#else
    char path1_norm[PATH_MAX], path2_norm[PATH_MAX];
#endif
    if (!archdep_real_path(path1, path1_norm)) {
        return 0;
    }
    if (!archdep_real_path(path2, path2_norm)) {
        return 0;
    }
    return !strcmp(path1_norm, path2_norm);
}
