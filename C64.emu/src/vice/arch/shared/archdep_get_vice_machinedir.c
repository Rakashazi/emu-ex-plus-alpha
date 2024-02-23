/** \file   archdep_get_vice_machinedir.c
 * \brief   Get path to machine dir in VICE datadir
 *
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

#include <stddef.h>

#include "archdep_get_vice_datadir.h"
#include "lib.h"
#include "machine.h"
#include "util.h"

#include "archdep_get_vice_machinedir.h"


/** \brief  Get the absolute path to the VICE datadir for the current machine
 *
 * \return  Path to VICE data/$machine_name dir (free with lib_free())
 */
char *archdep_get_vice_machinedir(void)
{
    char *datadir;
    char *machinedir;

    datadir = archdep_get_vice_datadir();
    machinedir = util_join_paths(datadir, machine_name, NULL);
    lib_free(datadir);
    return machinedir;
}
