/*
 * attach.c - File system attach management.
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

/* #define DEBUG_ATTACH */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attach.h"
#include "cmdline.h"
#include "diskimage.h"
#include "driveimage.h"
#include "fsdevice.h"
#include "fliplist.h"
#include "lib.h"
#include "log.h"
#include "machine-bus.h"
#include "machine-drive.h"
#include "network.h"
#include "resources.h"
#include "serial.h"
#include "types.h"
#include "uiapi.h"
#include "vdrive-bam.h"
#include "vdrive-iec.h"
#include "vdrive.h"
#include "vice-event.h"
#include "p64.h"
#include "arch/shared/archdep_real_path.h"

#ifdef DEBUG_ATTACH
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

typedef struct {
    serial_t *serial;
    vdrive_t *vdrive;
} file_system_t;

#define NUM_DISK_UNITS          4
#define NUM_DRIVES              2

static file_system_t file_system[NUM_DISK_UNITS];

static log_t attach_log = LOG_DEFAULT;

static int attach_device_readonly_enabled[NUM_DISK_UNITS][NUM_DRIVES];
static int file_system_device_enabled[NUM_DISK_UNITS] = { -1, -1, -1, -1 };

static int set_attach_device_readonly(int val, void *param);
static int set_file_system_device(int val, void *param);

static void vdrive_detach_disk_image_and_free(vdrive_t *vdrive,
                                              unsigned int unit,
                                              unsigned int drive);
static void detach_disk_image_and_free(disk_image_t *image, vdrive_t *vdrive,
                                       unsigned int unit, unsigned int drive);
static void detach_disk_image(disk_image_t *image, vdrive_t *vdrive,
                              unsigned int unit, unsigned int drive);
static int attach_disk_image(disk_image_t *oldimage, vdrive_t *vdrive,
                             const char *filename, unsigned int unit,
                             unsigned int drive,
                             int devicetype);

#define UNIT_AND_DRIVE(unit, drive)     ((unit << 8) | drive)
#define GET_UNIT(du)                    ((du >> 8) & 0xFF)
#define GET_DRIVE(du)                   (du & 0xFF)

static const resource_int_t resources_int[] = {
    { "AttachDevice8Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[0][0],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(8,0) },
    { "AttachDevice9Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[1][0],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(9,0) },
    { "AttachDevice10Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[2][0],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(10,0) },
    { "AttachDevice11Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[3][0],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(11,0) },
    { "AttachDevice8d1Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[0][1],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(8,1) },
    { "AttachDevice9d1Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[1][1],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(9,1) },
    { "AttachDevice10d1Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[2][1],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(10,1) },
    { "AttachDevice11d1Readonly", 0, RES_EVENT_SAME, NULL,
      &attach_device_readonly_enabled[3][1],
      set_attach_device_readonly, (void *)UNIT_AND_DRIVE(11,1) },
    { "FileSystemDevice8", ATTACH_DEVICE_FS,
      RES_EVENT_STRICT, (resource_value_t)ATTACH_DEVICE_FS,
      &file_system_device_enabled[0],
      set_file_system_device, (void *)8 },
    { "FileSystemDevice9", ATTACH_DEVICE_FS,
      RES_EVENT_STRICT, (resource_value_t)ATTACH_DEVICE_FS,
      &file_system_device_enabled[1],
      set_file_system_device, (void *)9 },
    { "FileSystemDevice10", ATTACH_DEVICE_FS,
      RES_EVENT_STRICT, (resource_value_t)ATTACH_DEVICE_FS,
      &file_system_device_enabled[2],
      set_file_system_device, (void *)10 },
    { "FileSystemDevice11", ATTACH_DEVICE_FS,
      RES_EVENT_STRICT, (resource_value_t)ATTACH_DEVICE_FS,
      &file_system_device_enabled[3],
      set_file_system_device, (void *)11 },
    RESOURCE_INT_LIST_END
};

int file_system_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-device8", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FileSystemDevice8", (void *)ATTACH_DEVICE_FS,
      "<Type>", "Set device type for device #8 (0: None, 1: Filesystem, 2: OpenCBM)" },
    { "-device9", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FileSystemDevice9", (void *)ATTACH_DEVICE_FS,
      "<Type>", "Set device type for device #9 (0: None, 1: Filesystem, 2: OpenCBM)" },
    { "-device10", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FileSystemDevice10", (void *)ATTACH_DEVICE_FS,
      "<Type>", "Set device type for device #10 (0: None, 1: Filesystem, 2: OpenCBM)" },
    { "-device11", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "FileSystemDevice11", (void *)ATTACH_DEVICE_FS,
      "<Type>", "Set device type for device #11 (0: None, 1: Filesystem, 2: OpenCBM)" },
    { "-attach8ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice8Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #8:0 read only" },
    { "-attach8rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice8Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #8:0 read write (if possible)" },
    { "-attach9ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice9Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #9:0 read only" },
    { "-attach9rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice9Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #9:0 read write (if possible)" },
    { "-attach10ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice10Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #10:0 read only" },
    { "-attach10rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice10Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #10:0 read write (if possible)" },
    { "-attach11ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice11Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #11:0 read only" },
    { "-attach11rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice11Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #11:0 read write (if possible)" },
    { "-attach8d1ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice8d1Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #8:1 read only" },
    { "-attach8d1rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice8d1Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #8:1 read write (if possible)" },
    { "-attach9d1ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice9d1Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #9:1 read only" },
    { "-attach9d1rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice9d1Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #9:1 read write (if possible)" },
    { "-attach10d1ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice10d1Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #10:1 read only" },
    { "-attach10d1rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice10d1Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #10:1 read write (if possible)" },
    { "-attach11d1ro", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice11d1Readonly", (resource_value_t)1,
      NULL, "Attach disk image for drive #11:1 read only" },
    { "-attach11d1rw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "AttachDevice11d1Readonly", (resource_value_t)0,
      NULL, "Attach disk image for drive #11:1 read write (if possible)" },
    CMDLINE_LIST_END
};

int file_system_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

/*
 * Setting the serial hooks is a per-unit thing, it does not make
 * sense (or isn't even possible) to differ between drive 0 and 1.
 *
 * NOTE: it requires that the corresponding vdrive(s) are not NULL.
 *
 * It may seem that int fs is a boolean, but in one location it is called
 * with the value from file_system_device_enabled[i], so in principle
 * it can receive all values ATTACH_DEVICE_NONE, _FS, _REAL and _VIRT. FIXME!
 */
static int file_system_set_serial_hooks(unsigned int unit, int fs)
{
    DBG(("file_system_set_serial_hooks dev %u: %s", unit, (fs == ATTACH_DEVICE_NONE) ? "vdrive" : "fsdevice"));

    if (fs == ATTACH_DEVICE_NONE) {
        if (vdrive_iec_attach(unit, "CBM Disk Drive")) {
            log_error(attach_log,
                      "Could not initialize vdrive emulation for device #%u.",
                      unit);
            return -1;
        }

        return 0;
    } else {
        unsigned int drive = 0;
        int rc = 0;

        for (drive = 0; drive < NUM_DRIVES; drive++) {
            if (fsdevice_attach(unit, drive, "FS Drive")) {
                log_error(attach_log,
                          "Could not initialize FS drive for device #%u.",
                          unit);
                rc = -1;
            }
        }

        return rc;
    }
}

void file_system_init(void)
{
    unsigned int i;

    attach_log = log_open("Attach");

    for (i = 0; i < 8; i++) {
        serial_device_type_set(SERIAL_DEVICE_VIRT, i);
    }

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        file_system[i].serial = serial_device_get(i + 8);
        file_system[i].vdrive = lib_calloc(1, sizeof(vdrive_t));

        switch (file_system_device_enabled[i]) {
            case ATTACH_DEVICE_NONE:
                vdrive_device_setup(file_system[i].vdrive, i + 8);
                serial_device_type_set(SERIAL_DEVICE_NONE, i + 8);
                break;
            case ATTACH_DEVICE_FS:
                vdrive_device_setup(file_system[i].vdrive, i + 8);
                serial_device_type_set(SERIAL_DEVICE_FS, i + 8);
                break;
            case ATTACH_DEVICE_REAL:
                vdrive_device_setup(file_system[i].vdrive, i + 8);
                serial_device_type_set(SERIAL_DEVICE_REAL, i + 8);
                break;
        }
        file_system_set_serial_hooks(i + 8, file_system_device_enabled[i]);
    }
}

void file_system_shutdown(void)
{
    unsigned int i;

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        vdrive_device_shutdown(file_system[i].vdrive);
        lib_free(file_system[i].vdrive);
        machine_bus_device_detach(i + 8); /* free memory allocated by file_system_set_serial_hooks() */
    }
}

struct vdrive_s *file_system_get_vdrive(unsigned int unit)
{
    if (unit < 8 || unit >= 8 + NUM_DISK_UNITS) {
        log_error(attach_log, "Wrong unit %u for vdrive", unit);
        return NULL;
    }

    return file_system[unit - 8].vdrive;
}

struct disk_image_s *file_system_get_image(unsigned int unit, unsigned int drive)
{
    return vdrive_get_image(file_system_get_vdrive(unit), drive);
}

const char *file_system_get_disk_name(unsigned int unit, unsigned int drive)
{
    vdrive_t *vdrive;
    disk_image_t *image;

    vdrive = file_system_get_vdrive(unit);
    image = vdrive_get_image(vdrive, drive);

    if (image == NULL) {
        return NULL;
    }
    if (image->device != DISK_IMAGE_DEVICE_FS) {
        return NULL;
    }

    return disk_image_fsimage_name_get(image);
}

int file_system_bam_get_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    return vdrive_bam_get_disk_id(unit, drive, id);
}

int file_system_bam_set_disk_id(unsigned int unit, unsigned int drive, uint8_t *id)
{
    return vdrive_bam_set_disk_id(unit, drive, id);
}

/* ------------------------------------------------------------------------- */

static int set_attach_device_readonly(int value, void *param)
{
    unsigned int unit = GET_UNIT(vice_ptr_to_uint(param));
    unsigned int drive = GET_DRIVE(vice_ptr_to_uint(param));
    const char *old_filename;
    char *new_filename;
    int rc;
    int val = value ? 1 : 0;

    /* Do nothing if resource is unchanged. */
    if (attach_device_readonly_enabled[unit - 8][drive] == val) {
        return 0;
    }

    old_filename = file_system_get_disk_name(unit, drive);

    /* If no disk is attached, just changed the resource.  */
    if (old_filename == NULL) {
        attach_device_readonly_enabled[unit - 8][drive] = val;
        return 0;
    }

    /* Old filename will go away after the image is detached.  */
    new_filename = lib_strdup(old_filename);

    file_system_detach_disk(unit, drive);
    attach_device_readonly_enabled[unit - 8][drive] = val;

    rc = file_system_attach_disk(unit, drive, new_filename);

    lib_free(new_filename);

    return rc;
}

/* ------------------------------------------------------------------------- */

#if 0
static int vdrive_device_setup_if_no_image(vdrive_t *vdrive, unsigned int unit, unsigned int drive)
{
    if (vdrive != NULL && vdrive_get_image(vdrive, drive) == NULL) {
        return vdrive_device_setup(vdrive, unit, drive);
    }

    return 0;
}
#endif

/* ------------------------------------------------------------------------- */

static int set_file_system_device(int val, void *param)
{
    vdrive_t *vdrive;
    unsigned int unit = vice_ptr_to_uint(param);
    unsigned int drive;
    unsigned int idx;
    int old_device_enabled, new_device_enabled;

    if ((unit < 8) || (unit >= 8 + NUM_DISK_UNITS)) {
        DBG(("set_file_system_device invalid dev #%u", unit));
        return -1;
    }
    idx = unit - 8;
    old_device_enabled = file_system_device_enabled[idx];
    new_device_enabled = val;

    DBG(("set_file_system_device dev #%u old dev:%d new dev:%d", unit, old_device_enabled, new_device_enabled));

    if (old_device_enabled == new_device_enabled) {
        return 0;
    }

    file_system_device_enabled[idx] = new_device_enabled;

    vdrive = file_system_get_vdrive(unit);

   if (vdrive == NULL) {
        /* file_system_set_serial_hooks() requires non-NULL... */
        DBG(("set_file_system_device: Too early in initialization; unit %u: vdrive is NULL", unit));
        return 0;
    }

    if (old_device_enabled == ATTACH_DEVICE_REAL) {
        DBG(("set_file_system_device: old == ATTACH_DEVICE_REAL, serial_realdevice_disable()"));
        serial_realdevice_disable();
    }

    if (new_device_enabled == ATTACH_DEVICE_REAL) {
        DBG(("set_file_system_device: new == ATTACH_DEVICE_REAL, serial_realdevice_enable()"));
        if (serial_realdevice_enable() < 0) {
            log_warning(attach_log, "Falling back to fs device.");
            return set_file_system_device(ATTACH_DEVICE_FS, param);
        }
    }

    /*
     * FIXME: Note about ATTACH_DEVICE_FS and ATTACH_DEVICE_VIRT:
     * Attaching a disk image also uses _FS even though you would expect _VIRT.
     * The value _VIRT seems to be unused in practice.
     * One would expect _FS for the fsdevice, and _VIRT for vdrive images.
     */
    switch (new_device_enabled) {
        case ATTACH_DEVICE_NONE:
            DBG(("set_file_system_device: new == ATTACH_DEVICE_NONE"));
#if 0
            for (drive = 0; drive < NUM_DRIVES; drive++) {
                vdrive_device_setup_if_no_image(vdrive, unit, drive);
            }
#endif
            serial_device_type_set(SERIAL_DEVICE_NONE, unit);
            file_system_set_serial_hooks(unit, ATTACH_DEVICE_NONE);
            break;
        case ATTACH_DEVICE_VIRT:
            DBG(("set_file_system_device: new == ATTACH_DEVICE_VIRT"));
#if 0
            for (drive = 0; drive < NUM_DRIVES; drive++) {
                vdrive_device_setup_if_no_image(vdrive, unit, drive);
            }
#endif
            serial_device_type_set(SERIAL_DEVICE_VIRT, unit);
            file_system_set_serial_hooks(unit, ATTACH_DEVICE_NONE);
            break;
        case ATTACH_DEVICE_FS:
            DBG(("set_file_system_device: new == ATTACH_DEVICE_FS"));
            for (drive = 0; drive < NUM_DRIVES; drive++) {
                vdrive_detach_disk_image_and_free(vdrive, unit, drive);
                ui_display_drive_current_image(idx, drive, "");
#if 0
                vdrive_device_setup_if_no_image(vdrive, unit, drive);
#endif
            }
            serial_device_type_set(SERIAL_DEVICE_FS, unit);
            file_system_set_serial_hooks(unit, ATTACH_DEVICE_FS);
            break;
#ifdef HAVE_REALDEVICE
        case ATTACH_DEVICE_REAL:
            DBG(("set_file_system_device: new == ATTACH_DEVICE_REAL"));
            for (drive = 0; drive < NUM_DRIVES; drive++) {
                vdrive_detach_disk_image_and_free(vdrive, unit, drive);
                ui_display_drive_current_image(idx, drive, "");
#if 0
                vdrive_device_setup(vdrive, unit, drive);
#endif
            }
            serial_device_type_set(SERIAL_DEVICE_REAL, unit);
            break;
#endif
        default:
            return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

static void detach_disk_image(disk_image_t *image, vdrive_t *vdrive,
                              unsigned int unit, unsigned int drive)
{
    switch (unit) {
        case 8:     /* fall through */
        case 9:     /* fall through */
        case 10:    /* fall through */
        case 11:
#if 0
            /*
             * TODO: This should not need unit and drive parameters:
             * remembered in vdrive. For now check the consistency.
             */
            if (vdrive->drive != drive || vdrive->unit != unit) {
                log_error(attach_log, "**** detach_disk_image vdrive %u:%u != %u:%u",
                          vdrive->unit, vdrive->drive, unit, drive);
            }
#endif
            machine_drive_image_detach(image, unit, drive);
            drive_image_detach(image, unit, drive);
            vdrive_detach_image(image, unit, drive, vdrive);
            break;
    }
    disk_image_close(image);

#if 0
    if (image != NULL) {
        P64ImageDestroy((PP64Image)image->p64);
        lib_free(image->p64);
    }
#endif
    disk_image_media_destroy(image);
}

static void vdrive_detach_disk_image_and_free(vdrive_t *vdrive,
                                              unsigned int unit,
                                              unsigned int drive)
{
    disk_image_t *image;

    image = vdrive_get_image(vdrive, drive);
    if (image != NULL) {
        detach_disk_image_and_free(image, vdrive, unit, drive);
    }
}

static void detach_disk_image_and_free(disk_image_t *image, vdrive_t *vdrive,
                                       unsigned int unit, unsigned int drive)
{
    disk_image_t *oldimg;

    oldimg = vdrive_get_image(vdrive, drive);

    if (!oldimg) {
        return;
    }

    detach_disk_image(image, vdrive, unit, drive);

    if ((image != NULL) && (image == oldimg)) {
        disk_image_destroy(image);
    }
}

static int attach_disk_image(disk_image_t *oldimage, vdrive_t *vdrive,
                             const char *filename, unsigned int unit,
                             unsigned int drive, int devicetype)
{
    disk_image_t *image;
    disk_image_t new_image;
    int err = -1;
    int test_unit;

    if (filename == NULL) {
        log_error(attach_log, "No name, cannot attach floppy image.");
        return -1;
    }

    /* Make sure that we aren't attaching a disk image that is already
     * attached */
    for (test_unit = 8; test_unit < 8 + NUM_DISK_UNITS; ++test_unit) {
        int test_drive;
        for (test_drive = 0; test_drive < NUM_DRIVES; ++test_drive) {
            /* It's OK to replace ourselves with the same disk */
            if (unit != test_unit || drive != test_drive) {
                const char *test_name = file_system_get_disk_name(test_unit, test_drive);
                if (test_name && archdep_real_path_equal(test_name, filename)) {
                    log_error(attach_log, "`%s' is already mounted on drive %d:%d", filename, test_unit, test_drive);
                    return -1;
                }
            }            
        }
    }

    new_image.gcr = NULL;
    new_image.p64 = lib_calloc(1, sizeof(TP64Image));
    new_image.read_only = (unsigned int)attach_device_readonly_enabled[unit - 8][drive];

    switch (devicetype) {
        case ATTACH_DEVICE_NONE:
        case ATTACH_DEVICE_VIRT:
        case ATTACH_DEVICE_FS:
            new_image.device = DISK_IMAGE_DEVICE_FS;
            break;
    }

    disk_image_media_create(&new_image);

    switch (devicetype) {
        case ATTACH_DEVICE_NONE:
        case ATTACH_DEVICE_VIRT:
        case ATTACH_DEVICE_FS:
            disk_image_fsimage_name_set(&new_image, filename);
            break;
    }

    if (disk_image_open(&new_image) < 0) {
        P64ImageDestroy((PP64Image) new_image.p64);
        lib_free(new_image.p64);
        disk_image_media_destroy(&new_image);
        return -1;
    }

    detach_disk_image_and_free(oldimage, vdrive, unit, drive);

    image = disk_image_create();

    memcpy(image, &new_image, sizeof(disk_image_t));
    /* free the P64 stuff, fixes the leak in src/attach.c, reported when
     * using --enable-debug */
    lib_free(new_image.p64);

    switch (unit) {
        case 8:
        case 9:
        case 10:
        case 11:
            /* "wired OR". If any of the three succeeds, err becomes 0 */
            err = drive_image_attach(image, unit, drive);
            err &= vdrive_attach_image(image, unit, drive, vdrive);
            err &= machine_drive_image_attach(image, unit, drive);
            break;
    }
    if (err) {
        disk_image_close(image);
#if 0
        P64ImageDestroy((PP64Image)image->p64);
        lib_free(image->p64);
#endif
        disk_image_media_destroy(image);
        disk_image_destroy(image);
    }
    return err;
}

/* ------------------------------------------------------------------------- */

static int file_system_attach_disk_internal(unsigned int unit, unsigned int drive,
                                            const char *filename)
{
    vdrive_t *vdrive;
    disk_image_t *image;

    vdrive = file_system_get_vdrive(unit);
    image = vdrive_get_image(vdrive, drive);
    serial_device_type_set(SERIAL_DEVICE_VIRT, unit);

    if (attach_disk_image(image, vdrive, filename, unit, drive,
                          file_system_device_enabled[unit - 8]) < 0) {
        return -1;
    } else {
        file_system_set_serial_hooks(unit, ATTACH_DEVICE_NONE);
        fliplist_set_current(unit, filename);
        ui_display_drive_current_image(unit - 8, drive, filename);
    }

    image = vdrive_get_image(vdrive, drive);
    if (image) {
        event_record_attach_image(unit, drive, filename, image->read_only);
    }

    return 0;
}

int file_system_attach_disk(unsigned int unit, unsigned int drive, const char *filename)
{
    if (event_playback_active()) {
        return -1;
    }

    /* TODO: drive 1? */
    if (network_connected() && drive == 0) {
        network_attach_image(unit, filename);
        return 0;
    }

    return file_system_attach_disk_internal(unit, drive, filename);
}

static void file_system_detach_disk_single(unsigned int unit, unsigned int drive)
{
    vdrive_t *vdrive;
    disk_image_t *image;

    vdrive = file_system_get_vdrive(unit);
    image = vdrive_get_image(vdrive, drive);
    if (image) {
        detach_disk_image_and_free(image, vdrive, unit, drive);
        ui_display_drive_current_image(unit - 8, drive, "");
    }
}

static void file_system_detach_disk_internal(unsigned int unit, unsigned int drive)
{
    char event_data[2];

    if ((unit >= 8) && (unit < 8 + NUM_DISK_UNITS)) {
        file_system_detach_disk_single(unit, drive);
        file_system_set_serial_hooks(unit, ATTACH_DEVICE_FS);
    } else {
        log_error(attach_log, "Cannot detach unit %u drive %u.", unit, drive);
    }

    /* TODO: drive 1 for EVENT_ATTACHDISK */
    event_data[0] = (char)unit;
    event_data[1] = 0;

    event_record(EVENT_ATTACHDISK, (void *)event_data, 2);
}

void file_system_detach_disk(unsigned int unit, unsigned int drive)
{
    char event_data[2];

    if (event_playback_active()) {
        return;
    }

    event_data[0] = (char)unit;
    event_data[1] = 0;

    if (network_connected()) {
        network_event_record(EVENT_ATTACHDISK, (void *)event_data, 2);
        return;
    }

    file_system_detach_disk_internal(unit, drive);
}

void file_system_detach_disk_shutdown(void)
{
    vdrive_t *vdrive;
    unsigned int i, j;
    disk_image_t *image;

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        if (file_system_device_enabled[i] == ATTACH_DEVICE_REAL) {
            serial_realdevice_disable();
        } else {
            vdrive = file_system_get_vdrive(i + 8);
            for (j = 0; j < NUM_DRIVES; j++) {
                image = vdrive_get_image(vdrive, j);
                if (image) {
                    detach_disk_image_and_free(image, vdrive, i + 8, j);
                }
            }
        }
    }
}

void file_system_event_playback(unsigned int unit, unsigned int drive, const char *filename)
{
    if (filename == NULL || filename[0] == 0) {
        file_system_detach_disk_internal(unit, drive);
    } else {
        file_system_attach_disk_internal(unit, drive, filename);
    }
}
