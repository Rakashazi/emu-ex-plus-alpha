/** \file   archdep_xdg.c
 * \brief   XDG base dir specification support
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Freedesktop XDG basedir spec support.
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
#include <stdlib.h>

/* #include "archdep_defs.h" */
#include "archdep_home_path.h"
#include "lib.h"
#include "util.h"

#include "archdep_xdg.h"


/** \brief  Get XDG_CACHE_HOME
 *
 * Either returns the value of $XDG_CACHE_HOME or the default $HOME/.cache
 *
 * \return  heap-allocated string, free with lib_free()
 */
char *archdep_xdg_cache_home(void)
{
    const char *path = getenv("XDG_CACHE_HOME");

    if (path != NULL && *path != '\0') {
        return lib_strdup(path);
    }
    return util_join_paths(archdep_home_path(), ".cache", NULL);
}


/** \brief  Get XDG_CONFIG_HOME
 *
 * Either returns the value of $XDG_CONFIG_HOME or the default $HOME/.config
 *
 * \return  heap-allocated string, free with lib_free()
 */
char *archdep_xdg_config_home(void)
{
    const char *path = getenv("XDG_CONFIG_HOME");

    if (path != NULL && *path != '\0') {
        return lib_strdup(path);
    }
    return util_join_paths(archdep_home_path(), ".config", NULL);
}


/** \brief  Get XDG_DATA_HOME
 *
 * Either returns the value of $XDG_DATA_HOME or the default $HOME/.local/share
 *
 * \return  heap-allocated string, free with lib_free()
 */
char *archdep_xdg_data_home(void)
{
    const char *path = getenv("XDG_DATA_HOME");

    if (path != NULL && *path != '\0') {
        /* got env var, heap-allocate since the util_join_paths() function
         * also returns a heap-allocated string.
         */
        return lib_strdup(path);
    }
    return util_join_paths(archdep_home_path(), ".local", "share", NULL);
}


/** \brief  Get XDG_STATE_HOME
 *
 * Either returns the value of $XDG_STATE_HOME or the default $HOME/.local/state
 *
 * \return  heap-allocated string, free with lib_free()
 */
char *archdep_xdg_state_home(void)
{
    const char *path = getenv("XDG_STATE_HOME");

    if (path != NULL && *path != '\0') {
        return lib_strdup(path);
    }
    return util_join_paths(archdep_home_path(), ".local", "state", NULL);
}
