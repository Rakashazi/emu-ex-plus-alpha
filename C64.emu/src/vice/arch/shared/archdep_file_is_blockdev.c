/** \file   archdep_file_is_blockdev.c
 * \brief   Determine if a pathname refers to a block device
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

#ifdef UNIX_COMPILE
# include <sys/stat.h>
# include <sys/types.h>
#endif

#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_file_is_blockdev.h"


/** \brief  Determine if \a name is a block device
 *
 * \param[in]   name    pathname
 *
 * \return  non-0 if \a name is a block device
 *
 * \note    returns 0 on non-Unix
 */
int archdep_file_is_blockdev(const char *name)
{
#ifdef UNIX_COMPILE
    struct stat buf;

    if (stat(name, &buf) != 0) {
        return 0;
    }
    if (S_ISBLK(buf.st_mode)) {
        return 1;
    }
#endif
    return 0;
}
