/*
 * drive.h - Hardware-level disk drive emulation.
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

#ifndef VICE_DRIVE_H
#define VICE_DRIVE_H

#include "types.h"
#include "rtc/ds1216e.h"
#include "p64.h"

/** \brief  Number of supported disk units
 */
#define NUM_DISK_UNITS   4

/** \brief  Minimum drive unit number
 */
#define DRIVE_UNIT_MIN  8

/** \brief  Maximum drive unit number
 */
#define DRIVE_UNIT_MAX  (DRIVE_UNIT_MIN + NUM_DISK_UNITS - 1)

/** \brief  Default drive unit number
 */
#define DRIVE_UNIT_DEFAULT  DRIVE_UNIT_MIN

/** \brief  Minimum drive number
 */
#define DRIVE_NUMBER_MIN  0

/** \brief  Maximum drive number
 */
#define DRIVE_NUMBER_MAX  1

#define NUM_DRIVES	2

/** \brief  Default drive number
 */
#define DRIVE_NUMBER_DEFAULT  DRIVE_NUMBER_MIN

#define MAX_PWM 1000

#define DRIVE_ROM_SIZE 0x8000
/* Upped to 64K due to CMD HD */
#define DRIVE_RAM_SIZE 0x10000

/* Extended disk image handling.  */
#define DRIVE_EXTEND_NEVER  0
#define DRIVE_EXTEND_ASK    1
#define DRIVE_EXTEND_ACCESS 2

/* Drive idling methods.  */
#define DRIVE_IDLE_NO_IDLE     0
#define DRIVE_IDLE_SKIP_CYCLES 1
#define DRIVE_IDLE_TRAP_IDLE   2

/* Drive type ID's and names. When adding things here, please also update
 * the `drive_type_info_list` array in src/drive/drive.c to keep UI's current
 */
#define DRIVE_TYPE_NONE     0
#define DRIVE_NAME_NONE     "None"

#define DRIVE_TYPE_ANY      9999
#define DRIVE_NAME_ANY      "Any"

#define DRIVE_TYPE_1540     1540
#define DRIVE_NAME_1540     "CBM 1540"

#define DRIVE_TYPE_1541     1541
#define DRIVE_NAME_1541     "CBM 1541"

#define DRIVE_TYPE_1541II   1542
#define DRIVE_NAME_1541II   "CBM 1541-II"

#define DRIVE_TYPE_1551     1551
#define DRIVE_NAME_1551     "CBM 1551"

#define DRIVE_TYPE_1570     1570
#define DRIVE_NAME_1570     "CBM 1570"

#define DRIVE_TYPE_1571     1571
#define DRIVE_NAME_1571     "CBM 1571"

#define DRIVE_TYPE_1571CR   1573
#define DRIVE_NAME_1571CR   "CBM 1571 CR"

#define DRIVE_TYPE_1581     1581
#define DRIVE_NAME_1581     "CBM 1581"

#define DRIVE_TYPE_2000     2000
#define DRIVE_NAME_2000     "CMD FD-2000"

#define DRIVE_TYPE_4000     4000
#define DRIVE_NAME_4000     "CMD FD-4000"

#define DRIVE_TYPE_2031     2031
#define DRIVE_NAME_2031     "CBM 2031"

#define DRIVE_TYPE_2040     2040    /* DOS 1 dual floppy drive, 170k/disk */
#define DRIVE_NAME_2040     "CBM 2040"

#define DRIVE_TYPE_3040     3040    /* DOS 2.0 dual floppy drive, 170k/disk */
#define DRIVE_NAME_3040     "CBM 3040"

#define DRIVE_TYPE_4040     4040    /* DOS 2.5 dual floppy drive, 170k/disk */
#define DRIVE_NAME_4040     "CBM 4040"

#define DRIVE_TYPE_1001     1001    /* DOS 2.7 single floppy drive, 1M/disk */
#define DRIVE_NAME_1001     "CBM SFD-1001"

#define DRIVE_TYPE_8050     8050    /* DOS 2.7 dual floppy drive, 0.5M/disk */
#define DRIVE_NAME_8050     "CBM 8050"

#define DRIVE_TYPE_8250     8250    /* DOS 2.7 dual floppy drive, 1M/disk */
#define DRIVE_NAME_8250     "CBM 8250"

#define DRIVE_TYPE_9000     9000    /* DOS 3.0 hard drive */
#define DRIVE_NAME_9000     "CBM D9090/60"

#define DRIVE_TYPE_CMDHD    4844    /* ASCII for HD */
#define DRIVE_NAME_CMDHD    "CMD HD"

#define DRIVE_TYPE_NUM    19

/* max. half tracks */
#define DRIVE_HALFTRACKS_1541   84
/* FIXME: this constant is at some places used unconditionally for all 2-sided drives */
#define DRIVE_HALFTRACKS_1571   84

/* Possible colors of the drive active LED.  */
#define DRIVE_LED1_RED     0
#define DRIVE_LED1_GREEN   1
#define DRIVE_LED2_RED     0
#define DRIVE_LED2_GREEN   2

/* Number of cycles before an attached disk becomes visible to the R/W head.
   This is mostly to make routines that auto-detect disk changes happy.  */
#define DRIVE_ATTACH_DELAY           (3 * 600000)

/* Number of cycles the write protection is activated on detach.  */
#define DRIVE_DETACH_DELAY           (3 * 200000)

/* Number of cycles the after a disk can be inserted after a disk has been
   detached.  */
#define DRIVE_ATTACH_DETACH_DELAY    (3 * 400000)

/* Parallel cables available.  */
#define DRIVE_PC_NONE     0
#define DRIVE_PC_STANDARD 1  /* speed-dos userport cable */
#define DRIVE_PC_DD3      2  /* dolphin-dos 3 userport cable */
#define DRIVE_PC_FORMEL64 3  /* formel 64 cartridge */

#define DRIVE_PC_NUM 4

/* ------------------------------------------------------------------------- */

typedef struct drive_type_info_s {
    const char *name;
    int         id;
} drive_type_info_t;


struct gcr_s;
struct disk_image_s;

/* TODO: more parts of that struct should go into diskunit_context_s.
   candidates: clk, clock_frequency
 */
typedef struct drive_s {
    unsigned int unit;  /* 0 ... NUM_DISK_UNITS-1 */
    unsigned int drive; /* DRIVE_NUMBER_MIN ... DRIVE_NUMBER_MAX */

    /* Pointer to the containing diskunit_context */
    struct diskunit_context_s *diskunit;

    /* Pointer to the diskunit clock.  */
    CLOCK *clk;

    int led_status;

    CLOCK led_last_change_clk;
    CLOCK led_last_uiupdate_clk;
    CLOCK led_active_ticks;
    unsigned int led_last_pwm;

    /* Current half track on which the R/W head is positioned.  */
    int current_half_track;

    /* last clock and new value for stepper position */
    CLOCK stepper_last_change_clk;
    int stepper_new_position;

    /* Disk side.  */
    unsigned int side;

    /* Byte ready line.  */
    unsigned int byte_ready_level;
    unsigned int byte_ready_edge;

    /* Flag: does the current track need to be written out to disk?  */
    int GCR_dirty_track;

    /* GCR value being written to the disk.  */
    uint8_t GCR_write_value;

    /* Pointer to the start of the GCR data of this track.  */
    uint8_t *GCR_track_start_ptr;

    /* Size of the GCR data for the current track.  */
    unsigned int GCR_current_track_size;

    /* Offset of the R/W head on the current track (bytes).  */
    unsigned int GCR_head_offset;

    /* Are we in read or write mode? 0 is write, <>0 is read */
    int read_write_mode;

    /* Activates the byte ready line.  */
    int byte_ready_active;
#define BRA_BYTE_READY  0x02    /* chosen for the bit in the VIA2 PCR register */
#define BRA_MOTOR_ON    0x04    /* chosen for the bit in the VIA2 PB  register */
#define BRA_LED         0x08

    /* Tick when the disk image was attached.  */
    CLOCK attach_clk;

    /* Tick when the disk image was detached.  */
    CLOCK detach_clk;

    /* Tick when the disk image was attached, but an old image was just
       detached.  */
    CLOCK attach_detach_clk;

    /* Byte to read from r/w head.  */
    uint8_t GCR_read;

    /* Only used for snapshot */
    unsigned long snap_accum;
    CLOCK snap_rotation_last_clk;
    int snap_last_read_data;
    uint8_t snap_last_write_data;
    int snap_bit_counter;
    int snap_zero_count;
    int snap_seed;
    uint32_t snap_speed_zone;
    uint32_t snap_ue7_dcba;
    uint32_t snap_ue7_counter;
    uint32_t snap_uf4_counter;
    uint32_t snap_fr_randcount;
    uint32_t snap_filter_counter;
    uint32_t snap_filter_state;
    uint32_t snap_filter_last_state;
    uint32_t snap_write_flux;
    uint32_t snap_PulseHeadPosition;
    uint32_t snap_xorShift32;
    uint32_t snap_so_delay;
    uint32_t snap_cycle_index;
    uint32_t snap_ref_advance;
    uint32_t snap_req_ref_cycles;

    /* IF: requested additional R cycles */
    int req_ref_cycles;

    /* UI stuff.  */
    int old_led_status;
    int old_half_track;
    unsigned int old_side;

    /* Complicated image, with complex emulation requirements */
    int complicated_image_loaded;

    /* Is a GCR image loaded?  */
    int GCR_image_loaded;

    /* Is a P64 image loaded?  */
    int P64_image_loaded;

    /* Is P64 image dirty?  */
    int P64_dirty;

    /* is this disk read only?  */
    int read_only;

    /* What extension policy?  */
    int extend_image_policy;

    /* If the user does not want to extend the disk image and `ask mode' is
    selected this flag gets cleared.  */
    int ask_extend_disk_image;

    /* Pointer to the attached disk image.  */
    struct disk_image_s *image;

    /* Pointer to the gcr image.  */
    struct gcr_s *gcr;

    PP64Image p64;

    /* rotations per minute (300rpm = 30000) */
    int rpm;

    /* state of the wobble emulation */
    float wobble_sin_count;
    int wobble_factor;      /* calculated factor used in the rotation code */
    int wobble_frequency;   /* from the resource */
    int wobble_amplitude;   /* from the resource */

} drive_t;


extern CLOCK diskunit_clk[NUM_DISK_UNITS];

/* Drive context structure for low-level drive emulation.
   Full definition in drivetypes.h */
#include "drivetypes.h"
extern struct diskunit_context_s *diskunit_context[NUM_DISK_UNITS];

extern int rom_loaded;

extern int drive_init(void);
extern int drive_enable(struct diskunit_context_s *drv);
extern void drive_disable(struct diskunit_context_s *drv);
extern void drive_move_head(int step, struct drive_s *drive);
/* Don't use these pointers before the context is set up!  */
extern struct monitor_interface_s *drive_cpu_monitor_interface_get(unsigned int dnr);
extern void drive_cpu_early_init_all(void);
extern void drive_cpu_prevent_clk_overflow_all(CLOCK sub);
extern void drive_cpu_trigger_reset(unsigned int dnr);
extern void drive_reset(void);
extern void drive_shutdown(void);
extern void drive_cpu_execute_one(struct diskunit_context_s *drv, CLOCK clk_value);
extern void drive_cpu_execute_all(CLOCK clk_value);
extern void drive_cpu_set_overflow(struct diskunit_context_s *drv);
extern void drive_vsync_hook(void);
extern int drive_get_disk_drive_type(int dnr);
extern void drive_enable_update_ui(struct diskunit_context_s *drv);
extern void drive_update_ui_status(void);
extern void drive_gcr_data_writeback(struct drive_s *drive);
extern void drive_gcr_data_writeback_all(void);
extern void drive_set_active_led_color(unsigned int type, unsigned int dnr);
extern int drive_set_disk_drive_type(unsigned int drive_type,
                                     struct diskunit_context_s *drv);

extern void drive_set_half_track(int num, int side, drive_t *dptr);
extern void drive_set_machine_parameter(long cycles_per_sec);
extern void drive_set_disk_memory(uint8_t *id, unsigned int track,
                                  unsigned int sector,
                                  struct diskunit_context_s *drv);
extern void drive_set_last_read(unsigned int track, unsigned int sector,
                                uint8_t *buffer, struct diskunit_context_s *drv);

extern int drive_check_type(unsigned int drive_type, unsigned int dnr);
extern int drive_check_extend_policy(int drive_type);
extern int drive_check_idle_method(int drive_type);
extern int drive_check_expansion(int drive_type);
extern int drive_check_expansion2000(int drive_type);
extern int drive_check_expansion4000(int drive_type);
extern int drive_check_expansion6000(int drive_type);
extern int drive_check_expansion8000(int drive_type);
extern int drive_check_expansionA000(int drive_type);
extern int drive_check_parallel_cable(int drive_type);
extern int drive_check_extend_policy(int drive_type);
extern int drive_check_profdos(int drive_type);
extern int drive_check_supercard(int drive_type);
extern int drive_check_stardos(int drive_type);
extern int drive_check_rtc(int drive_type);
extern int drive_check_iec(int drive_type);
extern int drive_num_leds(unsigned int dnr);

int drive_get_type_by_devnr(int devnr);
int drive_is_dualdrive_by_devnr(int devnr);

extern void drive_setup_context(void);

extern int drive_resources_type_init(unsigned int default_type);

extern int drive_has_buttons(unsigned int dnr);
extern void drive_cpu_trigger_reset_button(unsigned int dnr, unsigned int button);

#endif
