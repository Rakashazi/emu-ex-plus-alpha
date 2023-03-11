/** \file   archdep_mkstemp_fd.h
 * \brief   Create temporary file, return file pointer - header
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_ARCHDEP_MKSTEMP_FD_H
#define VICE_ARCHDEP_MKSTEMP_FD_H

#include <stdio.h>

/**
 * \brief   Create temporary file
 *
 * Create a temporary file with a random name.
 *
 * \param[out]  filename    target of temporary file's name
 * \param[in]   mode        file mode
 *
 * \return  file pointer
 *
 * \note    The filename must be freed with lib_free().
 *
 * FIXME:   For some reason Doxygen doesn't accept this docblock in the .c file
 *          with '\\fn archdep_mkstemp_fd'. So I had to put it here.
 */
FILE *archdep_mkstemp_fd(char **filename, const char *mode);

#endif
