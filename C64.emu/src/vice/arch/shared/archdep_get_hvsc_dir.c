/** \file   archdep_get_hvsc_dir.c
 * \brief   Get HVSC base directory
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Get High Voltage SID Collection base directory, either from the HVSCRoot
 * resource or the HVSC_BASE environment variable.
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
#include "resources.h"

#include "archdep_get_hvsc_dir.h"


/** \brief  Get HVSC base directory
 *
 * Returns the resource "HVSCRoot" if set, otherwise returns the environment
 * variable "HVSC_BASE" if set.
 *
 * \return  HVSC base dir
 */
const char *archdep_get_hvsc_dir(void)
{
    const char *hvsc = NULL;

    /* resource overrides env var */
    resources_get_string("HVSCRoot", &hvsc);
    if (hvsc == NULL || *hvsc == '\0') {
        /* try env var */
        hvsc = getenv("HVSC_BASE");
    }
    return hvsc;
}
