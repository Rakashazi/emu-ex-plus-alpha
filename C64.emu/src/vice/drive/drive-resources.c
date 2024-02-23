/*
 * drive-resources.c - Hardware-level disk drive emulation, resource module.
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

/* #define DEBUGDRIVE */

#include "vice.h"

#include <stdio.h>

#include "attach.h"
#include "drive-check.h"
#include "drive-resources.h"
#include "drive.h"
#include "drivecpu.h"
#include "drivecpu65c02.h"
#include "driverom.h"
#include "drivetypes.h"
#include "ds1216e.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "machine-bus.h"
#include "machine-drive.h"
#include "resources.h"
#include "vdrive.h"

#ifdef DEBUGDRIVE
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

/* Is drive sound emulation switched on?  */
int drive_sound_emulation;

/* volume of the drive sound */
int drive_sound_emulation_volume;

static int set_drive_true_emulation(int val, void *param)
{
    unsigned int dnr;
    unsigned int thistde = val ? 1 : 0;
    unsigned int thisdnr = vice_ptr_to_int(param);

    DBG(("set_drive_true_emulation unit %u enabled: %u", thisdnr + 8, thistde));

    /* always enable TDE on both units of a drive */
    diskunit_context[thisdnr]->drives[0]->true_emulation = thistde;
    diskunit_context[thisdnr]->drives[1]->true_emulation = thistde;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        machine_bus_status_truedrive_set(dnr + 8, diskunit_context[dnr]->drives[0]->true_emulation);
    }
    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        if (diskunit_context[dnr]->drives[0]->true_emulation) {
            diskunit_context_t *unit = diskunit_context[dnr];

            vdrive_flush(dnr + 8);
            if (unit->type != DRIVE_TYPE_NONE) {
                unit->enable = 1;
                /* reset drive CPU */
                if (unit->type == DRIVE_TYPE_2000 ||
                    unit->type == DRIVE_TYPE_4000 ||
                    unit->type == DRIVE_TYPE_CMDHD) {
                    drivecpu65c02_reset_clk(unit);
                } else {
                    drivecpu_reset_clk(unit);
                }
            }
            drive_enable(diskunit_context[dnr]);
        } else {
            drive_disable(diskunit_context[dnr]);
            vdrive_refresh(dnr + 8);
#if 0
            if (drive->image != NULL) {
                /* TODO: drive 1? */
                vdrive_bam_reread_bam(dnr + 8, 0);
            }
#endif
        }
    }
    return 0;
}

static int set_drive_sound_emulation(int val, void *param)
{
    drive_sound_emulation = val ? 1 : 0;

    return 0;
}

static int set_drive_sound_emulation_volume(int val, void *param)
{
    if ((val < 0) || (val > DRIVE_SOUND_VOLUME_MAX)) {
        return -1;
    }
    drive_sound_emulation_volume = val;
    return 0;
}

static int set_drive_extend_image_policy(int val, void *param)
{
    switch (val) {
        case DRIVE_EXTEND_NEVER:
        case DRIVE_EXTEND_ASK:
        case DRIVE_EXTEND_ACCESS:
            /* TODO: add resource for drive 1 */
            diskunit_context[vice_ptr_to_int(param)]->drives[0]->extend_image_policy = val;
            diskunit_context[vice_ptr_to_int(param)]->drives[1]->extend_image_policy = val;
            return 0;
        default:
            return -1;
    }
}

static int drive_resources_type(int val, void *param)
{
    unsigned int type, current;
    int dnr;
    int busses;
    diskunit_context_t *unit;
    drive_t *drive;
    char *rtc_device = NULL;

    dnr = vice_ptr_to_uint(param);
    unit = diskunit_context[dnr];
    drive = unit->drives[0];

    type = (unsigned int)val;
    busses = iec_available_busses();

    /* first of all, detach any disk images, so we don't end up with images
       attached to drives that do not support them */
    if (file_system_get_vdrive(dnr + 8) != NULL) {
        file_system_detach_disk(dnr + 8, 0);
        file_system_detach_disk(dnr + 8, 1);
    }

    /* if bus for drive type is not allowed, set to default value for bus */
    if (!drive_check_bus(type, busses)) {
        if (busses & IEC_BUS_IEC) {
            type = DRIVE_TYPE_1541;
        } else
        if (busses & IEC_BUS_IEEE) {
            type = DRIVE_TYPE_2031;
        } else
        if (busses & IEC_BUS_TCBM) {
            type = DRIVE_TYPE_1551;
        } else {
            type = DRIVE_TYPE_NONE;
        }
    }

    switch (type) {
        case DRIVE_TYPE_1540:
        case DRIVE_TYPE_1541:
        case DRIVE_TYPE_1541II:
        case DRIVE_TYPE_1551:
        case DRIVE_TYPE_1570:
        case DRIVE_TYPE_1571:
        case DRIVE_TYPE_1571CR:
        case DRIVE_TYPE_1581:
        case DRIVE_TYPE_2000:
        case DRIVE_TYPE_4000:
        case DRIVE_TYPE_CMDHD:
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
        case DRIVE_TYPE_9000:
            current = unit->type;
            /* make sure the switch is successful before moving on */
            if (drive_set_disk_drive_type(type, diskunit_context[dnr]) < 0) {
                return -1;
            }
            /* allocate or unallocate rtc module for FD2K/4K */
            if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
                if (current != DRIVE_TYPE_2000 && current != DRIVE_TYPE_4000) {
                    rtc_device = lib_msprintf("FD%d", dnr + 8);
                    unit->ds1216 = ds1216e_init(rtc_device);
                    unit->ds1216->hours12 = 1;
                    lib_free(rtc_device);
                }
            } else {
                if (current == DRIVE_TYPE_2000 || current == DRIVE_TYPE_4000) {
                    if (unit->ds1216) {
                        ds1216e_destroy(unit->ds1216, unit->rtc_save);
                        unit->ds1216 = NULL;
                    }
                }
            }
            if (current != type) {
                /* the drives that support half tracks are all single drives */
                drive->current_half_track = 2 * 18;
                if ((type == DRIVE_TYPE_1001)
                    || (type == DRIVE_TYPE_8050)
                    || (type == DRIVE_TYPE_8250)) {
                    drive->current_half_track = 2 * 38;
                } else if (type == DRIVE_TYPE_9000) {
                    drive->current_half_track = 2 * 76;
                }
            }
            if (drive->true_emulation) {
                unit->enable = 1;
                drive_enable(diskunit_context[dnr]);
                /* 1551 drive does not use the IEC bus */
                machine_bus_status_drivetype_set(dnr + 8, drive_check_bus(type,
                                                                          IEC_BUS_IEC));
            }
            drive_enable_update_ui(diskunit_context[dnr]);
            driverom_initialize_traps(diskunit_context[dnr]);
            machine_drive_idling_method(dnr);
            return 0;
        case DRIVE_TYPE_NONE:
            unit->type = type;
            drive_disable(diskunit_context[dnr]);
            machine_bus_status_drivetype_set(dnr + 8, 0);
            return 0;
        default:
            return -1;
    }
}

static resource_int_t res_drive_type[] = {
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, drive_resources_type, NULL },
    RESOURCE_INT_LIST_END
};

int drive_resources_type_init(unsigned int default_type)
{
    unsigned int type;
    int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (dnr == 0) {
            type = default_type;
        } else {
            type = DRIVE_TYPE_NONE;
        }

        res_drive_type[0].name = lib_msprintf("Drive%iType", dnr + 8);
        res_drive_type[0].factory_value = (int)type;
        res_drive_type[0].value_ptr = (int *)&(unit->type);
        res_drive_type[0].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive_type) < 0) {
            return -1;
        }

        lib_free(res_drive_type[0].name);
    }

    return 0;
}

static int set_drive_idling_method(int val, void *param)
{
    unsigned int dnr = vice_ptr_to_uint(param);
    diskunit_context_t *unit = diskunit_context[dnr];

    /* FIXME: Maybe we should call `drive_cpu_execute()' here?  */
    switch (val) {
        case DRIVE_IDLE_SKIP_CYCLES:
        case DRIVE_IDLE_TRAP_IDLE:
        case DRIVE_IDLE_NO_IDLE:
            break;
        default:
            return -1;
    }

    unit->idling_method = val;

    if (!rom_loaded) {
        return 0;
    }

    driverom_initialize_traps(unit);

    return 0;
}

static int set_drive_rpm(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive, *drive1;

    if ((val < 0) || (val > DRIVE_RPM_MAX)) {
        return -1;
    }

    dnr = vice_ptr_to_uint(param);
    drive = diskunit_context[dnr]->drives[0];
    drive1 = diskunit_context[dnr]->drives[1];

    drive->rpm = val;
    drive1->rpm = val;

    return 0;
}

static int set_drive_wobble_frequency(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive, *drive1;

    if ((val < 0) || (val > DRIVE_WOBBLE_FREQ_MAX)) {
        return -1;
    }

    dnr = vice_ptr_to_uint(param);
    drive = diskunit_context[dnr]->drives[0];
    drive1 = diskunit_context[dnr]->drives[1];

    drive->wobble_frequency = val;
    drive1->wobble_frequency = val;
    return 0;
}

static int set_drive_wobble_amplitude(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive, *drive1;

    if ((val < 0) || (val > DRIVE_WOBBLE_AMPLITUDE_MAX)) {
        return -1;
    }

    dnr = vice_ptr_to_uint(param);
    drive = diskunit_context[dnr]->drives[0];
    drive1 = diskunit_context[dnr]->drives[1];

    drive->wobble_amplitude = val;
    drive1->wobble_amplitude = val;
    return 0;
}

static int set_drive_rtc_save(int val, void *param)
{
    unsigned int dnr;
    diskunit_context_t *unit;

    dnr = vice_ptr_to_uint(param);
    unit = diskunit_context[dnr];

    unit->rtc_save = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DriveSoundEmulation", 0, RES_EVENT_NO, (resource_value_t)0,
      &drive_sound_emulation, set_drive_sound_emulation, NULL },
    { "DriveSoundEmulationVolume", 1000, RES_EVENT_NO, (resource_value_t)1000,
      &drive_sound_emulation_volume, set_drive_sound_emulation_volume, NULL },
    RESOURCE_INT_LIST_END
};

static resource_int_t res_drive[] = {
    { NULL, DRIVE_EXTEND_ASK, RES_EVENT_SAME, NULL,
      NULL, set_drive_extend_image_policy, NULL },
    { NULL, DRIVE_IDLE_NO_IDLE, RES_EVENT_SAME, NULL,
      NULL, set_drive_idling_method, NULL },
    { NULL, DRIVE_RPM_DEFAULT, RES_EVENT_SAME, NULL,
      NULL, set_drive_rpm, NULL },
    { NULL, DRIVE_WOBBLE_FREQ_DEFAULT, RES_EVENT_SAME, NULL,
      NULL, set_drive_wobble_frequency, NULL },
    { NULL, DRIVE_WOBBLE_AMPLITUDE_DEFAULT, RES_EVENT_SAME, NULL,
      NULL, set_drive_wobble_amplitude, NULL },
    { NULL, 1, RES_EVENT_STRICT, NULL,
      NULL, set_drive_true_emulation, NULL },
    RESOURCE_INT_LIST_END
};

static resource_int_t res_drive_rtc[] = {
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_drive_rtc_save, NULL },
    RESOURCE_INT_LIST_END
};

int drive_resources_init(void)
{
    int dnr;
    int has_iec;
    int i;

    switch (machine_class) {
        case VICE_MACHINE_PET:
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
        case VICE_MACHINE_VSID:
            has_iec = 0;
            break;
        default:
            has_iec = 1;
    }

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];
        drive_t *drive0 = unit->drives[0];

        /* TODO: add resources for drive 1 */
        res_drive[0].name = lib_msprintf("Drive%iExtendImagePolicy", dnr + 8);
        res_drive[0].value_ptr = (int *)&(drive0->extend_image_policy);
        res_drive[0].param = uint_to_void_ptr(dnr);
        res_drive[1].name = lib_msprintf("Drive%iIdleMethod", dnr + 8);
        res_drive[1].value_ptr = &(unit->idling_method);
        res_drive[1].param = uint_to_void_ptr(dnr);
        res_drive[2].name = lib_msprintf("Drive%iRPM", dnr + 8);
        res_drive[2].value_ptr = &(drive0->rpm);
        res_drive[2].param = uint_to_void_ptr(dnr);
        res_drive[3].name = lib_msprintf("Drive%iWobbleFrequency", dnr + 8);
        res_drive[3].value_ptr = &(drive0->wobble_frequency);
        res_drive[3].param = uint_to_void_ptr(dnr);
        res_drive[4].name = lib_msprintf("Drive%iWobbleAmplitude", dnr + 8);
        res_drive[4].value_ptr = &(drive0->wobble_amplitude);
        res_drive[4].param = uint_to_void_ptr(dnr);
        res_drive[5].name = lib_msprintf("Drive%iTrueEmulation", dnr + 8);
        res_drive[5].value_ptr = &(drive0->true_emulation);
        res_drive[5].param = uint_to_void_ptr(dnr);

        if (has_iec) {
            res_drive_rtc[0].name = lib_msprintf("Drive%iRTCSave", dnr + 8);
            res_drive_rtc[0].value_ptr = &(unit->rtc_save);
            res_drive_rtc[0].param = uint_to_void_ptr(dnr);
            if (resources_register_int(res_drive_rtc) < 0) {
                return -1;
            }
        }

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        for (i = 0; i <= 5; i++) {
            lib_free(res_drive[i].name);
        }
        if (has_iec) {
            lib_free(res_drive_rtc[0].name);
        }
    }

    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    /* make sure machine_drive_resources_init() is called last here, as that
       will also initialize the default drive type and if it fails to do that
       because other drive related resources are not initialized yet then we
       end up with a non functioning drive at startup */
    return machine_drive_resources_init();
}

void drive_resources_shutdown(void)
{
    machine_drive_resources_shutdown();
}
