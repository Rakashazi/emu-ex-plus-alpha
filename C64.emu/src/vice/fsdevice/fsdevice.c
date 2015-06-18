/*
 * fsdevice.c - File system device.
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
#include <string.h>

#include "attach.h"
#include "cbmdos.h"
#include "fileio.h"
#include "fsdevice-close.h"
#include "fsdevice-flush.h"
#include "fsdevice-open.h"
#include "fsdevice-read.h"
#include "fsdevice-resources.h"
#include "fsdevice-write.h"
#include "fsdevice.h"
#include "fsdevicetypes.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "machine-bus.h"
#include "resources.h"
#include "tape.h"
#include "vdrive-command.h"
#include "vdrive.h"


fsdevice_dev_t fsdevice_dev[FSDEVICE_DEVICE_MAX];


void fsdevice_set_directory(char *filename, unsigned int unit)
{
    switch (unit) {
        case 8:
        case 9:
        case 10:
        case 11:
            resources_set_string_sprintf("FSDevice%iDir", filename, unit);
            break;
        default:
            log_message(LOG_DEFAULT, "Invalid unit number %d.", unit);
    }
    return;
}

char *fsdevice_get_path(unsigned int unit)
{
    switch (unit) {
        case 8:
        case 9:
        case 10:
        case 11:
            return fsdevice_dir[unit - 8];
        default:
            log_error(LOG_DEFAULT,
                      "fsdevice_get_path() called with invalid device %d.", unit);
            break;
    }
    return NULL;
}

void fsdevice_error(vdrive_t *vdrive, int code)
{
    unsigned int dnr;
    static int last_code[4];
    const char *message;
    unsigned int trk = 0, sec = 0;

    dnr = vdrive->unit - 8;

    /* Only set an error once per command */
    if (code != CBMDOS_IPE_OK && last_code[dnr] != CBMDOS_IPE_OK
        && last_code[dnr] != CBMDOS_IPE_DOS_VERSION) {
        return;
    }

    last_code[dnr] = code;

    if (code != CBMDOS_IPE_MEMORY_READ) {
        if (code == CBMDOS_IPE_DOS_VERSION) {
            message = "VICE FS DRIVER V2.0";
        } else {
            message = cbmdos_errortext(code);
        }

        if ((code != CBMDOS_IPE_OK) && (code != CBMDOS_IPE_DOS_VERSION)) {
            trk = fsdevice_dev[dnr].track;
            sec = fsdevice_dev[dnr].sector;
        }

        sprintf(fsdevice_dev[dnr].errorl, "%02d,%s,%02d,%02d\015", code, message, trk, sec);

        fsdevice_dev[dnr].elen = (unsigned int)strlen(fsdevice_dev[dnr].errorl);

        if (code && code != CBMDOS_IPE_DOS_VERSION) {
            log_message(LOG_DEFAULT, "Fsdevice: ERR = %02d, %s, %02d, %02d", code, message, trk, sec);
        }
    } else {
        memcpy(fsdevice_dev[dnr].errorl, vdrive->mem_buf, vdrive->mem_length);
        fsdevice_dev[dnr].elen = vdrive->mem_length;
    }

    fsdevice_dev[dnr].eptr = 0;
}

int fsdevice_error_get_byte(vdrive_t *vdrive, BYTE *data)
{
    unsigned int dnr;
    int rc;

    dnr = vdrive->unit - 8;
    rc = SERIAL_OK;

    if (!fsdevice_dev[dnr].elen) {
        fsdevice_error(vdrive, CBMDOS_IPE_OK);
    }

    *data = (BYTE)(fsdevice_dev[dnr].errorl)[(fsdevice_dev[dnr].eptr)++];
    if (fsdevice_dev[dnr].eptr >= fsdevice_dev[dnr].elen) {
        fsdevice_error(vdrive, CBMDOS_IPE_OK);
        rc = SERIAL_EOF;
    }

#if 0
    if (fsdevice_dev[dnr].eptr < fsdevice_dev[dnr].elen) {
        *data = (BYTE)(fsdevice_dev[dnr].errorl)[(fsdevice_dev[dnr].eptr)++];
        rc = SERIAL_OK;
    } else {
        fsdevice_error(vdrive, CBMDOS_IPE_OK);
        *data = 0xc7;
        rc = SERIAL_EOF;
    }
#endif

    return rc;
}

int fsdevice_attach(unsigned int device, const char *name)
{
    vdrive_t *vdrive;

    vdrive = file_system_get_vdrive(device);

    if (machine_bus_device_attach(device, name, fsdevice_read, fsdevice_write,
                                  fsdevice_open, fsdevice_close,
                                  fsdevice_flush, NULL)) {
        return 1;
    }

    vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
    fsdevice_error(vdrive, CBMDOS_IPE_DOS_VERSION);
    return 0;
}

void fsdevice_init(void)
{
    unsigned int i, j;
    unsigned int maxpathlen;

    maxpathlen = ioutil_maxpathlen();

    for (i = 0; i < FSDEVICE_DEVICE_MAX; i++) {
        bufinfo_t *bufinfo;

        fsdevice_dev[i].errorl = lib_calloc(1, maxpathlen);
        fsdevice_dev[i].cmdbuf = lib_calloc(1, maxpathlen);

        fsdevice_dev[i].cptr = 0;

        bufinfo = fsdevice_dev[i].bufinfo;

        for (j = 0; j < FSDEVICE_BUFFER_MAX; j++) {
            bufinfo[j].tape = lib_calloc(1, sizeof(tape_image_t));
            bufinfo[j].dir = lib_calloc(1, maxpathlen);
            bufinfo[j].name = lib_calloc(1, maxpathlen);
            bufinfo[j].dirmask = lib_calloc(1, maxpathlen);
        }
    }
}

void fsdevice_shutdown(void)
{
    unsigned int i, j;

    for (i = 0; i < FSDEVICE_DEVICE_MAX; i++) {
        bufinfo_t *bufinfo;

        bufinfo = fsdevice_dev[i].bufinfo;

        for (j = 0; j < FSDEVICE_BUFFER_MAX; j++) {
            lib_free(bufinfo[j].tape);
            lib_free(bufinfo[j].dir);
            lib_free(bufinfo[j].name);
            lib_free(bufinfo[j].dirmask);
        }

        lib_free(fsdevice_dev[i].errorl);
        lib_free(fsdevice_dev[i].cmdbuf);
    }
}
