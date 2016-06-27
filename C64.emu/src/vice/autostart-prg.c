/*
 * autostart-prg.c - Handle autostart of program files
 *
 * Written by
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#include <string.h>

#include "archdep.h"
#include "attach.h"
#include "autostart-prg.h"
#include "fsdevice.h"
#include "lib.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "util.h"

#include "diskimage.h"
#include "vdrive.h"
#include "vdrive-iec.h"
#include "vdrive-internal.h"
#include "drive.h"

/* ----- Globals ----- */
extern int autostart_basic_load;

/* program from last injection */
static autostart_prg_t *inject_prg;


static autostart_prg_t * load_prg(const char *file_name, fileio_info_t *finfo, log_t log)
{
    DWORD ptr;
    DWORD end;
    BYTE lo, hi;
    int i;
    autostart_prg_t *prg;

    prg = lib_malloc(sizeof(autostart_prg_t));
    if (prg == NULL) {
        return NULL;
    }

    /* get data size of file */
    prg->size = fileio_get_bytes_left(finfo);
    prg->data = NULL;

    /* read start address */
    if ((fileio_read(finfo, &lo, 1) != 1) || (fileio_read(finfo, &hi, 1) != 1)) {
        log_error(log, "Cannot read start address from '%s'", file_name);
        return NULL;
    }

    /* get load addr */
    if (autostart_basic_load) {
        mem_get_basic_text(&prg->start_addr, NULL);
    } else {
        prg->start_addr = (WORD)hi << 8 | (WORD)lo;
    }
    prg->size -= 2; /* skip load addr */

    /* check range */
    end = prg->start_addr + prg->size - 1;
    if (end > 0xffff) {
        log_error(log, "Invalid size of '%s': %d", file_name, (unsigned int)prg->size);
        return NULL;
    }

    /* load to memory */
    prg->data = lib_malloc(prg->size);
    if (prg->data == NULL) {
        log_error(log, "No memory for '%s'", file_name);
        return NULL;
    }

    /* copy data to memory */
    ptr = prg->start_addr;
    i = 0;
    while (ptr <= end) {
        if (fileio_read(finfo, &(prg->data[i]), 1) != 1) {
            log_error(log, "Error loading data from '%s'", file_name);
            lib_free(prg->data);
            return NULL;
        }
        ptr++;
        i++;
    }

    return prg;
}

static void free_prg(autostart_prg_t *prg)
{
    lib_free(prg->data);
    lib_free(prg);
}

/* ---------- main interface ---------- */

void autostart_prg_init(void)
{
    inject_prg = NULL;
}

void autostart_prg_shutdown(void)
{
    if (inject_prg != NULL) {
        free_prg(inject_prg);
    }
}

int autostart_prg_with_virtual_fs(const char *file_name,
                                  fileio_info_t *fh,
                                  log_t log)
{
    char *directory;
    char *file;
    int val;

    /* Extract the directory path to allow FS-based drive emulation to
       work.  */
    util_fname_split(file_name, &directory, &file);

    if (archdep_path_is_relative(directory)) {
        char *tmp;
        archdep_expand_path(&tmp, directory);
        lib_free(directory);
        directory = tmp;

        /* FIXME: We should actually eat `.'s and `..'s from `directory'
           instead.  */
    }

    /* Setup FS-based drive emulation.  */
    fsdevice_set_directory(directory ? directory : ".", 8);
    resources_get_int("AutostartHandleTrueDriveEmulation", &val);
    if (val == 0) {
        resources_set_int("DriveTrueEmulation", 0);
    }
    resources_set_int("VirtualDevices", 1);
    resources_set_int("FSDevice8ConvertP00", 1);
    file_system_detach_disk(8);
    resources_set_int("FileSystemDevice8", ATTACH_DEVICE_FS);

    lib_free(directory);
    lib_free(file);

    return 0;
}

int autostart_prg_with_ram_injection(const char *file_name,
                                     fileio_info_t *fh,
                                     log_t log)
{
    /* clean up old injection */
    if (inject_prg != NULL) {
        free_prg(inject_prg);
    }

    /* load program file into memory */
    inject_prg = load_prg(file_name, fh, log);
    return (inject_prg == NULL) ? -1 : 0;
}

int autostart_prg_with_disk_image(const char *file_name,
                                  fileio_info_t *fh,
                                  log_t log,
                                  const char *image_name)
{
    const int drive = 8;
    const int secondary = 1;
    autostart_prg_t *prg;
    vdrive_t *vdrive;
    int i;
    int old_tde_state;
    int file_name_size;
    BYTE data;
    unsigned int disk_image_type;
    int result, result2;

    /* identify disk image type */
    switch (drive_get_disk_drive_type(drive - 8)) {
    case DRIVE_TYPE_1540:
    case DRIVE_TYPE_1541:
    case DRIVE_TYPE_1541II:
    case DRIVE_TYPE_1551:
    case DRIVE_TYPE_1570:
    case DRIVE_TYPE_2031:
        disk_image_type = DISK_IMAGE_TYPE_D64; 
        break;
    case DRIVE_TYPE_2040:
    case DRIVE_TYPE_3040:
    case DRIVE_TYPE_4040:
        disk_image_type = DISK_IMAGE_TYPE_D67; 
        break;
    case DRIVE_TYPE_1571:
    case DRIVE_TYPE_1571CR:
        disk_image_type = DISK_IMAGE_TYPE_D71;
        break;
    case DRIVE_TYPE_1581:
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
        disk_image_type = DISK_IMAGE_TYPE_D81; 
        break;
    case DRIVE_TYPE_8050:
        disk_image_type = DISK_IMAGE_TYPE_D80;
        break;
    case DRIVE_TYPE_8250:
    case DRIVE_TYPE_1001:
        disk_image_type = DISK_IMAGE_TYPE_D82;
        break;
    default: 
        log_error(log, "No idea what disk image format to use.");
        return -1;
    }

    /* read prg file */
    prg = load_prg(file_name, fh, log);
    if (prg == NULL) {
        return -1;
    }

    /* disable TDE */
    resources_get_int("DriveTrueEmulation", &old_tde_state);
    if (old_tde_state != 0) {
        log_message(log, "Turning true drive emulation off.");
        resources_set_int("DriveTrueEmulation", 0);
    }

    do {
        result = -1;

        /* create empty image */
        if (vdrive_internal_create_format_disk_image(image_name, (char *)"AUTOSTART", disk_image_type) < 0) {
            log_error(log, "Error creating autostart disk image: %s", image_name);
            break;
        }

        /* attach disk image */
        if (file_system_attach_disk(drive, image_name) < 0) {
            log_error(log, "Could not attach disk image: %s", image_name);
            break;
        }

        /* get vdrive */
        vdrive = file_system_get_vdrive((unsigned int)drive);
        if (vdrive == NULL) {
            break;
        }

        /* get file name size */
        file_name_size = strlen((const char *)fh->name);
        if (file_name_size > 16) {
            file_name_size = 16;
        }

        /* open file on disk */
        if (vdrive_iec_open(vdrive, (const BYTE *)fh->name, file_name_size, secondary, NULL) != SERIAL_OK) {
            log_error(log, "Could not open file");
            break;
        }

        result2 = 0;
        /* write PRG data to file */
        for (i = -2; i < (int)prg->size; i++) {
            switch (i) {
            case -2: 
                data = (BYTE)prg->start_addr; 
                break;
            case -1: 
                data = (BYTE)(prg->start_addr >> 8); 
                break;
            default: 
                data = prg->data[i]; 
                break;
            }
            if (vdrive_iec_write(vdrive, data, secondary) != SERIAL_OK) {
                log_error(log, "Could not write file");
                result2 = -1;
                break;
            }
        }

        /* close file */
        if (vdrive_iec_close(vdrive, secondary) != SERIAL_OK) {
            log_error(log, "Could not close file");
            break;
        }
        result = result2;
    } while (0);

    /* free prg file */
    free_prg(prg);

    /* re-enable TDE */
    if (old_tde_state != 0) {
        log_message(log, "Turning true drive emulation on.");
        resources_set_int("DriveTrueEmulation", old_tde_state);
    }

    /* ready */
    return result;
}

int autostart_prg_perform_injection(log_t log)
{
    unsigned int i;
    WORD start, end;

    autostart_prg_t *prg = inject_prg;

    if (prg == NULL) {
        log_error(log, "Nothing to inject!");
        return -1;
    }

    log_message(log, "Injecting program data at $%04x (size $%04x)",
                prg->start_addr,
                (unsigned int)prg->size);

    /* store data in emu memory */
    for (i = 0; i < prg->size; i++) {
        mem_inject((WORD)(prg->start_addr + i), prg->data[i]);
    }

    /* now simulate a basic load */
    mem_get_basic_text(&start, &end);
    end = (WORD)(prg->start_addr + prg->size);
    mem_set_basic_text(start, end);

    /* clean up injected prog */
    free_prg(inject_prg);
    inject_prg = NULL;

    return 0;
}
