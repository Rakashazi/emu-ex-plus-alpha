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

#include "vice.h"

#include <stdio.h>

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
#include "vdrive-bam.h"


/* Is true drive emulation switched on?  */
static int drive_true_emulation;

/* Is drive sound emulation switched on?  */
int drive_sound_emulation;
/* volume of the drive sound */
int drive_sound_emulation_volume;

static int set_drive_true_emulation(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive;

    drive_true_emulation = val ? 1 : 0;

    machine_bus_status_truedrive_set((unsigned int)drive_true_emulation);

    if (val) {
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive = drive_context[dnr]->drive;
            if (drive->type != DRIVE_TYPE_NONE) {
                drive->enable = 1;
                if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
                    drivecpu65c02_reset_clk(drive_context[dnr]);
                } else {
                    drivecpu_reset_clk(drive_context[dnr]);
                }
            }
        }
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive_enable(drive_context[dnr]);
        }
    } else {
        for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
            drive = drive_context[dnr]->drive;
            drive_disable(drive_context[dnr]);
            if (drive->image != NULL) {
                vdrive_bam_reread_bam(dnr + 8);
            }
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
    if ((val < 0) || (val > 4000)) {
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
            drive_context[vice_ptr_to_int(param)]->drive->extend_image_policy = val;
            return 0;
        default:
            return -1;
    }
}

static int drive_resources_type(int val, void *param)
{
    unsigned int type, dnr;
    int busses;
    drive_t *drive, *drive0;
    char *rtc_device = NULL;

    dnr = vice_ptr_to_uint(param);
    drive = drive_context[dnr]->drive;

    type = (unsigned int)val;
    busses = iec_available_busses();

    /* if bus for drive type is not allowed, set to default value for bus */
    if (!drive_check_bus(type, busses)) {
        if (busses & IEC_BUS_IEC) {
            type = DRIVE_TYPE_1541;
        } else
        if (busses & IEC_BUS_IEEE) {
            type = DRIVE_TYPE_2031;
        } else {
            type = DRIVE_TYPE_NONE;
        }
    }

    if (is_drive0(dnr)) {
        if (drive_check_dual(type)) {
            int drive1 = mk_drive1(dnr);

            /* dual disk drives disable second emulated unit */
            log_warning(drive->log,
                        "Dual disk drive %d disables emulated drive %d", dnr, drive1);

            drive_resources_type(DRIVE_TYPE_NONE, int_to_void_ptr(drive1));
        }
    } else {
        drive0 = drive_context[mk_drive0(dnr)]->drive;
        if (drive0->enable && drive_check_dual(drive0->type)) {
            /* dual disk drives disable second emulated unit */
            log_warning(drive->log,
                        "Dual disk drive %d disables emulated drive %d", mk_drive0(dnr), dnr);

            type = DRIVE_TYPE_NONE;
        }
    }

    if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000) {
        if (drive->type != DRIVE_TYPE_2000 && drive->type != DRIVE_TYPE_4000) {
            rtc_device = lib_msprintf("FD%d", dnr + 8);
            drive->ds1216 = ds1216e_init(rtc_device);
            drive->ds1216->hours12 = 1;
            lib_free(rtc_device);
        }
    } else {
        if (drive->type == DRIVE_TYPE_2000 || drive->type == DRIVE_TYPE_4000) {
            if (drive->ds1216) {
                ds1216e_destroy(drive->ds1216, drive->rtc_save);
                drive->ds1216 = NULL;
            }
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
        case DRIVE_TYPE_2031:
        case DRIVE_TYPE_1001:
        case DRIVE_TYPE_2040:
        case DRIVE_TYPE_3040:
        case DRIVE_TYPE_4040:
        case DRIVE_TYPE_8050:
        case DRIVE_TYPE_8250:
            if (drive->type != type) {
                drive->current_half_track = 2 * 18;
                if ((type == DRIVE_TYPE_1001)
                    || (type == DRIVE_TYPE_8050)
                    || (type == DRIVE_TYPE_8250)) {
                    drive->current_half_track = 2 * 38;
                }
            }
            drive->type = type;
            if (drive_true_emulation) {
                drive->enable = 1;
                drive_enable(drive_context[dnr]);
                /* 1551 drive does not use the IEC bus */
                machine_bus_status_drivetype_set(dnr + 8, drive_check_bus(type,
                                                                          IEC_BUS_IEC));
            } else {
                drive_enable_update_ui(drive_context[dnr]);
            }
            drive_set_disk_drive_type(type, drive_context[dnr]);
            driverom_initialize_traps(drive);
            machine_drive_idling_method(dnr);
            return 0;
        case DRIVE_TYPE_NONE:
            drive->type = type;
            drive_disable(drive_context[dnr]);
            machine_bus_status_drivetype_set(dnr + 8, 0);
            return 0;
        default:
            return -1;
    }
}

static resource_int_t res_drive_type[] = {
    { NULL, 0, RES_EVENT_SAME, NULL,
      NULL, drive_resources_type, NULL },
    { NULL }
};


int drive_resources_type_init(unsigned int default_type)
{
    unsigned int dnr, type;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;
        if (dnr == 0) {
            type = default_type;
        } else {
            type = DRIVE_TYPE_NONE;
        }

        res_drive_type[0].name = lib_msprintf("Drive%iType", dnr + 8);
        res_drive_type[0].factory_value = (int)type;
        res_drive_type[0].value_ptr = (int *)&(drive->type);
        res_drive_type[0].param = uint_to_void_ptr(dnr);

        if (resources_register_int(res_drive_type) < 0) {
            return -1;
        }

        lib_free((char *)(res_drive_type[0].name));
    }

    return 0;
}

static int set_drive_idling_method(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive;

    dnr = vice_ptr_to_uint(param);
    drive = drive_context[dnr]->drive;

    /* FIXME: Maybe we should call `drive_cpu_execute()' here?  */
    switch (val) {
        case DRIVE_IDLE_SKIP_CYCLES:
        case DRIVE_IDLE_TRAP_IDLE:
        case DRIVE_IDLE_NO_IDLE:
            break;
        default:
            return -1;
    }

    drive->idling_method = val;

    if (!rom_loaded) {
        return 0;
    }

    driverom_initialize_traps(drive);
    return 0;
}

static int set_drive_rtc_save(int val, void *param)
{
    unsigned int dnr;
    drive_t *drive;

    dnr = vice_ptr_to_uint(param);
    drive = drive_context[dnr]->drive;

    drive->rtc_save = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DriveTrueEmulation", 1, RES_EVENT_STRICT, (resource_value_t)1,
      &drive_true_emulation, set_drive_true_emulation, NULL },
    { "DriveSoundEmulation", 0, RES_EVENT_NO, (resource_value_t)0,
      &drive_sound_emulation, set_drive_sound_emulation, NULL },
    { "DriveSoundEmulationVolume", 1000, RES_EVENT_NO, (resource_value_t)1000,
      &drive_sound_emulation_volume, set_drive_sound_emulation_volume, NULL },
    { NULL }
};

static resource_int_t res_drive[] = {
    { NULL, DRIVE_EXTEND_NEVER, RES_EVENT_SAME, NULL,
      NULL, set_drive_extend_image_policy, NULL },
    { NULL, DRIVE_IDLE_NO_IDLE, RES_EVENT_SAME, NULL,
      NULL, set_drive_idling_method, NULL },
    { NULL }
};

static resource_int_t res_drive_rtc[] = {
    { NULL, 0, RES_EVENT_NO, NULL,
      NULL, set_drive_rtc_save, NULL },
    { NULL }
};

int drive_resources_init(void)
{
    unsigned int dnr;
    drive_t *drive;
    int has_iec;

    switch (machine_class) {
        case VICE_MACHINE_NONE:
        case VICE_MACHINE_PET:
        case VICE_MACHINE_CBM5x0:
        case VICE_MACHINE_CBM6x0:
        case VICE_MACHINE_VSID:
            has_iec = 0;
            break;
        default:
            has_iec = 1;
    }

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        res_drive[0].name = lib_msprintf("Drive%iExtendImagePolicy", dnr + 8);
        res_drive[0].value_ptr = (int *)&(drive->extend_image_policy);
        res_drive[0].param = uint_to_void_ptr(dnr);
        res_drive[1].name = lib_msprintf("Drive%iIdleMethod", dnr + 8);
        res_drive[1].value_ptr = &(drive->idling_method);
        res_drive[1].param = uint_to_void_ptr(dnr);

        if (has_iec) {
            res_drive_rtc[0].name = lib_msprintf("Drive%iRTCSave", dnr + 8);
            res_drive_rtc[0].value_ptr = &(drive->rtc_save);
            res_drive_rtc[0].param = uint_to_void_ptr(dnr);
            if (resources_register_int(res_drive_rtc) < 0) {
                return -1;
            }
        }

        if (resources_register_int(res_drive) < 0) {
            return -1;
        }

        lib_free((char *)(res_drive[0].name));
        lib_free((char *)(res_drive[1].name));
        if (has_iec) {
            lib_free((char *)(res_drive_rtc[0].name));
        }
    }

    return machine_drive_resources_init()
           | resources_register_int(resources_int);
}

void drive_resources_shutdown(void)
{
    machine_drive_resources_shutdown();
}

#ifdef ANDROID_COMPILE
void loader_set_drive_true_emulation(int val)
{
    set_drive_true_emulation(val, 0);
}

int loader_get_drive_true_emulation()
{
    return drive_true_emulation;
}
#endif
