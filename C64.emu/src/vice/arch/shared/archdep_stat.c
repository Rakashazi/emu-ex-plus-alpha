/** \file   archdep_stat.c
 * \brief   Simplified stat(2) call
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Determine if file exists, its length and if its a directory.
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

#include "lib.h"
#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "archdep_stat.h"


/** \brief  Determine the size of \a path and whether it's a directory
 *
 * \param[in]   path    pathname
 * \param[out]  len     length of file \a path (pass `NULL` to ignore)
 * \param[out]  isdir   \a path is a directory (pass `NULL` to ignore)
 *
 * \return  0 on success, -1 on failure
 */
int archdep_stat(const char *path, size_t *len, unsigned int *isdir)
{
    struct stat statbuf;

    if (stat(path, &statbuf) < 0) {
        if (len != NULL) {
            *len = -1;
        }
        if (isdir != NULL) {
            *isdir = 0;
        }
        return -1;
    }
    if (len != NULL) {
        *len = statbuf.st_size;
    }
    if (isdir != NULL) {
        *isdir = S_ISDIR(statbuf.st_mode);
    }
    return 0;
}
