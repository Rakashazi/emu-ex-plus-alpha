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
#include "vdrive.h"
#include "vdrive-internal.h"

image_contents_t *diskcontents_filesystem_read(const char *file_name)
{
    vdrive_t *vdrive;
    image_contents_t *contents = NULL;

    vdrive = vdrive_internal_open_fsimage(file_name, 1);

    if (vdrive) {
        contents = diskcontents_block_read(vdrive, 0);
        vdrive_internal_close_disk_image(vdrive);
    }

    return contents;
}
