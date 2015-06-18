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

#define DRIVE_NUM 4
#define MAX_PWM 1000

#define DRIVE_ROM_SIZE 0x8000
#define DRIVE_RAM_SIZE 0xc000

/* Extended disk image handling.  */
#define DRIVE_EXTEND_NEVER  0
#define DRIVE_EXTEND_ASK    1
#define DRIVE_EXTEND_ACCESS 2

/* Drive idling methods.  */
#define DRIVE_IDLE_NO_IDLE     0
#define DRIVE_IDLE_SKIP_CYCLES 1
#define DRIVE_IDLE_TRAP_IDLE   2

/* Drive type.  */
#define DRIVE_TYPE_NONE      0
#define DRIVE_TYPE_ANY    9999

#define DRIVE_TYPE_1540   1540
#define DRIVE_TYPE_1541   1541
#define DRIVE_TYPE_1541II 1542
#define DRIVE_TYPE_1551   1551
#define DRIVE_TYPE_1570   1570
#define DRIVE_TYPE_1571   1571
#define DRIVE_TYPE_1571CR 1573
#define DRIVE_TYPE_1581   1581
#define DRIVE_TYPE_2000   2000
#define DRIVE_TYPE_4000   4000
#define DRIVE_TYPE_2031   2031
#define DRIVE_TYPE_2040   2040  /* DOS 1 dual floppy drive, 170k/disk */
#define DRIVE_TYPE_3040   3040  /* DOS 2.0 dual floppy drive, 170k/disk */
#define DRIVE_TYPE_4040   4040  /* DOS 2.5 dual floppy drive, 170k/disk */
#define DRIVE_TYPE_1001   1001  /* DOS 2.7 single floppy drive, 1M/disk */
#define DRIVE_TYPE_8050   8050  /* DOS 2.7 dual floppy drive, 0.5M/disk */
#define DRIVE_TYPE_8250   8250  /* DOS 2.7 dual floppy drive, 1M/disk */

#define DRIVE_TYPE_NUM    17

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

struct gcr_s;
struct disk_image_s;

typedef struct drive_s {
    unsigned int mynumber;

    /* Pointer to the drive clock.  */
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

    /* Is this drive enabled?  */
    unsigned int enable;

    /* What drive type we have to emulate?  */
    unsigned int type;

    /* Disk side.  */
    unsigned int side;

    /* What idling method?  (See `DRIVE_IDLE_*')  */
    int idling_method;

    /* FD2000/4000 RTC save? */
    int rtc_save;

    /* pointers for detecting dual drives and finding the other one */
    /* (only needed as long as we abuse the odd devices for drive 1:) */
    struct drive_s *drive0;
    struct drive_s *drive1;
    int trap, trapcont;

    /* Byte ready line.  */
    unsigned int byte_ready_level;
    unsigned int byte_ready_edge;

    /* Flag: does the current track need to be written out to disk?  */
    int GCR_dirty_track;

    /* GCR value being written to the disk.  */
    BYTE GCR_write_value;

    /* Pointer to the start of the GCR data of this track.  */
    BYTE *GCR_track_start_ptr;

    /* Size of the GCR data for the current track.  */
    unsigned int GCR_current_track_size;

    /* Offset of the R/W head on the current track (bytes).  */
    unsigned int GCR_head_offset;

    /* Are we in read or write mode?  */
    int read_write_mode;

    /* Activates the byte ready line.  */
    int byte_ready_active;

    /* Clock frequency of this drive in 1MHz units.  */
    int clock_frequency;

    /* Tick when the disk image was attached.  */
    CLOCK attach_clk;

    /* Tick when the disk image was detached.  */
    CLOCK detach_clk;

    /* Tick when the disk image was attached, but an old image was just
       detached.  */
    CLOCK attach_detach_clk;

    /* Byte to read from r/w head.  */
    BYTE GCR_read;

    /* Only used for snapshot */
    unsigned long snap_accum;
    CLOCK snap_rotation_last_clk;
    int snap_last_read_data;
    BYTE snap_last_write_data;
    int snap_bit_counter;
    int snap_zero_count;
    int snap_seed;
    DWORD snap_speed_zone;
    DWORD snap_ue7_dcba;
    DWORD snap_ue7_counter;
    DWORD snap_uf4_counter;
    DWORD snap_fr_randcount;
    DWORD snap_filter_counter;
    DWORD snap_filter_state;
    DWORD snap_filter_last_state;
    DWORD snap_write_flux;
    DWORD snap_PulseHeadPosition;
    DWORD snap_xorShift32;
    DWORD snap_so_delay;
    DWORD snap_cycle_index;
    DWORD snap_ref_advance;
    DWORD snap_req_ref_cycles;

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

    /* Flag: What parallel cable do we emulate?  */
    int parallel_cable;

    /* If the user does not want to extend the disk image and `ask mode' is
    selected this flag gets cleared.  */
    int ask_extend_disk_image;

    /* Drive-specific logging goes here.  */
    signed int log;

    /* Pointer to the attached disk image.  */
    struct disk_image_s *image;

    /* Pointer to the gcr image.  */
    struct gcr_s *gcr;

    PP64Image p64;

    /* Which RAM expansion is enabled?  */
    int drive_ram2_enabled, drive_ram4_enabled, drive_ram6_enabled,
        drive_ram8_enabled, drive_rama_enabled;

    /* Is the Professional DOS extension enabled?  */
    int profdos;
    /* Is the Supercard+ extension enabled? */
    int supercard;

    /* RTC context */
    rtc_ds1216e_t *ds1216;

    /* Current ROM image.  */
    BYTE rom[DRIVE_ROM_SIZE];

    /* Current trap ROM image.  */
    BYTE trap_rom[DRIVE_ROM_SIZE];

    /* Drive RAM */
    BYTE drive_ram[DRIVE_RAM_SIZE];
} drive_t;


extern CLOCK drive_clk[DRIVE_NUM];

/* Drive context structure for low-level drive emulation.
   Full definition in drivetypes.h */
struct drive_context_s;
extern struct drive_context_s *drive_context[DRIVE_NUM];

extern int rom_loaded;

extern int drive_init(void);
extern int drive_enable(struct drive_context_s *drv);
extern void drive_disable(struct drive_context_s *drv);
extern void drive_move_head(int step, struct drive_s *drive);
/* Don't use these pointers before the context is set up!  */
extern struct monitor_interface_s *drive_cpu_monitor_interface_get(unsigned int dnr);
extern void drive_cpu_early_init_all(void);
extern void drive_cpu_prevent_clk_overflow_all(CLOCK sub);
extern void drive_cpu_trigger_reset(unsigned int dnr);
extern void drive_reset(void);
extern void drive_shutdown(void);
extern void drive_cpu_execute_one(struct drive_context_s *drv, CLOCK clk_value);
extern void drive_cpu_execute_all(CLOCK clk_value);
extern void drive_cpu_set_overflow(struct drive_context_s *drv);
extern void drive_vsync_hook(void);
extern int drive_get_disk_drive_type(int dnr);
extern void drive_enable_update_ui(struct drive_context_s *drv);
extern void drive_update_ui_status(void);
extern void drive_gcr_data_writeback(struct drive_s *drive);
extern void drive_gcr_data_writeback_all(void);
extern void drive_set_active_led_color(unsigned int type, unsigned int dnr);
extern int drive_set_disk_drive_type(unsigned int drive_type,
                                     struct drive_context_s *drv);

extern void drive_set_half_track(int num, int side, drive_t *dptr);
extern void drive_set_machine_parameter(long cycles_per_sec);
extern void drive_set_disk_memory(BYTE *id, unsigned int track,
                                  unsigned int sector,
                                  struct drive_context_s *drv);
extern void drive_set_last_read(unsigned int track, unsigned int sector,
                                BYTE *buffer, struct drive_context_s *drv);

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

extern int drive_num_leds(unsigned int dnr);

extern void drive_setup_context(void);

extern int drive_resources_type_init(unsigned int default_type);

#endif
