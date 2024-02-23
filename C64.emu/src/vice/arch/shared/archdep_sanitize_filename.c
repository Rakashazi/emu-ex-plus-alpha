/** \file   archdep_sanitize_filename.c
 * \brief   Sanitize a filename for writing to the host OS
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Replace all occurences in a filename that would result in illegal tokens
 * in the host OS' file system with underscores.
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - BeOS/Haiku (untested)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"

#include "archdep_sanitize_filename.h"



/** \brief  Tokens that are illegal in a path/filename
 */
#if defined(UNIX_COMPILE) || defined(BEOS_COMPILE)
static const char illegal_name_tokens[] = "/";
#elif defined(WINDOWS_COMPILE)
static const char illegal_name_tokens[] = "/\\?*:|\"<>";
#else
static const char illegal_name_tokens[] = "";
#endif



/** \brief  Sanitize \a name by removing invalid characters for the current OS
 *
 * Replace all illegal tokens in-place with underscores.
 *
 * \param[in,out]   name    0-terminated string
 */
void archdep_sanitize_filename(char *name)
{
    while (*name != '\0') {
        int i = 0;
        while (illegal_name_tokens[i] != '\0') {
            if (illegal_name_tokens[i] == *name) {
                *name = '_';
                break;
            }
            i++;
        }
        name++;
    }
}
