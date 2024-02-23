/** \file   archdep_tmpnam.c
 * \brief   Generate a unique, temporary filename
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

/* TODO: include for BeOS (not Haiku) tmpnam() */

/* Seems like tmpnam() is available in stdio.h for Linux, BSD, Windows and
 * perhaps others
 */
#include <stdio.h>
#include <string.h>
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
# include <unistd.h>
#endif
#ifdef WINDOWS_COMPILE
# include <windows.h>
#endif

#include "archdep_exit.h"
#include "archdep_boot_path.h"
#include "lib.h"
#include "log.h"

#include "archdep_tmpnam.h"


char *archdep_tmpnam(void)
{
#if defined(BEOS_COMPILE) && !defined(HAIKU_COMPILE)
    return lib_strdup(tmpnam(NULL));
#elif defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    /*
     * Linux manpage for tmpnam(3) says to never use it, FreeBSD indicates the
     * same.
     */
# ifdef HAVE_MKSTEMP
    char *tmp_name;
    const char mkstemp_template[] = "/vice.XXXXXX";
    int fd;
    char *tmp;

    tmp_name = lib_malloc(ARCHDEP_PATH_MAX + 1U);

    tmp = getenv("TMPDIR");
    if (tmp != NULL) {
        strncpy(tmp_name, tmp, ARCHDEP_PATH_MAX);
        tmp_name[ARCHDEP_PATH_MAX - sizeof(mkstemp_template)] = '\0';
    } else {
        /* fall back to /tmp */
        strcpy(tmp_name, "/tmp");
    }
    strcat(tmp_name, mkstemp_template);
    fd = mkstemp(tmp_name);
    if (fd < 0) {
        tmp_name[0] = '\0';
    } else {
        close(fd);
    }

    return tmp_name;
# else
    return lib_strdup(tmpnam(NULL));
# endif
#elif defined(WINDOWS_COMPILE)
    /*
     * This blows and should probably be replaced with GetTempFileNameA() or
     * something similar
     */
    char *temp_path;
    char *temp_name;

    temp_path = lib_malloc(ARCHDEP_PATH_MAX + 1U);
    temp_name = lib_malloc(ARCHDEP_PATH_MAX + 1U);

    if (GetTempPath(ARCHDEP_PATH_MAX, temp_path) == 0) {
        log_error(LOG_ERR, "failed to get Windows temp dir.");
        lib_free(temp_path);
        lib_free(temp_name);
        archdep_vice_exit(1);
    }


    if (GetTempFileName(temp_path, "vice", 0, temp_name) == 0) {
        log_error(LOG_ERR, "failed to construct a Windows temp file.");
        lib_free(temp_path);
        lib_free(temp_name);
        archdep_vice_exit(1);
    }

    lib_free(temp_path);
    return temp_name;
#endif
}
