/** \file   archdep_file_size.c
 * \brief   Get size of an open file
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
#if defined(ARCHEP_OS_WINDOWS)
# include <sys/types.h>
#endif

#include "archdep_fseeko.h"
#include "archdep_ftello.h"

#include "archdep_file_size.h"


off_t archdep_file_size(FILE *stream)
{
    off_t pos;
    off_t end;

    pos = archdep_ftello(stream);
    if (pos < 0) {
        return -1;
    }
    if (archdep_fseeko(stream, 0, SEEK_END) != 0) {
        return -1;
    }
    end = archdep_ftello(stream);
    if (archdep_fseeko(stream, pos, SEEK_SET) != 0) {
        return -1;
    }
    return end;
}
