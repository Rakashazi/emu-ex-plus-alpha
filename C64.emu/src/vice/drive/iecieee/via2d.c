/*
 * via2d.c - VIA2 emulation in the 1541, 1541II, 1571 and 2031 disk drive.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

/*
    FIXME: test new code for regressions, remove old code
*/
#define OLDCODE 0    /* set to 1 to use the old ca2/cb2 handling code */

/* #define DEBUG_VIA2 */

#ifdef DEBUG_VIA2
#define DBG(_x_) log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>

#include "drive-writeprotect.h"
#include "drive.h"
#include "drivetypes.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "rotation.h"
#include "types.h"
#include "via.h"
#include "viad.h"
#include "drive-sound.h"

typedef struct drivevia2_context_s {
    unsigned int number;
    struct drive_s *drive;
} drivevia2_context_t;

static void set_ca2(via_context_t *via_context, int state)
{
#if !OLDCODE
    int curr;
    drivevia2_context_t *via2p;
    drive_t *drv;
    via2p = (drivevia2_context_t *)(via_context->prv);
    drv = via2p->drive;
    curr = ((drv->byte_ready_active >> 1) & 1);
    if (state != curr) {
        DBG(("VIA2: set_ca2 (%d to %d) (byte rdy)", curr, state));
        rotation_rotate_disk(drv);
        drv->byte_ready_active &= ~(1 << 1);
        drv->byte_ready_active |= state << 1;
        if (drv->byte_ready_edge) {
           drive_context_t *drive_context = (drive_context_t *)(via_context->context);
           drive_cpu_set_overflow(drive_context);
           drv->byte_ready_edge = 0;
        }
    }
#endif
}

static void set_cb2(via_context_t *via_context, int state)
{
#if !OLDCODE
    int curr;
    drivevia2_context_t *via2p;
    drive_t *drv;
    via2p = (drivevia2_context_t *)(via_context->prv);
    drv = via2p->drive;
    curr = ((drv->read_write_mode >> 5) & 1);
    if (state != curr) {
        DBG(("VIA2: set_cb2 (%d to %d) (head mode)", curr, state));
        rotation_rotate_disk(drv);
        drv->read_write_mode = state << 5;
    }
#endif
}

/* see interrupt.h; ugly, but more efficient... */
static void set_int(via_context_t *via_context, unsigned int int_num,
                    int value, CLOCK rclk)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(via_context->context);

    interrupt_set_irq(drive_context->cpu->int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num, int value)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(via_context->context);

    interrupt_restore_irq(drive_context->cpu->int_status, int_num, value);
}

void via2d_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    viacore_store(ctxptr->via2, addr, data);
}

BYTE via2d_read(drive_context_t *ctxptr, WORD addr)
{
    return viacore_read(ctxptr->via2, addr);
}

BYTE via2d_peek(drive_context_t *ctxptr, WORD addr)
{
    return viacore_peek(ctxptr->via2, addr);
}

int via2d_dump(drive_context_t *ctxptr, WORD addr)
{
    viacore_dump(ctxptr->via2);
    return 0;
}

/*
    pcrval
      bit 5:  1: reading, 0: writing
      bit 1:  1: byte ready active
*/
void via2d_update_pcr(int pcrval, drive_t *dptr)
{
    int bra = dptr->byte_ready_active;
    rotation_rotate_disk(dptr);
    dptr->read_write_mode = pcrval & 0x20;
    dptr->byte_ready_active = (bra & ~0x02) | (pcrval & 0x02);
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE oldpa_value,
                      WORD addr)
{
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);
    rotation_rotate_disk(via2p->drive);

    via2p->drive->GCR_write_value = byte;

    via2p->drive->byte_ready_level = 0;
}

static void undump_pra(via_context_t *via_context, BYTE byte)
{

}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE poldpb,
                      WORD addr)
{
    drivevia2_context_t *via2p = (drivevia2_context_t *)(via_context->prv);
    drive_t *drv = via2p->drive;
    int bra;


    DBG(("VIA2: store_prb (%02x to %02x) clock:%d", poldpb, byte, *(via_context->clk_ptr)));

    rotation_rotate_disk(drv);

    if (drv->led_status) {
        drv->led_active_ticks += *(via_context->clk_ptr)
                                          - drv->led_last_change_clk;
    }
    drv->led_last_change_clk = *(via_context->clk_ptr);
    drv->led_status = (byte & 8) ? 1 : 0;

    /* IF: based on 1540008-01, the original 'Long Board' schematics
       Stepper motor control is the 2 bit value set here demuxed to 4 lines.
       The lines drive the inputs lines of a stepper motor and represent coil control.
       The stepper lines are only enabled if the drive motor is on.
       The rotor moves to absolute positions controlled by activating the coils - the programming method
       suggests a binary counter circuitry, but that is not the case, the similarity is just a side effect.
       Note, how switching the drive motor on/off may move the stepper motor as well.
    */
    /* Process stepper motor if the drive motor is on */
    if (byte & 0x4) {

        /* vice track numbering starts with 2... we need the real, physical track number */
        int track_number = drv->current_half_track - 2;

        /* the new coil line activated */
        int new_stepper_position = byte & 3;

        /*
          track halftrack stepper
          log.  log.phys. position
          1     2   0     0
          1.5   3   1     1
          2     4   2     2
          2.5   5   3     3
          3     6   4     0
          3.5   7   5     1
          ... */

        int old_stepper_position = track_number & 3;

        /* FIXME: emulating the mechanical delay with such naive approach does
                  not work, as the actual step is delayed to the next write to pb.
                  regardless how long that will take. for one example that does
                  not work like this, see bug #508

           FIXME: we should implement the intended behaviour using an alarm
                  instead.
         */

        /* the steps travelled and the direction */
        /* int step_count = (drv->stepper_new_position - old_stepper_position) & 3; */
        int step_count = (new_stepper_position - old_stepper_position) & 3;
        if (step_count == 3) {
            step_count = -1;
        }
        /*
            minimal simulation of mechanical delay.

            in reality, there are 3 kinds of delays:
            - startup time, the time it takes from changing the coils to when
              the head starts moving.
            - seek time, the time it takes the head to move from track to track
            - settle time, the time it takes from stopping the head to being 
              able to read reliably.

            the simplified emulation here only simulates startup time, and then
            steps immediatly. while still not being quite accurate, this avoids
            unwanted stepping at power-up and/or reset (bug #401)

            NOTE: this unwanted step to 18.5 at poweron/reset can infact be
                  observed at real 1541-II drives as well
         */
        /*
            Action Replay 6:                                     8333 = 8.3ms
            Cauldron/The Dreams:                                 7734 = 7.7ms
            fastest usable stepping speed seems to be around     4096 = 4.1ms 
            min delay so we dont get a step at reset              700 = 0.7ms
         */
        /* if ((*(via_context->clk_ptr) - drv->stepper_last_change_clk) >= 2000) */ {
            /* presumably only single steps work */
            /* TODO: the general assumption for the case when opposite coils are active is
                    that the stepper will _not_ move at all. however, that doesnt seem to
                    be true in all cases. when the stepper is already moving (up or down),
                    and such "double step" takes place with the right timing, then it would
                    actually result in two steps (in the "current" direction!). this seems
                    unreliable on real hw as well, and only very few original titles need it.
                    allowing it always does more harm than good, so we should simply ignore
                    this condition for the time being. */
            if ((step_count == 1) || (step_count == -1)) {
                DBG(("VIA2: store_prb drive_move_head(%d) (%02x to %02x) clk:%d delay:%d", 
                     step_count, poldpb, byte, *(via_context->clk_ptr), 
                     (*(via_context->clk_ptr) - drv->stepper_last_change_clk)));
                drive_move_head(step_count, drv);
            }
        }

        /* if (new_stepper_position != drv->stepper_new_position) {
            drv->stepper_new_position = new_stepper_position;
            drv->stepper_last_change_clk = *(via_context->clk_ptr);
        } */
    }

    if ((poldpb ^ byte) & 0x60) {   /* Zone bits */
        rotation_speed_zone_set((byte >> 5) & 0x3, via2p->number);
    }
    if ((poldpb ^ byte) & 0x04) {   /* Motor on/off */
        drive_sound_update((byte & 4) ? DRIVE_SOUND_MOTOR_ON : DRIVE_SOUND_MOTOR_OFF, via2p->number);
        bra = drv->byte_ready_active;
        drv->byte_ready_active = (bra & ~0x04) | (byte & 0x04);
        if ((byte & 0x04) != 0) {
            rotation_begins(drv);
        } else {
            if (drv->byte_ready_edge) {
               drive_context_t *drive_context = (drive_context_t *)(via_context->context);
               drive_cpu_set_overflow(drive_context);
               drv->byte_ready_edge = 0;
            }
        }
    }

    drv->byte_ready_level = 0;
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);

    via2p->drive->led_status = (byte & 8) ? 1 : 0;
    rotation_speed_zone_set((byte >> 5) & 0x3, via2p->number);
    via2p->drive->byte_ready_active
        = (via2p->drive->byte_ready_active & ~0x04) | (byte & 0x04);
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);
    rotation_rotate_disk(via2p->drive);

#if OLDCODE
    /* FIXME: this should use via_set_ca2() and via_set_cb2() */
    if (byte != via_context->via[VIA_PCR]) {
        BYTE tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((byte & 0x0c) != 0x0c) { /* CA2 not lo or hi output */
            tmp |= 0x02; /* byte ready */
        }
        if ((byte & 0xc0) != 0xc0) { /* CB2 not lo or hi output */
            tmp |= 0x20; /* reading */
        }
        /* insert_your_favourite_drive_function_here(tmp);
        bit 5 is the write output to the analog circuitry:
        0 = writing, 0x20 = reading
        bit 1 is the "byte ready" signal
        */
        via2d_update_pcr(tmp, via2p->drive);
    }
#endif
    return byte;
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);

    via2d_update_pcr(byte, via2p->drive);
}

static void undump_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_acr(via_context_t *via_context, BYTE byte)
{
}

static void store_sr(via_context_t *via_context, BYTE byte)
{
}

static void store_t2l(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);

    via2p->drive->led_status = 1;
    drive_update_ui_status();
}

static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    BYTE byte;
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);

    /* IF: add bus read delay */
    via2p->drive->req_ref_cycles = BUS_READ_DELAY;

    rotation_byte_read(via2p->drive);

    byte = ((via2p->drive->GCR_read & ~(via_context->via[VIA_DDRA]))
           | (via_context->via[VIA_PRA] & via_context->via[VIA_DDRA]));

    via2p->drive->byte_ready_level = 0;

    return byte;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;
    drivevia2_context_t *via2p;

    via2p = (drivevia2_context_t *)(via_context->prv);

    /* IF: add bus read delay */
    via2p->drive->req_ref_cycles = BUS_READ_DELAY;

    rotation_rotate_disk(via2p->drive);
    byte = ((rotation_sync_found(via2p->drive)
           | drive_writeprotect_sense(via2p->drive)
           | 0x6f) /* output bits read 1 if used as input */
           & ~(via_context->via[VIA_DDRB]))
           | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]);

    DBG(("read_prb %02x pb:%02x ddr:%02x",byte,via_context->via[VIA_PRB],via_context->via[VIA_DDRB]));

    via2p->drive->byte_ready_level = 0;

    return byte;
}

void via2d_init(drive_context_t *ctxptr)
{
    viacore_init(ctxptr->via2, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status, ctxptr->cpu->clk_guard);
}

void via2d_setup_context(drive_context_t *ctxptr)
{
    drivevia2_context_t *via2p;
    via_context_t *via;

    /* Clear struct as snapshot code may write uninitialized values.  */
    ctxptr->via2 = lib_calloc(1, sizeof(via_context_t));
    via = ctxptr->via2;

    via->prv = lib_malloc(sizeof(drivevia2_context_t));

    via2p = (drivevia2_context_t *)(via->prv);
    via2p->number = ctxptr->mynumber;
    via2p->drive = ctxptr->drive;

    via->context = (void *)ctxptr;

    via->rmw_flag = &(ctxptr->cpu->rmw_flag);
    via->clk_ptr = ctxptr->clk_ptr;

    via->myname = lib_msprintf("Drive%dVia2", via2p->number);
    via->my_module_name = lib_msprintf("VIA2D%d", via2p->number);

    viacore_setup_context(via);

    via->irq_line = IK_IRQ;
    via->int_num
        = interrupt_cpu_status_int_new(ctxptr->cpu->int_status, via->myname);

    via->undump_pra = undump_pra;
    via->undump_prb = undump_prb;
    via->undump_pcr = undump_pcr;
    via->undump_acr = undump_acr;
    via->store_pra = store_pra;
    via->store_prb = store_prb;
    via->store_pcr = store_pcr;
    via->store_acr = store_acr;
    via->store_sr = store_sr;
    via->store_t2l = store_t2l;
    via->read_pra = read_pra;
    via->read_prb = read_prb;
    via->set_int = set_int;
    via->restore_int = restore_int;
    via->set_ca2 = set_ca2;
    via->set_cb2 = set_cb2;
    via->reset = reset;
}
