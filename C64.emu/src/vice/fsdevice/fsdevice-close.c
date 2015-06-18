/*
 * fsdevice-close.c - File system device.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  pottendo <pottendo@gmx.net>
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

#include "cbmdos.h"
#include "fileio.h"
#include "fsdevice-close.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "tape.h"
#include "vdrive.h"


int fsdevice_close(vdrive_t *vdrive, unsigned int secondary)
{
    bufinfo_t *bufinfo;

    bufinfo = fsdevice_dev[vdrive->unit - 8].bufinfo;

    if (secondary == 15) {
        fsdevice_error(vdrive, CBMDOS_IPE_OK);
        return FLOPPY_COMMAND_OK;
    }

    switch (bufinfo[secondary].mode) {
        case Write:
        case Read:
        case Append:
            if (bufinfo[secondary].tape->name) {
                tape_image_close(bufinfo[secondary].tape);
            } else {
                if (bufinfo[secondary].fileio_info != NULL) {
                    fileio_close(bufinfo[secondary].fileio_info);
                    bufinfo[secondary].fileio_info = NULL;
                } else {
                    return FLOPPY_ERROR;
                }
            }
            break;
        case Directory:
            if (bufinfo[secondary].ioutil_dir == NULL) {
                return FLOPPY_ERROR;
            }

            ioutil_closedir(bufinfo[secondary].ioutil_dir);
            bufinfo[secondary].ioutil_dir = NULL;
            break;
    }

    return FLOPPY_COMMAND_OK;
}
