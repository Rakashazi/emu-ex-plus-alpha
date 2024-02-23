/*
 * drive.c - Hardware-level disk drive emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Teemu Rantanen <tvr@cs.hut.fi>
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

/* TODO:
        - more accurate emulation of disk rotation.
        - different speeds within one track.
        - check for byte ready *within* `BVC', `BVS' and `PHP'.
        - serial bus handling might be faster.  */

/* #define DEBUG_DRIVE */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "attach.h"
#include "archdep.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "drive-check.h"
#include "drive.h"
#include "drivecpu.h"
#include "drivecpu65c02.h"
#include "driveimage.h"
#include "drivesync.h"
#include "driverom.h"
#include "drivetypes.h"
#include "gcr.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "lib.h"
#include "log.h"
#include "machine-drive.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "rotation.h"
#include "sound.h"
#include "types.h"
#include "uiapi.h"
#include "ds1216e.h"
#include "drive-sound.h"
#include "p64.h"
#include "monitor.h"
#include "monitor_network.h"
#include "monitor_binary.h"
#include "vsync.h"

#ifdef DEBUG_DRIVE
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

static int drive_init_was_called = 0;

diskunit_context_t *diskunit_context[NUM_DISK_UNITS];

/* Generic drive logging goes here.  */
static log_t drive_log = LOG_ERR;

/* If nonzero, at least one vaild drive ROM has already been loaded.  */
int rom_loaded = 0;

/* ------------------------------------------------------------------------- */

static int drive_led_color[NUM_DISK_UNITS];
static bool is_jammed[NUM_DISK_UNITS] = { false, false, false, false };
static char *jam_reason[NUM_DISK_UNITS] = { NULL, NULL, NULL, NULL };
static int jam_action = MACHINE_JAM_ACTION_DIALOG;

/* ------------------------------------------------------------------------- */

void drive_set_disk_memory(uint8_t *id, unsigned int track, unsigned int sector,
                           struct diskunit_context_s *unit)
{
    if (unit->type == DRIVE_TYPE_1540
        || unit->type == DRIVE_TYPE_1541
        || unit->type == DRIVE_TYPE_1541II
        || unit->type == DRIVE_TYPE_1570
        || unit->type == DRIVE_TYPE_1571
        || unit->type == DRIVE_TYPE_1571CR) {
        unit->drive_ram[0x12] = id[0];
        unit->drive_ram[0x13] = id[1];
        unit->drive_ram[0x16] = id[0];
        unit->drive_ram[0x17] = id[1];
        unit->drive_ram[0x18] = track;
        unit->drive_ram[0x19] = sector;
        unit->drive_ram[0x22] = track;
    }
}

void drive_set_last_read(unsigned int track, unsigned int sector, uint8_t *buffer,
                         struct diskunit_context_s *unit)
{
    drive_t *drive;
    int side = 0;

    drive = unit->drives[0];

    /* TODO: drive 1 ? */
    drive_gcr_data_writeback(drive);

    if (unit->type == DRIVE_TYPE_1570
        || unit->type == DRIVE_TYPE_1571
        || unit->type == DRIVE_TYPE_1571CR) {
        if (track > (DRIVE_HALFTRACKS_1571 / 2)) {
            track -= (DRIVE_HALFTRACKS_1571 / 2);
            side = 1;
        }
    }
    /* TODO: drive 1 ? */
    drive_set_half_track(track * 2, side, drive);

    if (unit->type == DRIVE_TYPE_1540
        || unit->type == DRIVE_TYPE_1541
        || unit->type == DRIVE_TYPE_1541II
        || unit->type == DRIVE_TYPE_1570
        || unit->type == DRIVE_TYPE_1571
        || unit->type == DRIVE_TYPE_1571CR) {
        memcpy(&(unit->drive_ram[0x0400]), buffer, 256);
    }
}

/* ------------------------------------------------------------------------- */

/* Initialize the hardware-level drive emulation (should be called at least
   once before anything else).  Return 0 on success, -1 on error.  */
int drive_init(void)
{
    unsigned int unit;
    drive_t *drive;

    if (rom_loaded) {
        return 0;
    }

    drive_init_was_called = 1;

    driverom_init();
    drive_image_init();

    drive_log = log_open("Drive");

    for (unit = 0; unit < NUM_DISK_UNITS; unit++) {
        diskunit_context_t *diskunit =  diskunit_context[unit];
        char *logname;
        unsigned int d;

        logname = lib_msprintf("Unit %u", unit + 8);
        diskunit->log = log_open(logname);
        lib_free(logname);

        diskunit_clk[unit] = 0L;

        for (d = 0; d < NUM_DRIVES; d++) {
            drive = diskunit->drives[d];

            drive->drive = d;
            drive->diskunit = diskunit_context[unit];
        }

    }

    driverom_load_images();
    /* Do not error out if _SOME_ images are not found, ie. FD2K/4K, CMDHD */
#if 0
    if (driverom_load_images() < 0) {
        resources_set_int("Drive8Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive9Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive10Type", DRIVE_TYPE_NONE);
        resources_set_int("Drive11Type", DRIVE_TYPE_NONE);
        return -1;
    }
#endif

    log_message(drive_log, "Finished loading ROM images.");
    rom_loaded = 1;

    for (unit = 0; unit < NUM_DISK_UNITS; unit++) {
        diskunit_context_t *diskunit = diskunit_context[unit];
        drive = diskunit->drives[0];

        machine_drive_port_default(diskunit);

        if (drive_check_type(diskunit->type, unit) < 1) {
            resources_set_int_sprintf("Drive%uType", DRIVE_TYPE_NONE, unit + 8);
        }

        machine_drive_rom_setup_image(unit);
    }

    for (unit = 0; unit < NUM_DISK_UNITS; unit++) {
        diskunit_context_t *diskunit = diskunit_context[unit];
        int d;

        for (d = 0; d < NUM_DRIVES; d++) {
            drive = diskunit->drives[d];

            drive->gcr = gcr_create_image();
            drive->p64 = lib_calloc(1, sizeof(TP64Image));
            P64ImageCreate(drive->p64);
            drive->byte_ready_level = 1;
            drive->byte_ready_edge = 1;
            drive->GCR_dirty_track = 0;
            drive->GCR_write_value = 0x55;
            drive->GCR_track_start_ptr = NULL;
            drive->GCR_current_track_size = 0;
            drive->attach_clk = (CLOCK)0;
            drive->detach_clk = (CLOCK)0;
            drive->attach_detach_clk = (CLOCK)0;
            drive->old_led_status = 0;
            drive->old_half_track = 0;
            drive->side = 0;
            drive->GCR_image_loaded = 0;
            drive->P64_image_loaded = 0;
            drive->P64_dirty = 0;
            drive->read_only = 0;
            drive->led_last_change_clk = *(diskunit->clk_ptr);
            drive->led_last_uiupdate_clk = *(diskunit->clk_ptr);
            drive->led_active_ticks = 0;
            drive->read_write_mode = 1;

            /* Position the R/W head on the directory track.  */
            drive_set_half_track(36, 0, drive);
            drive_set_active_led_color(diskunit->type, unit);
        }
    }

    for (unit = 0; unit < NUM_DISK_UNITS; unit++) {
        diskunit_context_t *diskunit = diskunit_context[unit];
        drive = diskunit->drives[0];

        driverom_initialize_traps(diskunit);

        /* Sets diskunit->clock_frequency */
        drivesync_clock_frequency(diskunit, diskunit->type);

        /* TODO: rotation code is not drive1 aware */
        rotation_init((diskunit->clock_frequency == 2) ? 1 : 0, unit);
        rotation_reset(drive);

        if (diskunit->type == DRIVE_TYPE_2000 || diskunit->type == DRIVE_TYPE_4000 ||
            diskunit->type == DRIVE_TYPE_CMDHD) {
            drivecpu65c02_init(diskunit, diskunit->type);
        } else {
            drivecpu_init(diskunit, diskunit->type);
        }

        /* Make sure the sync factor is acknowledged correctly.  */
        drivesync_factor(diskunit);

        /* Make sure the traps are moved as needed.  */
        if (diskunit->enable) {
            drive_enable(diskunit);
        }
    }

    return 0;
}

void drive_shutdown(void)
{
    unsigned int unr, dnr;

    if (!drive_init_was_called) {
        /* happens at the -help command line command*/
        return;
    }

    for (unr = 0; unr < NUM_DISK_UNITS; unr++) {
        diskunit_context_t *unit = diskunit_context[unr];

        if (unit->type == DRIVE_TYPE_2000 || unit->type == DRIVE_TYPE_4000 ||
            unit->type == DRIVE_TYPE_CMDHD) {
            drivecpu65c02_shutdown(diskunit_context[unr]);
        } else {
            drivecpu_shutdown(diskunit_context[unr]);
        }

        if (unit->ds1216) {
            ds1216e_destroy(unit->ds1216, unit->rtc_save);
            unit->ds1216 = NULL;
        }

        for (dnr = 0; dnr < NUM_DRIVES; dnr++) {
            drive_t *drive = unit->drives[dnr];

            if (drive->gcr) {
                gcr_destroy_image(drive->gcr);
            }
            if (drive->p64) {
                P64ImageDestroy(drive->p64);
                lib_free(drive->p64);
                drive->p64 = NULL;
            }
        }
    }

    for (unr = 0; unr < NUM_DISK_UNITS; unr++) {
        diskunit_context_t *unit = diskunit_context[unr];

        for (dnr = 0; dnr < NUM_DRIVES; dnr++) {
            drive_t *drive = unit->drives[dnr];

            lib_free(drive);
            unit->drives[dnr] = NULL;
        }
        lib_free(unit);
        diskunit_context[unr] = NULL;
    }
}

void drive_set_active_led_color(unsigned int type, unsigned int dnr)
{
    switch (type) {
        case DRIVE_TYPE_1540:   /* green power, red drive, horizontal, round */
        case DRIVE_TYPE_1541:   /* green power, red drive, horizontal, round */
        case DRIVE_TYPE_1551:   /* green power, red drive, horizontal, round */
        case DRIVE_TYPE_1570:   /* green power, red drive, horizontal, round */
        case DRIVE_TYPE_2031:   /* green power, red drive, horizontal, round */
            drive_led_color[dnr] = DRIVE_LED1_RED;
            break;
        case DRIVE_TYPE_1571:   /* red power, green drive, horizontal, line */
        case DRIVE_TYPE_1571CR: /* red power, green drive, horizontal, line */
        case DRIVE_TYPE_1541II: /* red power, green drive, vertical, line (some models only) */
        case DRIVE_TYPE_1581:   /* red power, green drive, vertical, line */
            drive_led_color[dnr] = DRIVE_LED1_GREEN;
            break;
        case DRIVE_TYPE_2000:   /* red power, green activity, red error, horizontal, line */
        case DRIVE_TYPE_4000:   /* red power, green activity, red error, horizontal, line */
        case DRIVE_TYPE_CMDHD:   /* red power, green activity, red error, horizontal, line */
            drive_led_color[dnr] = DRIVE_LED1_GREEN | DRIVE_LED2_RED;
            break;
        case DRIVE_TYPE_2040:   /* red drive0, red error, red drive1, triangle, round */
        case DRIVE_TYPE_3040:   /* red drive0, red error, red drive1, triangle, round */
        case DRIVE_TYPE_4040:   /* red drive0, red error, red drive1, triangle, round */
        case DRIVE_TYPE_8050:   /* green drive0, green power/red error, green drive1, very flat triangle, round */
                                /* Some drives have red drive0/1 */
        case DRIVE_TYPE_1001:   /* green power/red error, red drive, horizontal, round */
            /* drive_led_color[dnr] = DRIVE_LED1_RED | DRIVE_LED2_RED;
             * We lie here and give the LEDs different colours.
             * The GUI can show only 2 LEDs (one for each drive) instead
             * of 3.  It does take 2 values, but they are displayed
             * (mixed) in the same place.  I guess it's thinking of
             * 2-colour LEDs.
             * So if both are set to red, we can't distinguish between
             * activity and error. Since it seems worse to make the
             * error LED green, I chose to make the activity green.
             * As soon as the GUI can show all 3 leds, we can make them
             * the right colours.
             */
            drive_led_color[dnr] = DRIVE_LED1_GREEN | DRIVE_LED2_RED;
            break;
        case DRIVE_TYPE_9000:   /* red drive1, green power, red drive2, horizontal, round */
            drive_led_color[dnr] = DRIVE_LED1_RED | DRIVE_LED2_RED;
            break;
        case DRIVE_TYPE_8250:   /* red drive0, green power/red error, green drive1, horizontal, round */
            /* drive_led_color[dnr] = DRIVE_LED1_GREEN | DRIVE_LED2_GREEN; * only the LP version is RED */
            /* The same note as for 8050. */
            drive_led_color[dnr] = DRIVE_LED1_GREEN | DRIVE_LED2_RED;
            break;
        default:
            drive_led_color[dnr] = DRIVE_LED1_RED;
            break;
    }
}

int drive_set_disk_drive_type(unsigned int type, struct diskunit_context_s *drv)
{
    unsigned int dnr;
    drive_t *drive0, *drive1;

    dnr = drv->mynumber;

    if (machine_drive_rom_check_loaded(type) < 0) {
        return -1;
    }

    drive0 = drv->drives[0];
    drive1 = drv->drives[1];

    /* TODO: drive 1? */
    rotation_rotate_disk(drive0);
    drivesync_clock_frequency(drv, type);

    rotation_init(0, dnr);
    drv->type = type;
    if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000 ||
        type == DRIVE_TYPE_CMDHD) {
        drivecpu65c02_setup_context(drv, 0);
    } else {
        drivecpu_setup_context(drv, 0);
    }
    drive0->side = 0;
    drive1->side = 0;
    machine_drive_rom_setup_image(dnr);
    drivesync_factor(drv);
    drive_set_active_led_color(type, dnr);

    if (type == DRIVE_TYPE_2000 || type == DRIVE_TYPE_4000 ||
        type == DRIVE_TYPE_CMDHD) {
        drivecpu65c02_init(drv, type);
    } else {
        drivecpu_init(drv, type);
    }

    return 0;
}

int drive_get_disk_drive_type(int dnr)
{
    if (dnr >= 0 && dnr < NUM_DISK_UNITS) {
        return diskunit_context[dnr]->type;
    }

    return DRIVE_TYPE_NONE;
}

void drive_enable_update_ui(diskunit_context_t *drv)
{
    int i;
    unsigned int enabled_units = 0;

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        unsigned int the_drive;
        diskunit_context_t *unit = diskunit_context[i];
        /* TODO: drive 1 */
        drive_t *drive = unit->drives[0];

        the_drive = 1 << i;

        if (unit->enable) {
            enabled_units |= the_drive;
            drive->old_led_status = -1;
            drive->old_half_track = -1;
            drive->old_side = -1;
        }
    }

    ui_enable_drive_status(enabled_units,
                           drive_led_color);
}

/* Activate full drive emulation. */
int drive_enable(diskunit_context_t *drv)
{
    int drive_true_emulation = 0;
    unsigned int dnr;
    unsigned int drive;

    dnr = drv->mynumber;

    /* This must come first, because this might be called before the drive
       initialization.  */
    if (!rom_loaded) {
        return -1;
    }

    DBG(("drive_enable unit: %d", 8 + drv->mynumber));
    resources_get_int_sprintf("Drive%uTrueEmulation", &drive_true_emulation, 8 + drv->mynumber);

    /* Always disable kernal traps. */
    if (!drive_true_emulation) {
        return 0;
    }

    if (drv->type == DRIVE_TYPE_NONE) {
        return 0;
    }

    /* Recalculate drive geometry.  */
    for (drive = 0; drive < NUM_DRIVES; drive++) {
        if (drv->drives[drive]->image != NULL) {
            drive_image_attach(drv->drives[drive]->image, dnr, drive);
        }
    }

    /* resync */
    drv->cpu->stop_clk = *(drv->clk_ptr);

    if (drv->type == DRIVE_TYPE_2000 ||
        drv->type == DRIVE_TYPE_4000 ||
        drv->type == DRIVE_TYPE_CMDHD) {
        drivecpu65c02_wake_up(drv);
    } else {
        drivecpu_wake_up(drv);
    }

    /* Make sure the UI is updated.  */
    drive_enable_update_ui(drv);
    return 0;
}

/* Disable full drive emulation.  */
void drive_disable(diskunit_context_t *drv)
{
    int drive_true_emulation = 0;
    unsigned int drive;

    /* This must come first, because this might be called before the true
       drive initialization.  */
    drv->enable = 0;

    DBG(("drive_disable unit: %u", 8 + drv->mynumber));
    resources_get_int_sprintf("Drive%uTrueEmulation", &drive_true_emulation, 8 + drv->mynumber);

    if (rom_loaded) {
        if (drv->type == DRIVE_TYPE_2000 || drv->type == DRIVE_TYPE_4000 ||
            drv->type == DRIVE_TYPE_CMDHD) {
            drivecpu65c02_sleep(drv);
        } else {
            drivecpu_sleep(drv);
        }
        machine_drive_port_default(drv);

        for (drive = 0; drive < NUM_DRIVES; drive++) {
            drive_gcr_data_writeback(drv->drives[drive]);
        }
    }

    /* Make sure the UI is updated.  */
    drive_enable_update_ui(drv);
}

monitor_interface_t *drive_cpu_monitor_interface_get(unsigned int dnr)
{
    return diskunit_context[dnr]->cpu->monitor_interface;
}

void drive_cpu_early_init_all(void)
{
    unsigned int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        machine_drive_init(diskunit_context[dnr]);
    }
}

void drive_cpu_trigger_reset(unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];

    if (unit->type == DRIVE_TYPE_2000 || unit->type == DRIVE_TYPE_4000 ||
        unit->type == DRIVE_TYPE_CMDHD) {
        drivecpu65c02_trigger_reset(dnr);
    } else {
        drivecpu_trigger_reset(dnr);
    }
    is_jammed[dnr] = false;
}

/* called by machine_specific_reset() */
void drive_reset(void)
{
    unsigned int dnr;
    unsigned int d;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (unit->type == DRIVE_TYPE_2000 || unit->type == DRIVE_TYPE_4000 ||
            unit->type == DRIVE_TYPE_CMDHD) {
            drivecpu65c02_reset(diskunit_context[dnr]);
        } else {
            drivecpu_reset(diskunit_context[dnr]);
        }

        for (d = 0; d < NUM_DRIVES; d++) {
            drive_t *drive = unit->drives[d];

            drive->led_last_change_clk = *(unit->clk_ptr);
            drive->led_last_uiupdate_clk = *(unit->clk_ptr);
            drive->led_active_ticks = 0;
        }
        is_jammed[dnr] = false;
    }
}

/* NOTE: this function is very similar to machine_jam - in case the behavior
         changes, change machine_jam too */
unsigned int drive_jam(int mynumber, const char *format, ...)
{
    va_list ap;
    ui_jam_action_t ret = JAM_NONE;

    /* always ignore subsequent JAMs. reset would clear the flag again, not
     * setting it when going to the monitor would just repeatedly pop up the
     * jam dialog (until reset)
     */
    if (is_jammed[mynumber]) {
        return JAM_NONE;
    }

    is_jammed[mynumber] = true;

    va_start(ap, format);
    if (jam_reason[mynumber]) {
        lib_free(jam_reason[mynumber]);
        jam_reason[mynumber] = NULL;
    }
    jam_reason[mynumber] = lib_mvsprintf(format, ap);
    va_end(ap);

    log_message(LOG_DEFAULT, "*** %s", jam_reason[mynumber]);

    vsync_suspend_speed_eval();
    sound_suspend();

    /* FIXME: perhaps we want a seperate setting for drives? */
    resources_get_int("JAMAction", &jam_action);

    if (jam_action == MACHINE_JAM_ACTION_DIALOG) {
        if (monitor_is_remote() || monitor_is_binary()) {
            if (monitor_is_remote()) {
                ret = monitor_network_ui_jam_dialog("%s", jam_reason[mynumber]);
            }

            if (monitor_is_binary()) {
                ret = monitor_binary_ui_jam_dialog("%s", jam_reason[mynumber]);
            }
        } else if (!console_mode) {
            ret = ui_jam_dialog("%s", jam_reason[mynumber]);
        }
    } else if (jam_action == MACHINE_JAM_ACTION_QUIT) {
        archdep_vice_exit(EXIT_SUCCESS);
    } else {
        int actions[4] = {
            -1, UI_JAM_MONITOR, UI_JAM_RESET_CPU, UI_JAM_POWER_CYCLE
        };
        ret = actions[jam_action - 1];
    }

    switch (ret) {
        case UI_JAM_RESET_CPU:
            return JAM_RESET_CPU;
        case UI_JAM_POWER_CYCLE:
            return JAM_POWER_CYCLE;
        case UI_JAM_MONITOR:
            return JAM_MONITOR;
        default:
            break;
    }
    return JAM_NONE;
}

bool drive_is_jammed(int mynumber)
{
    return is_jammed[mynumber];
}

char *drive_jam_reason(int mynumber)
{
    return jam_reason[mynumber];
}

/* Move the head to half track `num'.  */
void drive_set_half_track(int num, int side, drive_t *dptr)
{
    unsigned int type = dptr->diskunit->type;
    int tmp;

    if ((type == DRIVE_TYPE_1540
         || type == DRIVE_TYPE_1541
         || type == DRIVE_TYPE_1541II
         || type == DRIVE_TYPE_1551
         || type == DRIVE_TYPE_1570
         || type == DRIVE_TYPE_2031) && (num > DRIVE_HALFTRACKS_1541)) {
        num = DRIVE_HALFTRACKS_1541;
    }
    if ((type == DRIVE_TYPE_1571 || type == DRIVE_TYPE_1571CR)
        && (num > DRIVE_HALFTRACKS_1571)) {
        num = DRIVE_HALFTRACKS_1571;
    }
    if (num < 2) {
        num = 2;
    }

    if (dptr->current_half_track != num || dptr->side != side) {
        dptr->current_half_track = num;
        if (dptr->p64) {
            dptr->p64->PulseStreams[dptr->side][dptr->current_half_track].CurrentIndex = -1;
        }
    }
    dptr->side = side;

    /* FIXME: why would the offset be different for D71 and G71? */
    tmp = (dptr->image && dptr->image->type == DISK_IMAGE_TYPE_G71) ? DRIVE_HALFTRACKS_1571 : 70;

    dptr->GCR_track_start_ptr = dptr->gcr->tracks[dptr->current_half_track - 2 + (dptr->side * tmp)].data;

    if (dptr->GCR_current_track_size != 0) {
        dptr->GCR_head_offset = (dptr->GCR_head_offset
                                 * dptr->gcr->tracks[dptr->current_half_track - 2 + (dptr->side * tmp)].size)
                                / dptr->GCR_current_track_size;
    } else {
        dptr->GCR_head_offset = 0;
    }

    dptr->GCR_current_track_size =
        dptr->gcr->tracks[dptr->current_half_track - 2 + (dptr->side * tmp)].size;
}

/*-------------------------------------------------------------------------- */

/* Increment the head position by `step' half-tracks. Valid values
   for `step' are `+1', '+2' and `-1'.  */
void drive_move_head(int step, drive_t *drive)
{
    if ((step < -1) || (step > 1)) {
        log_warning(drive_log, "ambiguous step count (%d)", step);
    }
    drive_gcr_data_writeback(drive);
    drive_sound_head(drive->current_half_track, step, drive->diskunit->mynumber);
    drive_set_half_track(drive->current_half_track + step, drive->side, drive);
}

void drive_gcr_data_writeback(drive_t *drive)
{
    unsigned int half_track, track, end_half_track;
    int tmp;

    if (drive->image == NULL) {
        return;
    }

    /* FIXME: why would the offset be different for D71 and G71? */
    tmp = (drive->image && drive->image->type == DISK_IMAGE_TYPE_G71) ? DRIVE_HALFTRACKS_1571 : 70;
    half_track = drive->current_half_track + (drive->side * tmp);
    track = drive->current_half_track / 2;

    if (drive->image->type == DISK_IMAGE_TYPE_P64) {
        return;
    }

    if (!(drive->GCR_dirty_track)) {
        return;
    }

    /* always write track to GCR images, no need to extend the image */
    if ((drive->image->type == DISK_IMAGE_TYPE_G64) ||
        (drive->image->type == DISK_IMAGE_TYPE_G71)) {
        disk_image_write_half_track(drive->image, half_track,
                                    &drive->gcr->tracks[half_track - 2]);
        drive->GCR_dirty_track = 0;
        return;
    }
    /* writing beyond max tracks allowed in this image is not possible */
    if (half_track > drive->image->max_half_tracks) {
        drive->GCR_dirty_track = 0;
        return;
    }
    /* when trying beyond the image, check if we should extend the image */
    DBG(("check track: %u > drive->image->tracks: %u", track, drive->image->tracks));
    if (track > drive->image->tracks) {
        /* FIXME: doublesided images cant be extended with this logic, so
                  never do it */
        if ((drive->image->type == DISK_IMAGE_TYPE_D71) ||
#ifdef HAVE_X64_IMAGE
            (drive->image->type == DISK_IMAGE_TYPE_X64) ||
#endif
            (drive->image->type == DISK_IMAGE_TYPE_D81)) {
            drive->ask_extend_disk_image = DRIVE_EXTEND_ASK;
            drive->GCR_dirty_track = 0;
            return;
        }
        /* depending on the selected extend policy, ask or never/always extend */
        switch (drive->extend_image_policy) {
            case DRIVE_EXTEND_NEVER:
                drive->ask_extend_disk_image = DRIVE_EXTEND_ASK;
                drive->GCR_dirty_track = 0;
                return;
            case DRIVE_EXTEND_ASK:
                if (drive->ask_extend_disk_image == DRIVE_EXTEND_ASK) {
                    if (ui_extend_image_dialog() == 0) {
                        drive->GCR_dirty_track = 0;
                        drive->ask_extend_disk_image = DRIVE_EXTEND_NEVER;
                        return;
                    }
                    drive->ask_extend_disk_image = DRIVE_EXTEND_ACCESS;
                } else if (drive->ask_extend_disk_image == DRIVE_EXTEND_NEVER) {
                    drive->GCR_dirty_track = 0;
                    return;
                }
                break;
            case DRIVE_EXTEND_ACCESS:
                drive->ask_extend_disk_image = DRIVE_EXTEND_ASK;
                break;
        }
        /* determine the desired new size of the image. usually we want either
           35, 40 or 42 tracks */
        if (drive->image->tracks <= 35) {
            /* usually extend from 35 to 40 tracks */
            end_half_track = 2 + (40 * 2);
        } else if (drive->image->tracks <= 40) {
            /* next size is 42 tracks (usually the maximum) */
            end_half_track = 2 + (42 * 2);
        } else {
            /* beyond this, extend one track. this should never happen */
            end_half_track = half_track + 2;
        }
        /* write all tracks up to the end of the image */
        DBG(("extend track: %u drive->image->max_half_tracks: %u drive->image->tracks: %u", track, drive->image->max_half_tracks, drive->image->tracks));
        while (half_track < end_half_track) {
            DBG(("write halftrack: %u end: %u track: %u", half_track, end_half_track, half_track / 2));
            disk_image_write_half_track(drive->image, half_track, &drive->gcr->tracks[half_track - 2]);
            half_track += 2;
        }
    } else {
        /* write (only) the requested track */
        DBG(("write track: %u drive->image->max_half_tracks: %u drive->image->tracks: %u", track, drive->image->max_half_tracks, drive->image->tracks));
        disk_image_write_half_track(drive->image, half_track, &drive->gcr->tracks[half_track - 2]);
    }

    drive->GCR_dirty_track = 0;
}

void drive_gcr_data_writeback_all(void)
{
    drive_t *drive;
    unsigned int i, j;

    for (i = 0; i < NUM_DISK_UNITS; i++) {
        for (j = 0; j < 2; j++) {
            drive = diskunit_context[i]->drives[j];
            if (drive) {
                drive_gcr_data_writeback(drive);
                if (drive->P64_image_loaded && drive->image && drive->image->p64) {
                    if (drive->image->type == DISK_IMAGE_TYPE_P64) {
                        if (drive->P64_dirty) {
                            drive->P64_dirty = 0;
                            disk_image_write_p64_image(drive->image);
                        }
                    }
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

static void drive_led_update(diskunit_context_t *unit, drive_t *drive, int base)
{
    int my_led_status = 0;
    CLOCK led_period;
    int led_pwm1;

    /* Actually update the LED status only if the `trap idle'
       idling method is being used, as the LED status could be
       incorrect otherwise.  */

    if (unit->idling_method != DRIVE_IDLE_SKIP_CYCLES) {
        my_led_status = drive->led_status;
    }

    /* Update remaining led clock ticks. */
    if (drive->led_status & 1) {
        drive->led_active_ticks += *(unit->clk_ptr)
                                   - drive->led_last_change_clk;
    }
    drive->led_last_change_clk = *(unit->clk_ptr);

    led_period = *(unit->clk_ptr) - drive->led_last_uiupdate_clk;
    drive->led_last_uiupdate_clk = *(unit->clk_ptr);

    if (led_period == 0) {
        return;
    }

    if (drive->led_active_ticks > led_period) {
        /* during startup it has been observed that led_pwm1 > 1000,
           which potentially breaks several UIs */
        /* this also happens when the drive is reset from UI
           and the LED was on */
        led_pwm1 = 1000;
    } else {
        led_pwm1 = (int)(drive->led_active_ticks / led_period * 1000);
    }
    assert(led_pwm1 <= MAX_PWM);
    if (led_pwm1 > MAX_PWM) {
        led_pwm1 = MAX_PWM;
    }

    drive->led_active_ticks = 0;

    if (led_pwm1 != drive->led_last_pwm
        || my_led_status != drive->old_led_status) {
        ui_display_drive_led(drive->diskunit->mynumber, base, led_pwm1,
                             (my_led_status & 2) ? 1000 : 0);
        drive->led_last_pwm = led_pwm1;
        drive->old_led_status = my_led_status;
    }
}

/* Update the status bar in the UI.  */
void drive_update_ui_status(void)
{
    int i;

    if (console_mode || (machine_class == VICE_MACHINE_VSID)) {
        return;
    }

    /* Update the LEDs and the track indicators.  */
    for (i = 0; i < NUM_DISK_UNITS; i++) {
        diskunit_context_t *unit = diskunit_context[i];
        drive_t *drive0 = unit->drives[0];
        drive_t *drive1 = unit->drives[1];

        if (unit->enable) {

            drive_led_update(unit, drive0, 0);
            if (drive0->current_half_track != drive0->old_half_track
                || drive0->side != drive0->old_side) {
                drive0->old_half_track = drive0->current_half_track;
                drive0->old_side = drive0->side;
                ui_display_drive_track(i, 0, drive0->current_half_track, drive0->side);
            }
            /* update LED and track of the second drive for dual drives */
            if (drive_check_dual(unit->type)) {
                drive_led_update(unit, drive1, 1);
                if (drive1->current_half_track != drive1->old_half_track
                    || drive1->side != drive1->old_side) {
                    drive1->old_half_track = drive1->current_half_track;
                    drive1->old_side = drive1->side;
                    ui_display_drive_track(i, 1, drive1->current_half_track, drive1->side);
                }
            }
        }
    }
}

int drive_num_leds(unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];

    switch (unit->type) {
    case DRIVE_TYPE_2040:
    case DRIVE_TYPE_3040:
    case DRIVE_TYPE_4040:
    case DRIVE_TYPE_8050:
    case DRIVE_TYPE_8250:
    case DRIVE_TYPE_9000:
    case DRIVE_TYPE_2000:
    case DRIVE_TYPE_4000:
    case DRIVE_TYPE_CMDHD:
        return 2;
    default:
        return 1;
    }
}

void drive_cpu_execute_one(diskunit_context_t *drv, CLOCK clk_value)
{
    if (drv->type == DRIVE_TYPE_2000 || drv->type == DRIVE_TYPE_4000 ||
        drv->type == DRIVE_TYPE_CMDHD) {
        drivecpu65c02_execute(drv, clk_value);
    } else {
        drivecpu_execute(drv, clk_value);
    }
}

void drive_cpu_execute_all(CLOCK clk_value)
{
    unsigned int dnr;

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];

        if (unit->enable) {
            drive_cpu_execute_one(unit, clk_value);
        }
    }
}

void drive_cpu_set_overflow(diskunit_context_t *drv)
{
    if (drv->type == DRIVE_TYPE_2000 || drv->type == DRIVE_TYPE_4000 ||
        drv->type == DRIVE_TYPE_CMDHD) {
        /* nothing */
    } else {
        drivecpu_set_overflow(drv);
    }
}

/* This is called at every vsync. */
void drive_vsync_hook(void)
{
    unsigned int dnr;

    drive_update_ui_status();

    for (dnr = 0; dnr < NUM_DISK_UNITS; dnr++) {
        diskunit_context_t *unit = diskunit_context[dnr];
        drive_t *drive = unit->drives[0];

        if (unit->enable) {
            if (unit->idling_method != DRIVE_IDLE_SKIP_CYCLES) {
                drive_cpu_execute_one(diskunit_context[dnr], maincpu_clk);
            }
            if (unit->idling_method == DRIVE_IDLE_NO_IDLE) {
                /* if drive is never idle, also rotate the disk. this prevents
                 * huge peaks in cpu usage when the drive must catch up with
                 * a longer period of time.
                 */
                /* TODO: drive 1 */
                rotation_rotate_disk(drive);
            }
            /* printf("drive_vsync_hook drv %d @clk:%d\n", dnr, maincpu_clk); */
        }
    }
}

/* ------------------------------------------------------------------------- */

static void drive_setup_context_for_unit(diskunit_context_t *drv,
                                          unsigned int unr)
{
    unsigned int d;

    drv->mynumber = unr;

    for (d = 0; d < NUM_DRIVES; d++) {
        drv->drives[d] = lib_calloc(1, sizeof(drive_t));
        /* TODO: init functions for allocated memory */
        drv->drives[d]->image = NULL;
        drv->drives[d]->diskunit = drv;
        drv->drives[d]->drive = d;
    }

    drv->clk_ptr = &diskunit_clk[unr];

    drivecpu_setup_context(drv, 1); /* no need for 65c02, only allocating common stuff */

    machine_drive_setup_context(drv);
}

void drive_setup_context(void)
{
    unsigned int unr;

    for (unr = 0; unr < NUM_DISK_UNITS; unr++) {
        diskunit_context[unr] = lib_calloc(1, sizeof(diskunit_context_t));
        drive_setup_context_for_unit(diskunit_context[unr], unr);
    }
}

int drive_has_buttons(unsigned int dnr)
{
    diskunit_context_t *unit = diskunit_context[dnr];
    if (unit->type == DRIVE_TYPE_2000 || unit->type == DRIVE_TYPE_4000) {
        /* single swap */
        return DRIVE_BUTTON_SWAP_SINGLE;
    } else if (unit->type == DRIVE_TYPE_CMDHD) {
         /* write protect, swap 8, swap 9 */
        return DRIVE_BUTTON_WRITE_PROTECT | DRIVE_BUTTON_SWAP_8 | DRIVE_BUTTON_SWAP_9;
    }
    return 0;
}

void drive_cpu_trigger_reset_button(unsigned int dnr, unsigned int button)
{
    diskunit_context_t *unit = diskunit_context[dnr];
    unit->button = button;
    drive_cpu_trigger_reset(dnr);
}
