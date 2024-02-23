/** \file   archdep_get_runtime_info.c
 * \brief   Get runtime information
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "archdep_defs.h"
#include "log.h"

#ifdef UNIX_COMPILE
# include <sys/utsname.h>
#elif defined(WINDOWS_COMPILE)
# include <windows.h>
# include <versionhelpers.h>
#endif


#include "archdep_get_runtime_info.h"


#ifdef WINDOWS_COMPILE

/* The following might seem weird and convoluted, and it is: Windows does not
 * have a simple method to determine what arch a process is running on.
 *
 * IsWow64Process() doesn't test for a process being a 64-bit process, it
 * tests if a 32-bit process is being run as a 64-bit process.
 *
 * We can check if we are compiled as 64-bit, in that case, we cannot run on
 * 32-bit and thus we're running on 64-bit. Done.
 *
 * If we are compiled as 32-bit. we might be running as 64-bit, check that via
 * IsWow64Process()
 *
 * By Loki, Windows is great!
 */


/** \brief  No idea what this is supposed to do :)
 *
 */
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);


/** \brief  Check if the Windows OS is x64
 *
 * \return  boolean
 */
static BOOL os_is_win64(void)
{
    /* IsWow64Process() returns FALSE when both OS and process are 64-bit */
#ifdef WIN64_COMPILE
    return TRUE;
#else
    BOOL amd64 = FALSE;

    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
            GetModuleHandle("kernel32"), "IsWow64Process");

    if (fnIsWow64Process != NULL) {
        if (!fnIsWow64Process(GetCurrentProcess(), &amd64))  {
            log_error(LOG_ERR, "failed to determine arch");
            return FALSE;
        }
    }
    return amd64;
#endif
}
#endif


/** \brief  Get runtime info
 *
 * Try to retrieve OS name, version, release and arch.
 *
 * \param[out]  info    OS info struct
 *
 * \return  bool
 */
bool archdep_get_runtime_info(archdep_runtime_info_t *info)
{
#ifdef UNIX_COMPILE
    struct utsname buf;
#endif
#ifdef WINDOWS_COMPILE
    const char *name;
    OSVERSIONINFOA version;
#endif

    /* set defaults */
    memset(info->os_name, 0, ARCHDEP_RUNTIME_STRMAX);
    memset(info->os_version, 0, ARCHDEP_RUNTIME_STRMAX);
    memset(info->os_release, 0, ARCHDEP_RUNTIME_STRMAX);
    memset(info->machine, 0, ARCHDEP_RUNTIME_STRMAX);

#ifdef UNIX_COMPILE
    if (uname(&buf) == 0) {
        /* OK */
        printf("sysname = '%s'\n", buf.sysname);
        printf("release = '%s'\n", buf.release);
        printf("version = '%s'\n", buf.version);
        printf("machine = '%s'\n", buf.machine);

        strncpy(info->os_name, buf.sysname, ARCHDEP_RUNTIME_STRMAX - 1U);
        strncpy(info->os_version, buf.version, ARCHDEP_RUNTIME_STRMAX - 1U);
        strncpy(info->os_release, buf.release, ARCHDEP_RUNTIME_STRMAX - 1U);
        strncpy(info->machine, buf.machine, ARCHDEP_RUNTIME_STRMAX - 1U);
        return true;
    }
#elif defined(WINDOWS_COMPILE)

    version.dwOSVersionInfoSize = sizeof(version);
    if (!GetVersionEx(&version)) {
        return false;
    }

    name = "Unknown";

    if (IsWindows10OrGreater()) {
        /* yes, not correct, fuck it */
        name = "10";
    } else if (IsWindows8Point1OrGreater()) {
        name = "8.1";
    } else if (IsWindows8OrGreater()) {
        name = "8.0";
    } else if (IsWindows7SP1OrGreater()) {
        name = "7SP1";
    } else if (IsWindows7OrGreater()) {
        name = "7";
    }

    snprintf(info->os_name, ARCHDEP_RUNTIME_STRMAX - 1U, "Windows %s", name);
    snprintf(info->os_release, ARCHDEP_RUNTIME_STRMAX -1U, "%s",
            version.szCSDVersion);
    if (os_is_win64()) {
        strcpy(info->machine, "x86_64 (64-bit)");
    } else {
        strcpy(info->machine, "x86 (32-bit)");
    }
    return true;
#endif
    return false;
}
