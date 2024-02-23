/** \file   archdep_home_path.c
 * \brief   Retrieve home directory of current user
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Retrieve the home directory of the current user on systems that have a
 * concept of a home directory. On systems that don't have a home dir '.' is
 * returned (PROGDIR: in the case of AmigaOS).
 * Of course on systems that don't have a home dir, this function simply
 * shouldn't be used, archdep_boot_path() might be a better function.
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - BeOS/Haiku (always returns '/boot/home')
 *
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

#include "lib.h"
#include "log.h"

#ifdef UNIX_COMPILE
# include <unistd.h>
# include <sys/types.h>
# include <pwd.h>
#endif

#ifdef WINDOWS_COMPILE
# include <windows.h>
# include <shlobj.h>
#endif

#include "archdep_defs.h"

#include "archdep_home_path.h"


/** \brief  home directory reference
 *
 * Allocated once in the first call to archdep_home_path(), should be freed
 * on emulator exit with archdep_home_path_free()
 */
static char *home_dir = NULL;


/** \brief  Get user's home directory
 *
 * \return  user's home directory
 *
 * \note    Free memory used on emulator exit with archdep_home_path_free().
 */
const char *archdep_home_path(void)
{
    /* stupid vice code rules, only declare vars at the top */
#ifdef UNIX_COMPILE
    char *home;
#elif defined(WINDOWS_COMPILE)
    DWORD err;
    char home[MAX_PATH];
#endif

    if (home_dir != NULL) {
        return home_dir;
    }

#ifdef UNIX_COMPILE
    home = getenv("HOME");
    if (home == NULL) {
        struct passwd *pwd;

        pwd = getpwuid(getuid());
        if (pwd == NULL) {
            home = ".";
        } else {
            home = pwd->pw_dir;
        }
    }
    home_dir = lib_strdup(home);
#elif defined(WINDOWS_COMPILE)
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, home))) {
        /* error */
        err = GetLastError();
        printf("failed to get user profile root directory: 0x%lx.\n", err);
        /* set home dir to "." */
        home[0] = '.';
        home[1] = '\0';
    }
    home_dir = lib_strdup(home);
#elif defined(BEOS_COMPILE)
    /* Beos/Haiku is single-user */
    home_dir = lib_strdup("/boot/home");
#else
    /* all others: */
    home_dir = lib_strdup(".");
#endif
    return home_dir;
}


/** \brief  Free memory used by the home path
 */
void archdep_home_path_free(void)
{
    if (home_dir != NULL) {
        lib_free(home_dir);
        home_dir = NULL;
    }
}

