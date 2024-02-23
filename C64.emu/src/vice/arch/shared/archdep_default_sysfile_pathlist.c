/** \file   archdep_default_sysfile_pathlist.c
 * \brief   Get a list of paths of required data files
 *
 * A note about Gtk3 on Windows:
 *
 * For some reason Gtk3/GLib checks the path of a running binary to see if it's
 * running from a bin/ dir, so it can load stuff from bin/..
 *
 * Unfortunately this also means running a binary from, say 'C:/bin/foo/bar/vice'
 * will try to load DLL's and other data from C:/bin/{lib,share} or so, which in
 * my opinion is a seriouse bug.
 *
 * So we had to change the bindist script for Windows, and alter a few archdep
 * functions to support this weirdness.
 *
 * -- compyx, 2020-06-28
 *    (perhaps move this note/rant somewhere more public?)
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

#include <stddef.h>

#include "archdep_boot_path.h"
#include "archdep_get_vice_datadir.h"
#include "archdep_xdg.h"
#include "archdep_user_config_path.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_default_sysfile_pathlist.h"


/** \brief  Total number of pathnames to store in the pathlist
 *
 * 16 seems to be enough, but it can always be increased to support more.
 */
#define TOTAL_PATHS 16


/** \brief  Reference to the sysfile pathlist
 *
 * This keeps a copy of the generated sysfile pathlist so we don't have to
 * generate it each time it is needed.
 */
static char *sysfile_path = NULL;


/** \brief  Generate a list of search paths for VICE system files
 *
 * \param[in]   emu_id  emulator ID (ie 'C64 or 'VSID')
 *
 * \return  heap-allocated string, to be freed by the caller
 */
char *archdep_default_sysfile_pathlist(const char *emu_id)
{
    const char *boot_path = NULL;
    char *datadir = NULL;
#if !defined(WINDOWS_COMPILE) && !defined(BEOS_COMPILE)
    char *home_path = NULL;
# ifdef UNIX_COMPILE
    char *xdg_data = NULL;
# endif
#endif

    const char *paths[TOTAL_PATHS + 1];
    int i;


    if (sysfile_path != NULL) {
        /* sysfile.c appears to free() this */
        return lib_strdup(sysfile_path);
    }

    boot_path = archdep_boot_path();
    datadir = archdep_get_vice_datadir();
#if !defined(WINDOWS_COMPILE) && !defined(BEOS_COMPILE)

# ifdef UNIX_COMPILE
    xdg_data = archdep_xdg_data_home();
    home_path = util_join_paths(xdg_data, "vice", NULL);
    lib_free(xdg_data);
# else
    home_path = archdep_user_config_path();
# endif
#endif

    /* zero out the array of paths to join later */
    for (i = 0; i <= TOTAL_PATHS; i++) {
        paths[i] = NULL;
    }

    /* now join everything together */
    i = 0;

    /* home paths */
#if !defined(WINDOWS_COMPILE) && !defined(BEOS_COMPILE)
    if (home_path != NULL) {
        paths[i++] = home_path;
    }
#endif

    /* boot paths */
    if (boot_path != NULL) {
        paths[i++] = boot_path;
    }

    /* VICE_DATADIR paths */
    if (datadir != NULL) {
        paths[i++] = datadir;
    }

    /* terminate list */
    paths[i] = NULL;
    sysfile_path = util_strjoin(paths, ARCHDEP_FINDPATH_SEPARATOR_STRING);

    /* cleanup */
    if (datadir != NULL) {
        lib_free(datadir);
    }

#if !defined(WINDOWS_COMPILE) && !defined(BEOS_COMPILE)
    if (home_path != NULL) {
        lib_free(home_path);
    }
#endif

#if 0
    log_message(LOG_DEFAULT, "Search path = %s", sysfile_path);
    printf("%s(): paths = '%s'\n", __func__, sysfile_path);
#endif
    /* sysfile.c appears to free() this (ie TODO: fix sysfile.c) */
    return lib_strdup(sysfile_path);
}


/** \brief  Free the internal copy of the sysfile pathlist
 *
 * Call on emulator exit
 */
void archdep_default_sysfile_pathlist_free(void)
{
    if (sysfile_path != NULL) {
        lib_free(sysfile_path);
        sysfile_path = NULL;
    }
}
