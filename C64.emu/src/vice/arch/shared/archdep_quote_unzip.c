/** \file   archdep_quote_unzip.c
 * \brief   Add escape sequences to a filename for use with unzip
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

#include "archdep_quote_unzip.h"


/** \brief  Quote \a name for use as a parameter in exec() etc calls with unzip
 *
 * Surounds \a name with double-quotes and replaces brackets with escaped versions.
 *
 * \param[in]   name    string to quote
 *
 * \return  quoted string
 */
char *archdep_quote_unzip(const char *name)
{
#ifdef WINDOWS_COMPILE
    char *a;
    char *b;

    a = util_subst(name, "[", "[[]");
    b = util_concat("\"", a, "\"", NULL);
    lib_free(a);
    return b;
#else
    return util_subst(name, "[", "\\[");
#endif
}
