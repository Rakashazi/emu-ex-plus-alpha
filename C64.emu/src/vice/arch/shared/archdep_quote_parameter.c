/** \file   archdep_quote_parameter.c
 * \brief   Add escape sequences to a string
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

#include "lib.h"
#include "util.h"

#include "archdep_quote_parameter.h"


/** \brief  Quote \a name for use as a parameter in exec() etc calls
 *
 * Surounds \a name with double-quotes and replaces brackets with escaped
 * versions on Windows, on Unix it simply returns a heap-allocated copy.
 * Still leaves the OSX unzip bug. (See bug #920)
 *
 * \param[in]   name    string to quote
 *
 * \return  quoted string
 */
char *archdep_quote_parameter(const char *name)
{
    char *a;
    char *c;

    a = util_subst(name, "[", "\\[");

#if defined(WINDOWS_COMPILE)
    c = util_concat("\"", a, "\"", NULL);
    return c;
#else
    c = lib_strdup(a);
#endif
    lib_free(a);
    return c;
}
