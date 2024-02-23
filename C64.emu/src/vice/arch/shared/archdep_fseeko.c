/** \file   archdep_fseeko.c
 * \brief   Provides fseeko(3) replacement on systems that lack fseeko(3)
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
/* For off_t on system where off_t doesn't live in stdio.h or where off_t
 * doesn't exist at all */
#include "types.h"
#include <stdio.h>

#include "archdep_fseeko.h"


/** \brief  Set position in stream
 *
 * Set position in \a stream using a 64-bit signed type.
 *
 * \param[in]   stream  stream
 *
 * \return  0 on success, -1 otherwise
 *
 * \see     fseeko(3)
 */
int archdep_fseeko(FILE *stream, off_t offset, int whence)
{
#ifdef HAVE_FSEEKO
    return fseeko(stream, offset, whence);
#else
    /* Mingw appears to provide fseeko() on Windows, this is for non-Mingw */
# ifdef HAVE__FSEEKI64
    /* this assumes `off_t` matches `long long` on Windows */
    return _fseeki64(stream, offset, whence);
# else
    /* paniek! */
    return fseek(stream, (long)offset, whence);
# endif
#endif
}
