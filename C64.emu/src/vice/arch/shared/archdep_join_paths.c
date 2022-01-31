/** \file   archdep_join_paths.c
 * \brief   Concatenate multiple string into a single path
 *
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "lib.h"

#include "archdep_join_paths.h"


/** \brief  Join multiple paths into a single path
 *
 * Joins a list of strings into a path for use with the current arch
 *
 * \param   [in]    path    list of paths to join, NULL-terminated
 *
 * \return  heap-allocated string, free with lib_free()
 */
char *archdep_join_paths(const char *path, ...)
{
    const char *arg;
    char *result;
    char *endptr;
    size_t result_len;
    size_t len;
    va_list ap;
#if 0
    printf("%s: first argument: '%s'\n", __func__, path);
#endif
    /* silly way to use a varags function, but lets catch it anyway */
    if (path == NULL) {
        return NULL;
    }

    /* determine size of result string */
    va_start(ap, path);
    result_len = strlen(path);
    while ((arg = va_arg(ap, const char *)) != NULL) {
        result_len += (strlen(arg) + 1);
    }
    va_end(ap);
#if 0
    /* cannot use %zu here due to MS' garbage C lib */
    printf("%s: result length: %"PRI_SIZE_T"\n", __func__, result_len);
#endif
    /* initialize result string */
    result = lib_calloc(result_len + 1, 1);
    strcpy(result, path);
    endptr = result + (ptrdiff_t)strlen(path);

    /* now concatenate arguments into a pathname */
    va_start(ap, path);
    while ((arg = va_arg(ap, const char *)) != NULL) {
#if 0
        printf("%s: adding '%s' to the result.", __func__, arg);
#endif
        len = strlen(arg);
        if (*arg != ARCHDEP_DIR_SEPARATOR) {
            *endptr++ = ARCHDEP_DIR_SEPARATOR;
        }
        memcpy(endptr, arg, len + 1);
        endptr += (ptrdiff_t)len;
    }

    va_end(ap);
    return result;
}
