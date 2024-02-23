/** \file   archdep_default_portable_resource_file_name.c
 * \brief   Retrieve default portable resource file path
 * \author  groepaz <groepaz@gmx.de>
 *
 * Get path to default portable resource file (vicerc/vice.ini)
 *
 * Unlike the normal resource file, this one is located in the 'root' directory
 * of the bindist (Windows) or in the user's home directory (Unix).
 *
 * OS support:
 *  - Windows
 *  - Unix
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

#include <stdlib.h>

#include "archdep_boot_path.h"
#include "archdep_home_path.h"
#include "util.h"

#include "archdep_default_portable_resource_file_name.h"


/** \brief  Get path to default portable resource file
 *
 * \return  heap-allocated path, free with lib_free()
 */
char *archdep_default_portable_resource_file_name(void)
{
#ifdef WINDOWS_COMPILE
    return util_join_paths(archdep_boot_path(),
# ifdef USE_GTK3UI
                           "..", /* Gtk-Win binaries live in bin/, so go up */
# endif
                           ARCHDEP_VICERC_NAME,
                           NULL);
#else
    return util_join_paths(archdep_home_path(),
                           "." ARCHDEP_VICERC_NAME, /* ".vicerc" */
                           NULL);
#endif
}
