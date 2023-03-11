/** \file   archdep_win32.h
 * \brief   Windows-specific stuff - header
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_ARCHDEP_WIN32_H
#define VICE_ARCHDEP_WIN32_H

#define VICE_ARCHAPI_PRIVATE_API
#include "archapi.h"
#undef VICE_ARCHAPI_PRIVATE_API

/* Default MIDI devices. */
#define ARCHDEP_MIDI_IN_DEV  "0"    /**< MIDI input dev */
#define ARCHDEP_MIDI_OUT_DEV "0"    /**< MIDI output dev */

/* Filesystem-dependent constants  */
#define ARCHDEP_FSDEVICE_DEFAULT_DIR "."
#define ARCHDEP_DIR_SEP_STR "\\"
#define ARCHDEP_DIR_SEP_CHR '\\'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ';'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ";"

/* Modes for fopen().  */
#define MODE_READ              "rb"
#define MODE_READ_TEXT         "rt"
#define MODE_READ_WRITE        "rb+"
#define MODE_WRITE             "wb"
#define MODE_WRITE_TEXT        "wt"
#define MODE_APPEND            "ab"
#define MODE_APPEND_READ_WRITE "ab+"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "viceprnt.out"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "LPT1:"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "LPT2:"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "com1"
#define ARCHDEP_RS232_DEV2 "com2"
#define ARCHDEP_RS232_DEV3 "127.0.0.1:25232"
#define ARCHDEP_RS232_DEV4 "127.0.0.1:25232"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "A:"

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\r\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE ""

/*
    FIXME: confirm wether SIGPIPE must be handled or not. if the emulator quits
           or crashes when the connection is closed, you might have to install
           a signal handler which calls monitor_abort().

           see archdep_unix.c and bug #3201796
*/
void archdep_signals_init(int do_coredumps);

#define MAKE_SO_NAME_VERSION_PROTO(name, version)  #name "-" #version ".dll"

/* add second level macro to allow expansion and stringification */
#define ARCHDEP_MAKE_SO_NAME_VERSION(n, v) MAKE_SO_NAME_VERSION_PROTO(n, v)

#define ARCHDEP_OPENCBM_SO_NAME  "opencbm.dll"
#define ARCHDEP_LAME_SO_NAME     "lame.dll"

/* ffmpeg headers for windows don't seem to have some of the av_ prefixes */
#define ARCHDEP_AV_PREFIX_NEEDED

/* Needs extra call to log_archdep() even when logfile is already opened */
#define ARCHDEP_EXTRA_LOG_CALL

/* When using the ascii printer driver we need a return before the newline */
#define ARCHDEP_PRINTER_RETURN_BEFORE_NEWLINE

/* what to use to return an error when a socket error happens */
#define ARCHDEP_SOCKET_ERROR WSAGetLastError()

int archdep_is_windows_nt(void);

#endif

/* FIXME: was dangling around in old archdep. remove once everything is working */
#if 0
static char *system_mbstowcs_alloc(const char *mbs)
{
    char *wcs;
    size_t len;

    if (mbs == NULL) {
        return NULL;
    }

    len = strlen(mbs);

    wcs = lib_malloc(len + 1);
    return memcpy(wcs, mbs, len + 1);
}

static void system_mbstowcs_free(char *wcs)
{
    lib_free(wcs);
}
#endif
