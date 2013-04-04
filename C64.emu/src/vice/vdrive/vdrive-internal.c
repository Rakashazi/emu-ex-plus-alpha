/*
 * vdrive-internal.c - Virtual disk-drive implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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
#include <stdlib.h>

#include "attach.h"
#include "cbmdos.h"
#include "cbmimage.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "machine-drive.h"
#include "types.h"
#include "vdrive-command.h"
#include "vdrive-internal.h"
#include "vdrive.h"
#include "p64.h"


static log_t vdrive_internal_log = LOG_DEFAULT;


vdrive_t *vdrive_internal_open_fsimage(const char *name, unsigned int read_only)
{
    vdrive_t *vdrive;
    disk_image_t *image;

    image = lib_malloc(sizeof(disk_image_t));

    image->gcr = NULL;
    image->p64 = lib_calloc(1, sizeof(TP64Image));
    P64ImageCreate((void*)image->p64);
    image->read_only = read_only;

    image->device = DISK_IMAGE_DEVICE_FS;

    disk_image_media_create(image);

    disk_image_name_set(image, lib_stralloc(name));

    if (disk_image_open(image) < 0) {
        disk_image_media_destroy(image);
        P64ImageDestroy((void*)image->p64);
        lib_free(image->p64);
        lib_free(image);
        log_error(vdrive_internal_log, "Cannot open file `%s'", name);
        return NULL;
    }

    vdrive = lib_calloc(1, sizeof(vdrive_t));

    vdrive_device_setup(vdrive, 100);
    vdrive->image = image;
    vdrive_attach_image(image, 100, vdrive);
    return vdrive;
}

int vdrive_internal_close_disk_image(vdrive_t *vdrive)
{
    disk_image_t *image = vdrive->image;

    if (vdrive->unit != 8 && vdrive->unit != 9 && vdrive->unit != 10 && vdrive->unit != 11) {
        vdrive_detach_image(image, 100, vdrive);

        if (disk_image_close(image) < 0) {
            return -1;
        }

        P64ImageDestroy((void*)image->p64);

        disk_image_media_destroy(image);
        vdrive_device_shutdown(vdrive);
        lib_free(image->p64);
        lib_free(image);
        lib_free(vdrive);
    }

    return 0;
}

static int vdrive_internal_format_disk_image(const char *filename,
                                             const char *disk_name)
{
    vdrive_t *vdrive;
    const char *format_name;
    int status = 0;

    format_name = (disk_name == NULL) ? " " : disk_name;

    /* FIXME: Pass unit here.  */
    machine_drive_flush();
    vdrive = vdrive_internal_open_fsimage(filename, 0);

    if (vdrive == NULL) {
        return -1;
    }

    if (vdrive_command_format(vdrive, format_name) != CBMDOS_IPE_OK) {
        status = -1;
    }

    if (vdrive_internal_close_disk_image(vdrive) < 0) {
        return -1;
    }

    return status;
}

int vdrive_internal_create_format_disk_image(const char *filename,
                                             const char *diskname,
                                             unsigned int type)
{
    if (cbmimage_create_image(filename, type) < 0) {
        return -1;
    }
    if (vdrive_internal_format_disk_image(filename, diskname) < 0) {
        return -1;
    }

    return 0;
}

void vdrive_internal_init(void)
{
    vdrive_internal_log = log_open("VDrive Internal");
}
