/*
 * fsdevice-write.c - File system device.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  André Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include "fileio.h"
#include "fsdevice-flush.h"
#include "fsdevice-write.h"
#include "fsdevicetypes.h"
#include "types.h"
#include "vdrive.h"


int fsdevice_write(struct vdrive_s *vdrive, BYTE data, unsigned int secondary)
{
    bufinfo_t *bufinfo;

    bufinfo = fsdevice_dev[vdrive->unit - 8].bufinfo;

    if (secondary == 15) {
        return fsdevice_flush_write_byte(vdrive, data);
    }

    if (bufinfo[secondary].mode != Write
        && bufinfo[secondary].mode != Append) {
        return SERIAL_ERROR;
    }

    if (bufinfo[secondary].fileio_info != NULL) {
        unsigned int len;

        len = fileio_write(bufinfo[secondary].fileio_info, &data, 1);

        if (len == 0) {
            return SERIAL_ERROR;
        }

        return SERIAL_OK;
    }

    return SERIAL_ERROR;
}
