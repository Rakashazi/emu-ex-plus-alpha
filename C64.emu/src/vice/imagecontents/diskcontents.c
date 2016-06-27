/*
 * diskcontents.c - Extract the directory from disk images.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown@mail.matav.hu>
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

#include "diskcontents-block.h"
#include "diskcontents-iec.h"
#include "diskcontents.h"
#include "imagecontents.h"
#include "lib.h"
#include "machine-bus.h"
#include "machine.h"
#include "serial.h"
#include "attach.h"
#include "vdrive-internal.h"


image_contents_t *diskcontents_read(const char *file_name, unsigned int unit)
{
    switch (machine_bus_device_type_get(unit)) {
        default:
            return diskcontents_filesystem_read(file_name);
        case SERIAL_DEVICE_REAL:
            return machine_diskcontents_bus_read(unit);
        case SERIAL_DEVICE_RAW:
            return diskcontents_block_read(file_system_get_vdrive(unit));
    }
}

image_contents_t *diskcontents_filesystem_read(const char *file_name)
{
    return diskcontents_block_read(vdrive_internal_open_fsimage(file_name, 1));
}

image_contents_t *diskcontents_read_unit8(const char *file_name)
{
    return diskcontents_read(file_name, 8);
}

image_contents_t *diskcontents_read_unit9(const char *file_name)
{
    return diskcontents_read(file_name, 9);
}

image_contents_t *diskcontents_read_unit10(const char *file_name)
{
    return diskcontents_read(file_name, 10);
}

image_contents_t *diskcontents_read_unit11(const char *file_name)
{
    return diskcontents_read(file_name, 11);
}
