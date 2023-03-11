/** \file   archdep_beos.h
 * \brief   Miscellaneous BeOS system-specific stuff
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
 * \author  Marcus Sutton <loggedoubt@gmail.com>
 *
 * TODO:    Either of these authors should properly document the defines using
 *          Doxygen.
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

#ifndef VICE_ARCHDEP_BEOS_H
#define VICE_ARCHDEP_BEOS_H

#define VICE_ARCHAPI_PRIVATE_API
#include "archapi.h"
#undef VICE_ARCHAPI_PRIVATE_API

/* Video chip scaling.  */
#define ARCHDEP_VICII_DSIZE   1
#define ARCHDEP_VICII_DSCAN   1
#define ARCHDEP_VDC_DSIZE     1
#define ARCHDEP_VDC_DSCAN     1
#define ARCHDEP_VIC_DSIZE     1
#define ARCHDEP_VIC_DSCAN     1
#define ARCHDEP_CRTC_DSIZE    1
#define ARCHDEP_CRTC_DSCAN    1
#define ARCHDEP_TED_DSIZE     1
#define ARCHDEP_TED_DSCAN     1

/* Filesystem-dependent constants  */
#define ARCHDEP_FSDEVICE_DEFAULT_DIR "."
#define ARCHDEP_DIR_SEP_STR "/"
#define ARCHDEP_DIR_SEP_CHR '/'

/* Path separator.  */
#define ARCHDEP_FINDPATH_SEPARATOR_CHAR   ':'
#define ARCHDEP_FINDPATH_SEPARATOR_STRING ":"

/* Modes for fopen().  */
#define MODE_READ              "r"
#define MODE_READ_TEXT         "rt"
#define MODE_READ_WRITE        "r+"
#define MODE_WRITE             "w"
#define MODE_WRITE_TEXT        "wt"
#define MODE_APPEND            "a"
#define MODE_APPEND_READ_WRITE "a+"

/* Printer default devices.  */
#define ARCHDEP_PRINTER_DEFAULT_DEV1 "PrinterFile"
#define ARCHDEP_PRINTER_DEFAULT_DEV2 "/dev/parallel/parallel1"
#define ARCHDEP_PRINTER_DEFAULT_DEV3 "/dev/printer/usb/"

/* Default RS232 devices.  */
#define ARCHDEP_RS232_DEV1 "127.0.0.1:25232"
#define ARCHDEP_RS232_DEV2 "127.0.0.1:25232"
#define ARCHDEP_RS232_DEV3 "127.0.0.1:25232"
#define ARCHDEP_RS232_DEV4 "127.0.0.1:25232"

/* Default location of raw disk images.  */
#define ARCHDEP_RAWDRIVE_DEFAULT "/dev/disk/floppy/raw"

/* Standard line delimiter.  */
#define ARCHDEP_LINE_DELIMITER "\n"

/* Ethernet default device */
#define ARCHDEP_ETHERNET_DEFAULT_DEVICE ""

/*
    FIXME: confirm wether SIGPIPE must be handled or not. if the emulator quits
           or crashes when the connection is closed, you might have to install
           a signal handler which calls monitor_abort().

           see archdep_unix.c and bug #3201796
*/
#if 0
#define archdep_signals_init(x)
#define archdep_signals_pipe_set()
#define archdep_signals_pipe_unset()
#endif

/* what to use to return an error when a socket error happens */
#define ARCHDEP_SOCKET_ERROR errno

#endif
