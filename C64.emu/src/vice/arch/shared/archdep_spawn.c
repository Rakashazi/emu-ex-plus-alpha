/** \file   archdep_spawn.c
 * \brief   Process spawning
 *
 * Hopefully at some point this won't be required anymore.
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
# include <fcntl.h>

#ifdef UNIX_COMPILE
# include <sys/wait.h>
#endif

#ifdef WINDOWS_COMPILE
# include <windows.h>
# include <io.h>
# include <process.h>
#endif

#include "lib.h"
#include "log.h"
#include "util.h"

/* #include "debug_gtk3.h" */

#include "archdep.h"
#include "archdep_defs.h"

#include "archdep_spawn.h"

/* #define DEBUG_SPAWN */

#ifdef DEBUG_SPAWN
#define LOG(a)  log_debug a
#else
#define LOG(a)
#endif

#ifdef UNIX_COMPILE

int archdep_spawn(const char *name, char **argv,
                  char **pstdout_redir, const char *stderr_redir)
{
    pid_t child_pid;
    int child_status;
    char *stdout_redir;


    if (pstdout_redir != NULL) {
        if (*pstdout_redir == NULL) {
            *pstdout_redir = archdep_tmpnam();
        }
        stdout_redir = *pstdout_redir;
    } else {
        stdout_redir = NULL;
    }

    child_pid = vfork();
    if (child_pid < 0) {
        log_error(LOG_DEFAULT, "vfork() failed: %s.", strerror(errno));
        return -1;
    } else {
        if (child_pid == 0) {
            if (stdout_redir && freopen(stdout_redir, "w", stdout) == NULL) {
                log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stdout_redir, strerror(errno));
                _exit(-1);
            }
            if (stderr_redir && freopen(stderr_redir, "w", stderr) == NULL) {
                log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stderr_redir, strerror(errno));
                _exit(-1);
            }
            execvp(name, argv);
            _exit(-1);
        }
    }

    if (waitpid(child_pid, &child_status, 0) != child_pid) {
        log_error(LOG_DEFAULT, "waitpid() failed: %s", strerror(errno));
        return -1;
    }

    if (WIFEXITED(child_status)) {
        return WEXITSTATUS(child_status);
    } else {
        return -1;
    }
}

#elif defined(WINDOWS_COMPILE)

#include "archdep.h"
#include "coproc.h"
#include "lib.h"
#include "log.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

/* https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output */

/* https://marc.info/?l=apreq-dev&m=104840521714506 mentions some hackery we might need too */

/* Create a child process that uses the previously created pipes for STDIN and STDOUT. */
static int CreateChildProcess(
    TCHAR *szCmdline,
    HANDLE hChildStd_IN_Rd,
    HANDLE hChildStd_OUT_Wr,
    PROCESS_INFORMATION *piProcInfo)
{
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    /* Set up members of the PROCESS_INFORMATION structure. */
    ZeroMemory( piProcInfo, sizeof(PROCESS_INFORMATION) );

    /* Set up members of the STARTUPINFO structure. */
    /* This structure specifies the STDIN and STDOUT handles for redirection. */
    ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    siStartInfo.wShowWindow = SW_HIDE;

    /* Create the child process.  */
    bSuccess = CreateProcess(NULL,
        szCmdline,     /* command line */
        NULL,          /* process security attributes */
        NULL,          /* primary thread security attributes */
        TRUE,          /* handles are inherited */
        0,             /* creation flags */
        NULL,          /* use parent's environment */
        NULL,          /* use parent's current directory */
        &siStartInfo,  /* STARTUPINFO pointer */
        piProcInfo);  /* receives PROCESS_INFORMATION */

    /* If an error occurs, exit */
    if (!bSuccess) {
        return -1;
    } else {
        /* Close handles to the child process and its primary thread.
            Some applications might keep these handles to monitor the status
            of the child process, for example. */
        CloseHandle(piProcInfo->hProcess);
        CloseHandle(piProcInfo->hThread);

        /* Close handles to the stdin and stdout pipes no longer needed by the child process.
            If they are not explicitly closed, there is no way to recognize that the child process has ended. */
        CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_IN_Rd);
    }
    return 0;
}

/** \brief  Spawn new process
 */
int archdep_spawn(const char *cmd, char **argv, char **pstdout_redir, const char *stderr_redir)
{
    PROCESS_INFORMATION piProcInfo;
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL;
    SECURITY_ATTRIBUTES saAttr;
    char *cmdline;
    int n;
    int arglen;
    char *stdout_redir;

    if (pstdout_redir != NULL) {
        if (*pstdout_redir == NULL) {
            *pstdout_redir = archdep_tmpnam();
        }
        stdout_redir = *pstdout_redir;
    } else {
        stdout_redir = NULL;
    }
    LOG(("archdep_spawn: stdout_redir: '%s'", stdout_redir));

    if (stderr_redir) {
        /* FIXME: stderr is not redirected yet. No code uses this right now */
        log_error(LOG_DEFAULT, "FIXME: archdep_spawn does not redirect stderr yet");
    }

    /* Set the bInheritHandle flag so pipe handles are inherited.  */
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    /* Create a pipe for the child process's STDOUT. */
     if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        return -1;
    }

    /* Ensure the read handle to the pipe for STDOUT is not inherited. */
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }

    /* Create a pipe for the child process's STDIN. */
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) {
        return -1;
    }

    /* Ensure the write handle to the pipe for STDIN is not inherited. */
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        return -1;
    }

    /* create a string containing the command and arguments */
    arglen = n = 0;
    while(argv[n] != NULL) {
        LOG(("archdep_spawn: argv[%d]:%s", n, argv[n]));
        arglen += strlen(argv[n]) + 1;
        n++;
    }

    /* use a subshell to execute the given cmdline */
    cmdline = lib_malloc(strlen(cmd) + arglen + 20);
    strcpy(cmdline, "cmd.exe /C ");
    strcat(cmdline, cmd);
    n = 1; while(argv[n] != NULL) {
        strcat(cmdline, " ");
        strcat(cmdline, argv[n]);
        n++;
    }
    LOG(("archdep_spawn: cmdline: '%s'", cmdline));

    /* Create the child process. */
    if (CreateChildProcess(cmdline, hChildStd_IN_Rd, hChildStd_OUT_Wr, &piProcInfo) < 0) {
        lib_free(cmdline);
        return -1;
    }

    lib_free(cmdline);

    /* Read output from the child process's pipe for STDOUT
       Stop when there is no more data. */
    {
        #define BUFSIZE 0x100

        DWORD dwRead, dwWritten;
        CHAR chBuf[BUFSIZE];
        BOOL bSuccess = FALSE;
        /* HANDLE hFile = NULL; */
        FILE *fd = NULL;

        /* open a file to redirect stdout to */
        if (stdout_redir) {
#if 0
            /* FIXME: we should probably use CreateFile instead of fopen,
                      but somehow it doesnt work */
            hFile = CreateFile(stdout_redir,           /* name of the write     */
                               GENERIC_WRITE,          /* open for writing      */
                               0,                      /* do not share          */
                               NULL,                   /* default security      */
                               CREATE_NEW,             /* create new file only  */
                               FILE_ATTRIBUTE_NORMAL,  /* normal file           */
                               NULL);                  /* no attr. template     */
            if (hFile == INVALID_HANDLE_VALUE) {
                log_error(LOG_DEFAULT, "could not open '%s'", stdout_redir);
                return - 1;
            }
#else
            fd = fopen(stdout_redir, MODE_WRITE);
            if (fd == NULL) {
                log_error(LOG_DEFAULT, "could not open '%s'", stdout_redir);
                return - 1;
            }
#endif
        }

        for (;;) {
            /* read the stdout of the child */
            bSuccess = ReadFile( hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
            if( ! bSuccess || dwRead == 0 ) {
                break;
            }
            /* write to the file we want to redirect to */
#if 0
            if (!WriteFile(hFile, chBuf, dwRead, &dwWritten, NULL) ) {
                break;
            }
#else
            dwWritten = fwrite(chBuf, 1, dwRead, fd);
            if (dwWritten != dwRead) {
                break;
            }
#endif
       }
        /* wait for the child process to terminate */
        WaitForSingleObject(piProcInfo.hProcess, INFINITE);

        if (stdout_redir) {
#if 0
            CloseHandle(hFile);
#else
            fclose (fd);
#endif
        }
    }
    return 0;
}

#elif defined(BEOS_COMPILE)

int archdep_spawn(const char *name, char **argv,
                  char **pstdout_redir, const char *stderr_redir)
{
    pid_t child_pid;
    int child_status;
    char *stdout_redir = NULL;

    if (pstdout_redir != NULL) {
        if (*pstdout_redir == NULL) {
            *pstdout_redir = archdep_tmpnam();
        }
        stdout_redir = *pstdout_redir;
    }

#ifdef WORDS_BIGENDIAN
    child_pid = -1;
#else
    child_pid = vfork();
#endif

    if (child_pid < 0) {
        log_error(LOG_DEFAULT, "vfork() failed: %s.", strerror(errno));
        return -1;
    } else {
        if (child_pid == 0) {
            if (stdout_redir && freopen(stdout_redir, "w", stdout) == NULL) {
                log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stdout_redir, strerror(errno));
                _exit(-1);
            }
            if (stderr_redir && freopen(stderr_redir, "w", stderr) == NULL) {
                log_error(LOG_DEFAULT, "freopen(\"%s\") failed: %s.", stderr_redir, strerror(errno));
                _exit(-1);
            }
            execvp(name, argv);
            _exit(-1);
        }
    }

    if (waitpid(child_pid, &child_status, 0) != child_pid) {
        log_error(LOG_DEFAULT, "waitpid() failed: %s", strerror(errno));
        return -1;
    }

    if (WIFEXITED(child_status)) {
        return WEXITSTATUS(child_status);
    } else {
        return -1;
    }
}

#else
    /* Unsupported OS's */
int archdep_spawn(const char *name, char **argv,
                  char **stdout_redir, const char *stderr_redir)
{
    return -1;
}

#endif
