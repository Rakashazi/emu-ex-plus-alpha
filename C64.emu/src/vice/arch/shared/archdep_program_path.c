/** \file   archdep_program_path.c
 * \brief   Retrieve path of currently running binary
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Get path to running executable.
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - Haiku
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
/* for readlink(2) */
/* FIXME: This works for Haiku but might not for classic BeOS! */
#if defined(UNIX_COMPILE) || defined(BEOS_COMPILE)
# include <unistd.h>
# if defined(FREEBSD_COMPILE) || defined(DRAGONFLYBSD_COMPILE)
#  include <sys/sysctl.h>
# endif
# ifdef MACOS_COMPILE
#  include <libproc.h>
# endif
#endif
/* for GetModuleFileName() */
#ifdef WINDOWS_COMPILE
# include <windows.h>
# include <direct.h>
#endif

#include "archdep_exit.h"
#include "archdep_getcwd.h"
#include "archdep_path_is_relative.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_program_path.h"


/** \brief  Reference to program path string
 *
 * Should be freed on emulator exit with archdep_program_path_free()
 */
static char *program_path = NULL;


/** \brief  Reference to argv[0]
 *
 * Do not free this, the C standard guarantees argv is available during a
 * program's lifetime, so this will work.
 */
static char *argv0_ref = NULL;


/** \brief  Buffer used to retrieve pathnames
 *
 * Various OS calls use this buffer to store the path to the running binary, if
 * such a call exists.
 */
static char buffer[ARCHDEP_PATH_MAX];


/** \brief  Set reference to argv[0]
 *
 * \param[in]    argv0   argv[0]
 */
void archdep_program_path_set_argv0(char *argv0)
{
    argv0_ref = argv0;
}


/** \brief  Fall back: try to get absolute path to exec via argv[0]
 *
 * This is unreliable and should only be used as a last resort.
 *
 * \return  bool (if this fails, we have to give up)
 */
static bool argv_fallback(void)
{
    char cwd_buf[ARCHDEP_PATH_MAX];
    char *result;
    size_t res_len;

    if (argv0_ref == NULL) {
        log_error(LOG_ERR, "argv[0] is NULL, giving up.");
        return false;
    }
    if (*argv0_ref == '\0') {
        log_error(LOG_ERR, "argv[0] is empty, giving up.");
        return false;
    }

    /* do we have an absolute path in argv[0]? */
    if (!archdep_path_is_relative(argv0_ref)) {
        strncpy(buffer, argv0_ref, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';
        return true;
    }

    /*
     * Relative path in argv[0], try to get cwd and join it with argv[0]
     */
    if (archdep_getcwd(cwd_buf, sizeof(cwd_buf)) == NULL) {
        log_error(LOG_ERR, "failed to get cwd, giving up.");
        return false;
    }

    result = util_join_paths(cwd_buf, argv0_ref, NULL);
    res_len = strlen(result);
    if (res_len >= sizeof(buffer)) {
        /* insufficient space */
        log_error(LOG_ERR, "insufficient space for path, giving up.");
        lib_free(result);
        return false;
    }
    memcpy(buffer, result, res_len + 1);
    lib_free(result);
    return true;
}



/** \brief  Get absolute path to the running executable
 *
 * Free with archdep_program_path_free() on emulator exit.
 *
 * \return  absolute path to running executable
 */
const char *archdep_program_path(void)
{
#if defined(WINDOWS_COMPILE)
    DWORD result;
#endif
    if (program_path != NULL) {
        /* already got it, return */
        return program_path;
    }

    /* zero out the buffer since readlink(2) doesn't add a nul character */
    memset(buffer, 0, sizeof(buffer));

#if defined(WINDOWS_COMPILE)

    /* We use -1 since Windows XP will not include the terminating nul character,
     * while later versions do, yay! :) */
    result = GetModuleFileName(NULL, buffer, sizeof(buffer) - 1);
    if (result == 0 || ((result == sizeof(buffer) - 1) && GetLastError() != 0)) {
        log_error(LOG_ERR,
                "failed to retrieve executable path, falling back"
                " to getcwd() + argv[0]");
        if (!argv_fallback()) {
            archdep_vice_exit(1);
        }
    }

#elif defined(UNIX_COMPILE)

    /* XXX: Only works on Linux, OSX and FreeBSD/NetBSD, anyone wanting support
     *      for OpenBSD or DragonflyBSD will have to add it.
     *
     *      Linux:      readlink(/proc/self/exe)
     *      MacOS:      _NSGetExecutablePath()
     *      FreeBSD:    sysctl CTL_KERN_PROC KERN_PROC_PATHNAME - 1 (???)
     *      NetBSD:     readlink(/proc/curproc/exe)
     *      DFlyBSD:    ??? (errors out during build)
     *      OpenBSD:    ??? (using argv[0] fallback)
     */

# ifdef MACOS_COMPILE

    /* get path via libproc */
    pid_t pid = getpid();
    if (proc_pidpath(pid, buffer, sizeof(buffer) - 1) <= 0) {
        log_error(LOG_ERR,
                "failed to retrieve executable path, falling back"
                " to getcwd() + argv[0]");
        if (!argv_fallback()) {
            archdep_vice_exit(1);
        }
    }

# elif defined(LINUX_COMPILE)

    if (readlink("/proc/self/exe", buffer, sizeof(buffer) - 1) < 0) {
        log_error(LOG_ERR,
                "failed to retrieve executable path, falling back"
                " to getcwd() + argv[0]");
        if (!argv_fallback()) {
            archdep_vice_exit(1);
        }
    }

    /* BSD's */
# elif defined(BSD_COMPILE)
#  if defined(FREEBSD_COMPILE)

    int mib[4];
    size_t bufsize = ARCHDEP_PATH_MAX;

    /* /proc may not be available on FreeBSD */
    if (readlink("/proc/curproc/file", buffer, sizeof(buffer) - 1) < 0) {
        /* testing with FreeBSD 13.0 indicates it is indeed not present */
        /* try sysctl call */
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;

        if (sysctl(mib, 4, buffer, &bufsize, NULL, 0) < 0) {
            log_error(LOG_ERR,
                    "failed to retrieve executable path, falling back"
                    " to getcwd() + argv[0]");
            if (!argv_fallback()) {
                archdep_vice_exit(1);
            }
        }
#if 0
        printf("SYSCTL: %s\n", buffer);
#endif
    }

#  elif defined(NETBSD_COMPILE)

    if (readlink("/proc/curproc/exe", buffer, sizeof(buffer) - 1) < 0) {
        log_error(LOG_ERR,
                "failed to retrieve executable path, falling back"
                " to getcwd() + argv[0]");
        if (!argv_fallback()) {
            archdep_vice_exit(1);
        }
    }

#  elif defined(OPENBSD_COMPILE)
    /*
     * I couldn't find any non-argv[0] solution for OpenBSD, so this will have
     * to do. --compyx
     */
    if (!argv_fallback()) {
        archdep_vice_exit(1);
    }
#  elif defined(DRAGONFLYBSD_COMPILE)

    int mib[4];
    size_t bufsize = sizeof(buffer);

    /* /proc may not be available on FreeBSD */
    if (readlink("/proc/curproc/file", buffer, sizeof(buffer) - 1) < 0) {
        /* try sysctl call */
        mib[0] = CTL_KERN;
        mib[1] = KERN_PROC;
        mib[2] = KERN_PROC_PATHNAME;
        mib[3] = -1;

        if (sysctl(mib, 4, buffer, &bufsize, NULL, 0) < 0) {
            log_error(LOG_ERR,
                    "failed to retrieve executable path, falling back"
                    " to getcwd() + argv[0]");
            if (!argv_fallback()) {
                archdep_vice_exit(1);
            }
        }
    }
#  endif    /* end BSD's */

# endif /* end UNIX */
#else

    /*
     * Other systems (BeOS etc)
     */
    if (!argv_fallback()) {
        archdep_vice_exit(1);
    }

#endif
    program_path = lib_strdup(buffer);
#if 0
    printf("%s(): program_path = %s\n", __func__, program_path);
#endif
    return program_path;
}


/** \brief  Free memory used by path to running executable
 *
 * Call from program exit
 */
void archdep_program_path_free(void)
{
    if (program_path != NULL) {
        lib_free(program_path);
        program_path = NULL;
    }
}
