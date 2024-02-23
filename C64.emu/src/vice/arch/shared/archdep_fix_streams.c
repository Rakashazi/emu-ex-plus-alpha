/** \file   archdep_fix_streams.c
 * \brief   Fix stdin, stdout, stderr streams if applicable
 *
 * Redirects stdin, stdout and stderr in case these are not available to the
 * running emulator. Specifically this means Windows compiled with the -mwindows
 * switch, causing stdin, stdout and stderr to be disconnected from the spawning
 * shell (cmd.exe, looks like msys2's shell already does this hack).
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * OS support:
 *  - Windows
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

#include "archdep_fix_streams.h"

/** \fn archdep_fix_streams
 * \brief   Fix standard streams
 *
 * Make the standard streams `stdin`, `stdout` and `stderr` available to the
 * running application so we can log to the terminal, if any, using those
 * streams.
 *
 * \todo    Properly detect (if possible) if we're already redirected to a file
 *          or pipe so `x64sc -help > help.txt` works. GetFileType() should
 *          return something other than FILE_TYPE_CHAR when redirected or piped,
 *          but of course that doesn't actually work on this shitshow they call
 *          Windows.
 */

#if 0
#define DEBUG_FIX
#endif

#ifdef WINDOWS_COMPILE

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void archdep_fix_streams(void)
{
    /* With --enable-debug we compile with -mconsole on Windows, so the streams
     * are available as they should be. */
#ifndef DEBUG
    const char *msystem;
#ifdef DEBUG_FIX
    FILE *log = fopen("winapisux", "w");
#endif

    /* try to attach a console to the spawning process (cmd.exe), but not when
     * running from an msys2 shell */
    msystem = getenv("MSYSTEM");
#ifdef DEBUG_FIX
    fprintf(log, "env('MSYSTEM') = %s\n", msystem ? msystem : "<NULL>");
#endif
    if (msystem == NULL || *msystem == '\0') {
#ifdef DEBUG_FIX
        fprintf(log, "calling AttachConsole(ATTACH_PARENT_PROCESS): ");
        fflush(log);
#endif
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
#ifdef DEBUG_FIX
            fprintf(log, "OK\n");
#endif
            FILE *fp_stdin = stdin;
            FILE *fp_stdout = stdout;
            FILE *fp_stderr = stderr;
            HANDLE stdhandle;
            DWORD ftype;
            BY_HANDLE_FILE_INFORMATION finfo; /* wtf? */
#ifdef DEBUG_FIX
            DWORD size;
            char path[4096];
#endif

            /* redirect stdin */
            stdhandle = GetStdHandle(STD_INPUT_HANDLE);
            if (stdhandle) {
                ftype = GetFileType(stdhandle);
#ifdef DEBUG_FIX
                fprintf(log, "GetFileType(STD_INPUT_HANDLE) = %lu\n", ftype);
#endif
                if (ftype == FILE_TYPE_CHAR) {
                    freopen_s(&fp_stdin, "CONIN$", "r", stdin);
                }
                CloseHandle(stdhandle);
            }

            /* redirect stdout */
            stdhandle = GetStdHandle(STD_OUTPUT_HANDLE);
            if (stdhandle) {
                /* XXX: always 2 (FILE_TYPE_CHAR) */
                ftype = GetFileType(stdhandle);
#ifdef DEBUG_FIX
                fprintf(log, "GetFileType(STD_OUTPUT_HANDLE) = %lu\n", ftype);
#endif
                /* XXX: always fails */
                if (GetFileInformationByHandle(stdhandle, &finfo)) {
#ifdef DEBUG_FIX
                    fprintf(log,
                            "GetFileInformationByHandle(STD_OUTPUT_HANDLE) = %lx\n",
                            finfo.dwFileAttributes);
#endif
                }

                /* XXX: size is always 0 */
#ifdef DEBUG_FIX
                memset(path, 0, sizeof(path));
                size = GetFinalPathNameByHandleA(stdhandle, path, sizeof(path), 0);
                fprintf(log,
                        "GetFinalPathNameByHandleA(STD_OUTPUT_HANDLE) = %lu, '%s'\n",
                        size, path);
#endif

                if (ftype == FILE_TYPE_CHAR) {
                    freopen_s(&fp_stdout, "CONOUT$", "w", stdout);
                }
            }

            /* redirect stderr */
            stdhandle = GetStdHandle(STD_ERROR_HANDLE);
            if (stdhandle) {
                ftype = GetFileType(stdhandle);
#ifdef DEBUG_FIX
                fprintf(log, "GetFileType(STD_ERROR_HANDLE) = %lu\n", ftype);
#endif
                if (ftype == FILE_TYPE_CHAR) {
                    freopen_s(&fp_stderr, "CONOUT$", "w", stderr);
                }
                CloseHandle(stdhandle);
            }
        } else {
            /* Can't report failure since we can't use (f)printf and VICE's
             * logging system isn't initialized yet =) */
            /* NOP */
#ifdef DEBUG_FIX
            fprintf(log, "failed, giving up.\n");
#endif
        }
    }
#ifdef DEBUG_FIX
    fclose(log);
#endif
#endif  /* ifndef DEBUG */
}

#else   /* ifdef WINDOWS_COMPILE */
void archdep_fix_streams(void)
{
    /* Other OSes aren't retarded */
}
#endif  /* ifdef WINDOWS_COMPILE */
