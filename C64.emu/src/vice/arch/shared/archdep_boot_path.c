/** \file   archdep_boot_path.c
 * \brief   Retrieve dirname of currently running binary
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Get path to running executable, stripping executable name and extension.
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - BeOS/Haiku (untested)
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "archdep_program_path.h"

#include "archdep_boot_path.h"


/** \brief  Boot path
 *
 * Heap allocated on the first call, must be free when exiting the program
 * with lib_free().
 *
 * \see archdep_boot_path_free()
 */
static char *boot_path = NULL;


/** \brief  Get dirname of currently running binary
 *
 * Retrieve the path to the running binary, excluding binary name.
 *
 * \note    The path is allocated internally by VICE on the first call and
 *          should be freed on emulator shutdown with archdep_boot_path_free().
 *
 * \return  directory of running binary
 */
const char *archdep_boot_path(void)
{
    const char *full_path;
    char *p;

    if (boot_path != NULL) {
        /* already determined boot path, return */
        return boot_path;
    }

    full_path = archdep_program_path();

    p = strrchr(full_path, ARCHDEP_DIR_SEP_CHR);
    if (p == NULL) {
        /* didn't find a '/' or '\' anywhere */
        boot_path = lib_strdup(".");
    } else {
        boot_path = lib_malloc((size_t)(p - full_path + 1));
        memcpy(boot_path, full_path, (size_t)(p - full_path));
        boot_path[p - full_path] = '\0';
    }
    return boot_path;
}


/** \brief  Free memory used by boot path
 *
 * This must be called from the emulator shutdown code.
 */
void archdep_boot_path_free(void)
{
    if (boot_path != NULL) {
        lib_free(boot_path);
        boot_path = NULL;
    }
}
