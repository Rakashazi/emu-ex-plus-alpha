/** \file   archdep_filename_parameter.c
 * \brief   Quote filename parameters on systems that need it
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "util.h"
#include "archdep_expand_path.h"

#include "archdep_filename_parameter.h"


/** \brief  Quote \a name with double quotes
 *
 * \param[in]   name    string to quote
 *
 * \return  quoted (win32) and heap-allocated copy of \a name
 */
char *archdep_filename_parameter(const char *name)
{
#if defined(WINDOWS_COMPILE)
    char *path;
    char *result;

    archdep_expand_path(&path, name);
    result = util_concat("\"", path, "\"", NULL);
    lib_free(path);
    return result;
#else
    return lib_strdup(name);
#endif
}
