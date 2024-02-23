/** \file   archdep_open_default_log_file.c
 * \brief   Open default log file
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Open default log file `$XDG_STATE_HOME/vice/vice.log`, which defaults to
 * `$HOME/.local/state/vice/vice.log`.
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
#ifdef UNIX_COMPILE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include "archdep_default_logfile.h"
#include "lib.h"
#include "log.h"
#include "util.h"

#include "archdep_open_default_log_file.h"


/** \brief  Opens the default log file
 *
 * On *nix the log goes to stdout by default. If that does not exist, attempt
 * to open a log file in the user's vice state dir. If the file cannot be
 * opened for some reason, stdout is returned anyway.
 *
 * \return  file pointer to log file
 */
FILE *archdep_open_default_log_file(void)
{
    FILE *fp = stdout;
    char *path;

    /* quick fix. on non windows platforms this should check if VICE has been
       started from a terminal, and only if not open a file instead of stdout */
#ifdef UNIX_COMPILE
    if (!isatty(fileno(fp))) {
        struct stat statinfo;
        fstat(fileno(fp), &statinfo);
        /* also check if stdout is connected to a pipe or regular file, in that
           case do not open a logfile either, so we can redirect the output on
           the shell */
        if (!S_ISFIFO(statinfo.st_mode) && !S_ISREG(statinfo.st_mode)) {
#endif
            path = archdep_default_logfile();
            fp = fopen(path, "w");
            if (fp == NULL) {
                log_error(LOG_ERR,
                        "failed to open log file '%s' for writing, reverting to stdout",
                        path);
                fp = stdout;
            }
            lib_free(path);
#ifdef UNIX_COMPILE
        }
    }
#endif
    return fp;
}
