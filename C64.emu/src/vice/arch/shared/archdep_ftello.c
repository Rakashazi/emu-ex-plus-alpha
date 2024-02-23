/** \file   archdep_ftello.c
 * \brief   Provides ftello(3) replacement on systems that lack ftello(3)
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
#include "types.h"
#include <stdio.h>

#include "archdep_ftello.h"


/** \brief  Get position in stream
 *
 * Get position in \a stream using a 64-bit signed type.
 *
 * \param[in]   stream  stream
 *
 * \return  position in \a stream or -1 on error
 *
 * \see     ftello(3)
 */
off_t archdep_ftello(FILE *stream)
{
#ifdef HAVE_FTELLO
    return ftello(stream);
#else
    /* Mingw appears to provide ftello() on Windows, this is for non-Mingw */
# ifdef HAVE__FTELLI64
    /* this assumes `off_t` matches `long long` on Windows */
    return _ftelli64(stream);
# else
    /* paniek! */
    return (off_t)ftell(stream);
# endif
#endif
}
