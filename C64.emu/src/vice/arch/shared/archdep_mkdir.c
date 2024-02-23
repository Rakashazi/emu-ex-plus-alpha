/** \file   archdep_mkdir.c
 * \brief   Create a directory
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#if defined(HAIKU_COMPILE) || defined(UNIX_COMPILE)
# include <unistd.h>
# include <sys/stat.h>
# include <sys/types.h>
# if defined(HAIKU_COMPILE)
#  include "archdep_beos.h"
# else
#  include "archdep_unix.h"
# endif
#endif
#ifdef WINDOWS_COMPILE
# include <direct.h>
# include "archdep_win32.h"
#endif

#include "archdep_access.h"
#include "archdep_exit.h"
#include "archdep_stat.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_mkdir.h"


/** \brief  Create a directory \a pathname with \a mode
 *
 * \param[in]   pathname    directory to create
 * \param[in]   mode        access mode of directory (ignored on some systems)
 *
 * \return  0 on success, -1 on error
 */
int archdep_mkdir(const char *pathname, int mode)
{
#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE)
    return mkdir(pathname, (mode_t)mode);
#elif defined(WINDOWS_COMPILE)
    return _mkdir(pathname);
#else
    log_error(LOG_ERR,
              "%s(): not implemented for current system, whoops!\n",
              __func__);
    archdep_vice_exit(1);
#endif
}

#if defined(WINDOWS_COMPILE)
/** \brief  Test pathname for windows drive letter prefix
 *
 * Test if \a pathname starts with "[a-zAZ]:\\".
 *
 * \param[in]   pathname    path to check for dirve letter
 *
 * \return  `true` if \a pathname start with Windows drive letter prefix
 */
static bool has_drive_letter(const char *pathname)
{
    if (pathname != NULL &&
            isalpha((unsigned char)pathname[0])
            && pathname[1] == ':'
            && pathname[2] == '\\') {
        return true;
    }
    return false;
}
#endif


/** \brief  Create directory while creating parents if needed
 *
 * Please note: this function does <b>not</b> fail if \a pathname exists and
 * is a directory. So I might have to change that.
 *
 * \param[in]   pathname    directory to create
 * \param[in]   mode        mode for \a pathname
 *
 * \return  0 on success, -1 on failure
 */
int archdep_mkdir_recursive(const char *pathname, int mode)
{
    char     buffer[ARCHDEP_PATH_MAX];
    char   **parts;         /* pathname split into parts on dir separator */
    size_t   i;
    size_t   bpos;          /* position in buffer during rebuild of pathname */
    int      status = 0;    /* OK */

    if (pathname == NULL || *pathname == '\0') {
        log_error(LOG_ERR,
                  "%s(): pathname is NULL or empty.",
                  __func__);
        return -1;
    }

#if defined(WINDOWS_COMPILE)
    /* replace any forward slashes in windows paths to back slashes */
    for (i = 0; i < sizeof buffer && pathname[i] != '\0'; i++) {
        if (pathname[i] == '/') {
            buffer[i] = '\\';
        } else {
            buffer[i] = pathname[i];
        }
    }
    buffer[i] = '\0';
#else
    strncpy(buffer, pathname, sizeof buffer);
    buffer[sizeof buffer - 1U] = '\0';
#endif
#if 0
    printf("%s(): fixed path = %s\n", __func__, buffer);
#endif
#if defined(UNIX_COMPILE) || defined(HAIKU_COMPILE)
    /* skip leading '/' */
    if (buffer[0] == '/') {
        parts = util_strsplit(buffer + 1, "/", -1);
        /* start with '/' */
        bpos = 1;
    } else {
        parts = util_strsplit(buffer, "/", -1);
        /* start with empty buffer */
        bpos = 0;
    }
#elif defined(WINDOWS_COMPILE)
    /* skip drive letter */
    if (has_drive_letter(buffer)) {
        parts = util_strsplit(buffer + 3, "\\", -1);
        bpos = 3;
    } else if (buffer[0] == '\\') {
        parts = util_strsplit(buffer + 1, "\\", -1);
        bpos = 1;
    } else {
        parts = util_strsplit(buffer, "\\", -1);
        bpos = 0;
    }
#else
    log_error(LOG_ERR, "Unsupported OS: aborting.");
    archdep_exit(1);
#endif
    buffer[bpos] = '\0';

    i = 0;
    while (parts[i] != NULL) {
        size_t len = strlen(parts[i]);

        /* we can use strcpy() here since we know the parts will always be
         * smaller than the buffer and all parts together with the separators
         * will result in the orginal string */
        strcpy(buffer + bpos, parts[i]);
        if (archdep_access(buffer, ARCHDEP_ACCESS_F_OK) != 0) {
            /* doesn't exit, create */
            if (archdep_mkdir(buffer, mode) != 0) {
                log_error(LOG_ERR,
                          "%s(): failed to create directory %s: %d (%s).",
                          __func__, buffer, errno, strerror(errno));
                status = -1;
                goto cleanup;
            }
        } else {
            unsigned int isdir = 0;

            if (archdep_stat(buffer, NULL, &isdir) != 0) {
                /* printf("archdep_stat() failed!\n"); */
                log_error(LOG_ERR,
                          "%s(): archdep_stat(%s) failed: %d (%s).",
                          __func__, buffer, errno, strerror(errno));
                status = -1;
                goto cleanup;
            }
            if (!isdir) {
                /* printf("not a directory!\n"); */
                log_error(LOG_ERR,
                          "%s(): path %s exists but is not a directory.",
                          __func__, buffer);
                status = -1;
                goto cleanup;
            }

            /* path `buffer` exists and is a directory: NOP */
            /* printf("directory: OK, skipping\n"); */
        }

        /* do not append path sep if this was the last part */
        if (parts[i + 1] != NULL) {
            bpos += len;
            /* append path separator */
            buffer[bpos++] = ARCHDEP_DIR_SEP_CHR;
        }
        i++;
    }

cleanup:
    /* clean up */
    for (i = 0; parts[i] != NULL; i++) {
        lib_free(parts[i]);
    }
    lib_free(parts);

    return status;
}
