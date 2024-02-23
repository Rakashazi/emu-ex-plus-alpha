/** \file   archdep_program_name.c
 * \brief   Retrieve name of currently running binary
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Get name of running executable, stripping path and extension (if present).
 *
 * OS support:
 *  - Linux
 *  - Windows
 *  - MacOS
 *  - BeOS/Haiku
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

/* for readlink(2) */
#ifdef UNIX_COMPILE
# include <unistd.h>
#endif

#ifdef MACOS_COMPILE
# include <libproc.h>
#endif

/* for GetModuleFileName() */
#ifdef WINDOWS_COMPILE
# include "windows.h"
#endif

#include "archdep_exit.h"
#include "archdep_program_path.h"

#include "archdep_program_name.h"


/** \brief  Program name
 *
 * Heap allocated on the first call to #archdep_program_name(), must be freed
 * with #archdep_program_name_free() on emulator shutdown.
 */
static char *program_name = NULL;


#ifdef WINDOWS_COMPILE
/** \brief  Helper function for Windows
 *
 * \param[in]   buf string to parse binary name from
 *
 * \return  binary name
 *
 * \note    free after use with lib_free().
 */
static char *prg_name_win32(const char *buf)
{
    const char *s;
    const char *e;
    size_t len;
    char *tmp;

    s = strrchr(buf, '\\');
    if (s == NULL) {
        s = buf;
    } else {
        s++;
    }
    e = strchr(s, '.');
    if (e == NULL) {
        e = buf + strlen(buf);
    }
    len = (int)(e - s + 1);
    tmp = lib_malloc(len);
    memcpy(tmp, s, len - 1);
    tmp[len - 1] = 0;

    return tmp;
}
#endif


#if defined(UNIX_COMPILE) || defined(BEOS_COMPILE)
/** \brief  Helper function for Unix-ish systems
 *
 * \param[in]   buf string to parse binary name from
 *
 * \return  binary name
 *
 * \note    free after use with lib_free().
 */
static char *prg_name_unix(const char *buf)
{
    const char *p;
    char *tmp;

    p = strrchr(buf, '/');
    if (p == NULL) {
        tmp = lib_strdup(buf);
    } else {
        tmp = lib_strdup(p + 1);
    }
    return tmp;
}
#endif

/** \brief  Get name of the currently running binary
 *
 * Get the name of the running binary, striped from path and extension.
 *
 * \return  program name
 *
 * \note    Use #archdep_program_name_free on emulator shutdown to free memory.
 */
const char *archdep_program_name(void)
{
    const char *execpath;

    /* if we already have found the program name, just return it */
    if (program_name != NULL) {
        return program_name;
    }

    execpath = archdep_program_path();

#if defined(UNIX_COMPILE) || defined(BEOS_COMPILE)
    program_name = prg_name_unix(execpath);
#endif

#ifdef WINDOWS_COMPILE
    program_name = prg_name_win32(execpath);
#endif

    /* returns NULL on systems other than Windows/Unix */
    return program_name;
}


/** \brief  Free program name
 *
 * This function must be called on emulator shutdown to free the program name.
 */
void archdep_program_name_free(void)
{
    if (program_name != NULL) {
        lib_free(program_name);
        program_name = NULL;
    }
}
