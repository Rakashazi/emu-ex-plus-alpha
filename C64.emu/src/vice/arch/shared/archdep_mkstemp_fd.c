/** \file   archdep_mkstemp_fd.c
 * \brief   Create temporary file, return file pointer
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * \note    Only tested on Windows and Linux
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
#include "lib.h"
#include "util.h"

#include "archdep_mkstemp_fd.h"


/**
 * \fn      archdep_mkstemp_fd
 * \brief   Create temporary file
 *
 * \param[out]  filename    target of temporary file's name
 * \param[in]   mode        file mode
 *
 * \return  file pointer
 *
 * \note    The filename must be freed with lib_free().
 */

#if defined(BEOS_COMPILE)
/*
 * Looks like this can be used as a fallback for some of the other functions
 */

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
    char *tmp;
    FILE *fd;

    tmp = lib_strdup(tmpnam(NULL));

    fd = fopen(tmp, mode);

    if (fd == NULL) {
        return NULL;
    }

    *filename = tmp;

    return fd;
}


#elif defined(WINDOWS_COMPILE)

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
    char *tmp;
    FILE *fd;

    if (getenv("temp")) {
        tmp = util_concat(getenv("temp"), tmpnam(NULL), NULL);
    } else if (getenv("tmp")) {
        tmp = util_concat(getenv("tmp"), tmpnam(NULL), NULL);
    } else {
        tmp = lib_strdup(tmpnam(NULL));
    }

    fd = fopen(tmp, mode);

    if (fd == NULL) {
        return NULL;
    }

    *filename = tmp;

    return fd;
}

#else

/* Unix */

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
#if defined(HAVE_MKSTEMP)
    char *tmp;
    const char template[] = "/vice.XXXXXX";
    int fildes;
    FILE *fd;
    char *tmpdir;

    tmpdir = getenv("TMPDIR");
    if (tmpdir != NULL) {
        tmp = util_concat(tmpdir, template, NULL);
    } else {
        tmp = util_concat("/tmp", template, NULL);
    }

    fildes = mkstemp(tmp);

    if (fildes < 0) {
        lib_free(tmp);
        return NULL;
    }

    fd = fdopen(fildes, mode);

    if (fd == NULL) {
        lib_free(tmp);
        return NULL;
    }

    *filename = tmp;

    return fd;
#else
    char *tmp;
    FILE *fd;

    tmp = tmpnam(NULL);

    if (tmp == NULL) {
        return NULL;
    }

    fd = fopen(tmp, mode);

    if (fd == NULL) {
        return NULL;
    }

    *filename = lib_strdup(tmp);

    return fd;
#endif
}

#endif
