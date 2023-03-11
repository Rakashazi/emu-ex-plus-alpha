/** \file   archdep_unix.h
 * \brief   Miscellaneous UNIX-specific stuff - header
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

#ifndef VICE_ARCHDEP_UNIX_H
#define VICE_ARCHDEP_UNIX_H

#include "vice.h"

#define VICE_ARCHAPI_PRIVATE_API
#include "archapi.h"
#undef VICE_ARCHAPI_PRIVATE_API

/* Default MIDI devices. */
#define ARCHDEP_MIDI_IN_DEV  "/dev/midi"    /**< MIDI input dev */
#define ARCHDEP_MIDI_OUT_DEV "/dev/midi"    /**< MIDI output dev */


/* Filesystem-dependent constants */
#define ARCHDEP_FSDEVICE_DEFAULT_DIR "."    /**< CWD */
#define ARCHDEP_DIR_SEP_STR "/"     /**< directory separator as a string */
#define ARCHDEP_DIR_SEP_CHR '/'     /**< directory separator as an integer */

/* Path separator.  */

/** \brief  Separator character to split directory lists, as an integer
 */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ':'

/** \brief  Separator character to split directory lists, as a string
 */
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ":"

/* Modes for fopen()
 *
 * I'm not going to document this, just use "rb" or "wb" on all OS'es, and
 * handle line endings accordingly. At least that way using ftell()/fseek()
 * works properly without having to deal with any Windows weirdness.
 */
#define MODE_READ              "r"  /**< read mode (binary) */
#define MODE_READ_TEXT         "r"  /**< read mode (text) */
#define MODE_READ_WRITE        "r+" /**< read/write mode */
#define MODE_WRITE             "w"  /**< write mode (binary) */
#define MODE_WRITE_TEXT        "w"  /**< write mode (text) */
#define MODE_APPEND            "a"  /**< append mode */
#define MODE_APPEND_READ_WRITE "a+" /**< append mode and read/write(?) */

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "print.dump"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "|lpr"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "|petlp -F PS|lpr"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "/dev/ttyS0"
#define ARCHDEP_RS232_DEV2 "/dev/ttyS1"
#define ARCHDEP_RS232_DEV3 "127.0.0.1:25232"
#define ARCHDEP_RS232_DEV4 "|nc 127.0.0.1 25232"

/* Default MIDI devices.  */
#define ARCHDEP_MIDI_IN_DEV  "/dev/midi"
#define ARCHDEP_MIDI_OUT_DEV "/dev/midi"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "/dev/fd0"

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE "eth0"


/*
    these are used for socket send/recv. in this case we might
    get SIGPIPE if the connection is unexpectedly closed.
*/
void archdep_signals_init(int do_core_dumps);
void archdep_signals_pipe_set(void);
void archdep_signals_pipe_unset(void);

#ifdef MACOS_COMPILE
#define MAKE_SO_NAME_VERSION_PROTO(name, version)  "lib" #name "." #version ".dylib"
#else
#define MAKE_SO_NAME_VERSION_PROTO(name, version)  "lib" #name ".so." #version
#endif

/* add second level macro to allow expansion and stringification */
#define ARCHDEP_MAKE_SO_NAME_VERSION(n, v) MAKE_SO_NAME_VERSION_PROTO(n, v)

#ifdef MACOS_COMPILE
#define ARCHDEP_OPENCBM_SO_NAME  "libopencbm.dylib"
#define ARCHDEP_LAME_SO_NAME     "libmp3lame.dylib"
#else
#define ARCHDEP_OPENCBM_SO_NAME  "libopencbm.so"
#define ARCHDEP_LAME_SO_NAME     "libmp3lame.so"
#endif

/* what to use to return an error when a socket error happens
 *
 * Really?
 */
#define ARCHDEP_SOCKET_ERROR errno


#endif
