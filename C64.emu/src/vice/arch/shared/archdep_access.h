/** \file   archdep_access.h
 * \brief   Test access mode of a path - header
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

#ifndef VICE_ARCHDEP_ACCESS_H
#define VICE_ARCHDEP_ACCESS_H

#include "vice.h"
#include "archdep_defs.h"

#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)

/* covers linux, bsd, macos and haiku */
# include <unistd.h>
# define ARCHDEP_R_OK   R_OK
# define ARCHDEP_W_OK   W_OK
# define ARCHDEP_X_OK   X_OK
# define ARCHDEP_F_OK   F_OK

#elif defined(WINDOWS_COMPILE)

/* shitty windows */
# include <io.h>
# define ARCHDEP_R_OK   4
# define ARCHDEP_W_OK   2
# define ARCHDEP_X_OK   1
# define ARCHDEP_F_OK   0

#else
# error "Unsupported OS!"
#endif


/** \brief  File is readable */
#define ARCHDEP_ACCESS_R_OK 4

/** \brief  File is writeable */
#define ARCHDEP_ACCESS_W_OK 2

/** \brief  File is executable */
#define ARCHDEP_ACCESS_X_OK 1

/** \brief  File exists */
#define ARCHDEP_ACCESS_F_OK 0


int archdep_access(const char *pathname, int mode);

#endif
