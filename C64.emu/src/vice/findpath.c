/*
 * findpath.c - Find a file via search path.
 *
 * Written by
 *  Tomi Ollila <Tomi.Ollila@tfi.net>
 *
 * Minor changes for VICE by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "findpath.h"
#include "ioutil.h"
#include "lib.h"


/*
 * This function is checked to be robust with all path 3 types possible
 * (cmd has relative, absolute or no path component)
 * The returned path will always contain at least one '/'. (if not NULL).
 * Overflow testing for internal buffer is always done.
 */

char *findpath(const char *cmd, const char *syspath, int mode)
{
    char *pd = NULL;
    char *c, *buf;
    size_t maxpathlen;

    maxpathlen = (size_t)ioutil_maxpathlen();

    buf = lib_malloc(maxpathlen);

    buf[0] = '\0'; /* this will (and needs to) stay '\0' */

    if (strchr(cmd, FSDEV_DIR_SEP_CHR)) {
        size_t l;
        int state;
        const char *ps;

        if (archdep_path_is_relative(cmd)) {
            if (ioutil_getcwd(buf + 1, (int)maxpathlen - 128) == NULL) {
                goto fail;
            }

            l = strlen(buf + 1);
        } else {
            l = 0;
        }

        if (l + strlen(cmd) >= maxpathlen - 5) {
            goto fail;
        }

        ps = cmd;
        pd = buf + l; /* buf + 1 + l - 1 */

#if (FSDEV_DIR_SEP_CHR == '/')
        if (*pd++ != '/') {
            *pd++ = '/';
        }
#else
        pd++;
#endif

        state = 1;

        /* delete extra `/./', '/../' and '//':s from the path */
        while (*ps) {
            switch (state) {
                case 0:
                    if (*ps == '/') {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    break;
                case 1:
                    if (*ps == '.') {
                        state = 2;
                        break;
                    }
                    if (*ps == '/') {
                        pd--;
                    } else {
                        state = 0;
                    }
                    break;
                case 2:
                    if (*ps == '/') {
                        state = 1;
                        pd -= 2;
                        break;
                    }
                    if (*ps == '.') {
                        state = 3;
                    } else {
                        state = 0;
                    }
                    break;
                case 3:
                    if (*ps != '/') {
                        state = 0;
                        break;
                    }
                    state = 1;
                    pd -= 4;
                    while (*pd != '/' && *pd != '\0') {
                        pd--;
                    }
                    if (*pd == '\0') {
                        pd++;
                    }
                    state = 1;
                    break;
            }
            *pd++ = *ps++;
        }

        *pd = '\0';
        pd = buf + 1;
    } else {
        const char *path = syspath;
        const char *s;
        size_t cl = strlen(cmd) + 1;

        for (s = path; s; path = s + 1) {
            char * p;
            int l;

            s = strchr(path, ARCHDEP_FINDPATH_SEPARATOR_CHAR);
            l = s ? (int)(s - path) : (int)strlen(path);

            if (l + cl > maxpathlen - 5) {
                continue;
            }

            memcpy(buf + 1, path, l);

            p = buf + l;  /* buf + 1 + l - 1 */

            if (*p++ != '/') {
                *p++ = '/';
            }

            memcpy(p, cmd, cl);

            for (c = buf + 1; *c != '\0'; c++) {
#if (FSDEV_DIR_SEP_CHR == '\\')
                if (*c == '/') {
                    *c = '\\';
                }
#else
#if (FSDEV_DIR_SEP_CHR == '/')
                if (*c == '\\') {
                    *c = '/';
                }
#else
#error directory seperator for this platform not handled correctly, FIX NEEDED!
#endif
#endif
            }
            if (ioutil_access(buf + 1, mode) == 0) {
                pd = p /* + cl*/;
                break;
            }
        }
    }


    if (pd) {
        char *tmpbuf;
#if 0
        do {
            pd--;
        } while (*pd != '/'); /* there is at least one '/' */

        if (*(pd - 1) == '\0') {
            pd++;
        }
        *pd = '\0';
#endif

        tmpbuf = lib_stralloc(buf + 1);
        lib_free(buf);
        return tmpbuf;
    }
fail:
    lib_free(buf);
    return NULL;
}
