/** \file   archdep_current_dir.c
 * \brief   Get heap-allocated current working directory
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  trikalio
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
#include <errno.h>

#include "archdep_defs.h"
#include "archdep_getcwd.h"
#include "lib.h"

#include "archdep_current_dir.h"


/** \brief  Get heap-allocated current working directory
 *
 * Get the current working directory via archdep_cwd(), reallocating the buffer
 * until the result fits.
 *
 * \return  current working directory or NULL on failure
 *
 * \note    Free result with lib_free()
 */
char *archdep_current_dir(void)
{
    char *p;
    static size_t len = 256;    /* buffer size on first call of function */

    p = lib_malloc(len);
    while (archdep_getcwd(p, len) == NULL) {
        if (errno == ERANGE) {
            /* double buffer size and try again */
            len *= 2;
            p = lib_realloc(p, len);
        } else {
            return NULL;
        }
    }
    return p;
}
