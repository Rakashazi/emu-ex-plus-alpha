/** \file   archdep_is_haiku.c
 * \brief   Determine if BeOS is "Haiku"
 *
 * \author  groepaz <groepaz@gmx.net>
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

#include "archdep_is_haiku.h"

/** \fn archdep_is_haiku
 * \brief   Determine if we're running on Haiku
 *
 * This check is needed for Haiku, since it always returns 1 on
 * SupportsWindowMode().
 *
 * \return  0 if running Haiku, -1 otherwise (BeOS)
 *
 * FIXME:   Would make more sense to return a boolean value, since the function
 *          name contains *is*.
 */

#ifdef BEOS_COMPILE
#include <sys/utsname.h>
#include "util.h"

int archdep_is_haiku(void)
{
    struct utsname name;

    uname(&name);
    if (!util_strncasecmp(name.sysname, "Haiku", 5)) {
        return -1;
    }
    return 0;
}

#else

int archdep_is_haiku(void)
{
    return -1;
}
#endif
