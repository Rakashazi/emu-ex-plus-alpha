/*
 * mon_drive.c - The VICE built-in monitor drive functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
#include <string.h>

#include "attach.h"
#include "charset.h"
#include "diskcontents-block.h"
#include "diskimage.h"
#include "imagecontents.h"
#include "lib.h"
#include "machine-bus.h"
#include "montypes.h"
#include "mon_drive.h"
#include "mon_util.h"
#include "resources.h"
#include "serial.h"
#include "types.h"
#include "uimon.h"
#include "vdrive.h"
#include "vdrive-command.h"


#define ADDR_LIMIT(x) ((uint16_t)(addr_mask(x)))


void mon_drive_block_cmd(int op, int track, int sector, MON_ADDR addr)
{
    vdrive_t *vdrive;
    /* TODO: drive 1? */
    unsigned int drive = 0;

    mon_evaluate_default_addr(&addr);

    vdrive = file_system_get_vdrive(8);

    if (!vdrive) {
        mon_out("No disk attached\n");
        return;
    }

    if (!op) {
        uint8_t readdata[256];
        int i, j, dst;
        MEMSPACE dest_mem;

        /* We ignore disk error codes here.  */
        if (vdrive_ext_read_sector(vdrive, drive, readdata, track, sector)
            < 0) {
            mon_out("Error reading track %d sector %d\n", track, sector);
            return;
        }

        if (mon_is_valid_addr(addr)) {
            dst = addr_location(addr);
            dest_mem = addr_memspace(addr);

            for (i = 0; i < 256; i++) {
                mon_set_mem_val(dest_mem, ADDR_LIMIT(dst + i), readdata[i]);
            }

            mon_out("Read track %d sector %d into address $%04x\n",
                    track, sector, (unsigned int)dst);
        } else {
            for (i = 0; i < 16; i++) {
                mon_out(">%04x", (unsigned int)(i * 16));
                for (j = 0; j < 16; j++) {
                    if ((j & 3) == 0) {
                        mon_out(" ");
                    }
                    mon_out(" %02x", readdata[i * 16 + j]);
                }
                mon_out("\n");
            }
        }
    } else {
        uint8_t writedata[256];
        int i, src;
        MEMSPACE src_mem;

        src = addr_location(addr);
        src_mem = addr_memspace(addr);

        for (i = 0; i < 256; i++) {
            writedata[i] = mon_get_mem_val(src_mem, ADDR_LIMIT(src + i));
        }

        if (vdrive_ext_write_sector(vdrive, drive, writedata, track, sector)) {
            mon_out("Error writing track %d sector %d\n", track, sector);
            return;
        }

        mon_out("Write data from address $%04x to track %d sector %d\n",
                (unsigned int)src, track, sector);
    }
}


void mon_drive_execute_disk_cmd(char *cmd)
{
    unsigned int len;
    vdrive_t *vdrive;

    /* Unit? */
    vdrive = file_system_get_vdrive(8);

    len = (unsigned int)strlen(cmd);
    charset_petconvstring((uint8_t*)cmd, CONVERT_TO_PETSCII);
    vdrive_command_execute(vdrive, (uint8_t *)cmd, len);
}

/* FIXME: this function should perhaps live elsewhere */
/* check if a drive is associated with a filesystem/directory */
int mon_drive_is_fsdevice(int drive_unit)
{
    int virtualdev = 0, truedrive = 0, iecdevice = 0 /* , fsdevice = 0 */;
    /* FIXME: unsure if this check really works as advertised */
    resources_get_int_sprintf("VirtualDevice%d", &virtualdev, drive_unit);
    resources_get_int_sprintf("Drive%dTrueEmulation", &truedrive, drive_unit);
    resources_get_int_sprintf("IECDevice%i", &iecdevice, drive_unit);
    /* resources_get_int_sprintf("FileSystemDevice%i", &fsdevice, drive_unit); */
    if ((virtualdev && !truedrive) || (!virtualdev && iecdevice)) {
        if (machine_bus_device_type_get(drive_unit) == SERIAL_DEVICE_FS) {
            return 1;
        }
    }
    return 0;
}

/* FIXME: this function should perhaps live elsewhere */
/* for a given drive unit, return the associated fsdevice directory, or NULL
   if there is none */
const char *mon_drive_get_fsdevice_path(int drive_unit)
{
    const char *fspath = NULL;
    if (mon_drive_is_fsdevice(drive_unit)) {
        resources_get_string_sprintf("FSDevice%iDir", &fspath, drive_unit);
    }
    return fspath;
}

void mon_drive_list(int drive_unit)
{
    image_contents_t *listing;
    vdrive_t *vdrive;
    /* TODO: drive 1? */
    const char *fspath = NULL;

    if ((drive_unit < 8) || (drive_unit > 11)) {
        drive_unit = 8;
    }

    vdrive = file_system_get_vdrive(drive_unit);

    if (vdrive == NULL || vdrive->image == NULL) {
        if ((fspath = mon_drive_get_fsdevice_path(drive_unit))) {
            mon_show_dir(fspath);
            return;
        }
        mon_out("Drive %i not ready.\n", drive_unit);
        return;
    }

    listing = diskcontents_block_read(vdrive, 0);

    if (listing != NULL) {
        char *string = image_contents_to_string(listing, IMAGE_CONTENTS_STRING_ASCII);
        image_contents_file_list_t *element = listing->file_list;

        mon_out("%s\n", string);
        lib_free(string);

        if (element == NULL) {
            mon_out("Empty image\n");
        } else {
            do {
                string = image_contents_file_to_string(element, IMAGE_CONTENTS_STRING_ASCII);
                mon_out("%s\n", string);
                lib_free(string);
            }
            while ((element = element->next) != NULL);
        }

        if (listing->blocks_free >= 0) {
            string = lib_msprintf("%d blocks free.\n", listing->blocks_free);
            mon_out("%s", string);
            lib_free(string);
        }
    }
}

