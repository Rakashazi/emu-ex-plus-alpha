/*
 * via1d15xx.c - VIA1 emulation in the 1541, 1541II, 1570 and 1571 disk drive.
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

#include "vice.h"

#include <stdio.h>

#include "debug.h"
#include "drive.h"
#include "drivesync.h"
#include "drivetypes.h"
#include "glue1571.h"
#include "iecbus.h"
#include "iecdrive.h"
#include "interrupt.h"
#include "lib.h"
#include "rotation.h"
#include "types.h"
#include "via.h"
#include "via1d1541.h"
#include "viad.h"


#define iecbus (via1p->v_iecbus)

typedef struct drivevia1_context_s {
    unsigned int number;
    struct drive_s *drive;        /* TODO: remove when no longer needed */
    struct diskunit_context_s *diskunit;
    int parallel_id;
    int v_parieee_is_out;         /* init to 1 */
    struct iecbus_s *v_iecbus;
} drivevia1_context_t;


void via1d1541_store(diskunit_context_t *ctxptr, uint16_t addr, uint8_t data)
{
    ctxptr->cpu->cpu_last_data = data;
    viacore_store(ctxptr->via1d1541, addr, data);
}

uint8_t via1d1541_read(diskunit_context_t *ctxptr, uint16_t addr)
{
    return ctxptr->cpu->cpu_last_data = viacore_read(ctxptr->via1d1541, addr);
}

uint8_t via1d1541_peek(diskunit_context_t *ctxptr, uint16_t addr)
{
    return viacore_peek(ctxptr->via1d1541, addr);
}

int via1d1541_dump(diskunit_context_t *ctxptr, uint16_t addr)
{
    viacore_dump(((diskunit_context_t*)ctxptr)->via1d1541);
    return 0;
}

static void set_ca2(via_context_t *via_context, int state)
{
}

static void set_cb2(via_context_t *via_context, int state, int offset)
{
}

static void set_int(via_context_t *via_context, unsigned int int_num,
                    int value, CLOCK rclk)
{
    diskunit_context_t *dc;

    dc = (diskunit_context_t *) via_context->context;

    interrupt_set_irq(dc->cpu->int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num,
                        int value)
{
    diskunit_context_t *dc;

    dc = (diskunit_context_t *) via_context->context;

    interrupt_restore_irq(dc->cpu->int_status, int_num, value);
}

static void undump_pra(via_context_t *via_context, uint8_t byte)
{
    drivevia1_context_t *via1p;
    diskunit_context_t *dc;

    via1p = (drivevia1_context_t *)(via_context->prv);
    dc = (diskunit_context_t *) via_context->context;

    if (dc->type == DRIVE_TYPE_1570
        || dc->type == DRIVE_TYPE_1571
        || dc->type == DRIVE_TYPE_1571CR) {
        drivesync_set_1571(dc, byte & 0x20);
        glue1571_side_set((byte >> 2) & 1, via1p->drive);
    } else {
        switch (via1p->diskunit->parallel_cable) {
            case DRIVE_PC_STANDARD:
            case DRIVE_PC_21SEC_BACKUP:
            case DRIVE_PC_FORMEL64:
                if (dc->type == DRIVE_TYPE_1540
                    || dc->type == DRIVE_TYPE_1541
                    || dc->type == DRIVE_TYPE_1541II) {
                    parallel_cable_drive_write(dc->parallel_cable, byte,
                                               PARALLEL_WRITE, via1p->number);
                }
                break;
        }
    }
}

static void store_pra(via_context_t *via_context, uint8_t byte, uint8_t oldpa_value,
                      uint16_t addr)
{
    drivevia1_context_t *via1p;
    diskunit_context_t *dc;

    via1p = (drivevia1_context_t *)(via_context->prv);
    /* dc = (diskunit_context_t *)(via_context->context); */
    dc = via1p->diskunit;

    if (dc->type == DRIVE_TYPE_1570
        || dc->type == DRIVE_TYPE_1571
        || dc->type == DRIVE_TYPE_1571CR) {
        if ((oldpa_value ^ byte) & 0x20) {
            drivesync_set_1571(dc, byte & 0x20);
        }
        if ((oldpa_value ^ byte) & 0x04) {
            glue1571_side_set((byte >> 2) & 1, via1p->drive);
        }
        if ((oldpa_value ^ byte) & 0x02) {
            iec_fast_drive_direction(byte & 2, via1p->number);
        }
    } else {
        switch (dc->parallel_cable) {
            case DRIVE_PC_STANDARD:
            case DRIVE_PC_21SEC_BACKUP:
            case DRIVE_PC_FORMEL64:
                if (dc->type == DRIVE_TYPE_1540
                    || dc->type == DRIVE_TYPE_1541
                    || dc->type == DRIVE_TYPE_1541II) {
                    parallel_cable_drive_write(dc->parallel_cable, byte,
                                               (((addr == VIA_PRA) && ((via_context->via[VIA_PCR]
                                                                        & 0xe) == 0xa)) ? PARALLEL_WRITE_HS : PARALLEL_WRITE),
                                               via1p->number);
                }
                break;
        }
    }
}

static void undump_prb(via_context_t *via_context, uint8_t byte)
{
    drivevia1_context_t *via1p;

    via1p = (drivevia1_context_t *)(via_context->prv);

    if (iecbus != NULL) {
        uint8_t *drive_bus, *drive_data;
        unsigned int unit;

        drive_bus = &(iecbus->drv_bus[via1p->number + 8]);
        drive_data = &(iecbus->drv_data[via1p->number + 8]);

        *drive_data = ~byte;
        *drive_bus = ((((*drive_data) << 3) & 0x40)
                      | (((*drive_data) << 6)
                         & ((uint8_t)(~(*drive_data) ^ iecbus->cpu_bus) << 3) & 0x80));

        iecbus->cpu_port = iecbus->cpu_bus;
        for (unit = 4; unit < 8 + NUM_DISK_UNITS; unit++) {
            iecbus->cpu_port &= iecbus->drv_bus[unit];
        }

        iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                            | (iecbus->cpu_port >> 7)
                            | ((iecbus->cpu_bus << 3) & 0x80));
    } else {
        iec_drive_write((uint8_t)(~byte), via1p->number);
    }
}

static void store_prb(via_context_t *via_context, uint8_t byte, uint8_t p_oldpb,
                      uint16_t addr)
{
    drivevia1_context_t *via1p;

    via1p = (drivevia1_context_t *)(via_context->prv);

    if (byte != p_oldpb) {
        DEBUG_IEC_DRV_WRITE(byte);

        if (iecbus != NULL) {
            uint8_t *drive_data, *drive_bus;
            unsigned int unit;

            drive_bus = &(iecbus->drv_bus[via1p->number + 8]);
            drive_data = &(iecbus->drv_data[via1p->number + 8]);

            *drive_data = ~byte;
            *drive_bus = ((((*drive_data) << 3) & 0x40)
                          | (((*drive_data) << 6)
                             & ((uint32_t)(~(*drive_data) ^ iecbus->cpu_bus) << 3) & 0x80));

            iecbus->cpu_port = iecbus->cpu_bus;
            for (unit = 4; unit < 8 + NUM_DISK_UNITS; unit++) {
                iecbus->cpu_port &= iecbus->drv_bus[unit];
            }

            iecbus->drv_port = (((iecbus->cpu_port >> 4) & 0x4)
                                | (iecbus->cpu_port >> 7)
                                | ((iecbus->cpu_bus << 3) & 0x80));

            DEBUG_IEC_BUS_WRITE(iecbus->drv_port);
        } else {
            iec_drive_write((uint8_t)(~byte), via1p->number);
            DEBUG_IEC_BUS_WRITE(~byte);
        }
    }
}

static void undump_pcr(via_context_t *via_context, uint8_t byte)
{
#if 0
    drivevia1_context_t *via1p;

    via1p = (drivevia1_context_t *)(via_context->prv);

    /* FIXME: Is this correct? */
    if (via1p->number != 0) {
        via2d_update_pcr(byte, &drive[0]);
    }
#endif
}

static uint8_t store_pcr(via_context_t *via_context, uint8_t byte, uint16_t addr)
{
    return byte;
}

static void undump_acr(via_context_t *via_context, uint8_t byte)
{
}

static void store_acr(via_context_t *via_context, uint8_t byte)
{
}

static void store_sr(via_context_t *via_context, uint8_t byte)
{
}

static void store_t2l(via_context_t *via_context, uint8_t byte)
{
}

static void reset(via_context_t *via_context)
{
}

static uint8_t read_pra(via_context_t *via_context, uint16_t addr)
{
    uint8_t byte;
    drivevia1_context_t *via1p;

    via1p = (drivevia1_context_t *)(via_context->prv);

    if (via1p->diskunit->type == DRIVE_TYPE_1570
        || via1p->diskunit->type == DRIVE_TYPE_1571
        || via1p->diskunit->type == DRIVE_TYPE_1571CR) {
        uint8_t tmp;
        rotation_rotate_disk(via1p->drive);
        tmp = (via1p->drive->byte_ready_level ? 0 : 0x80)
              | (via1p->drive->current_half_track == 2 ? 0 : 1);
        return (tmp & ~(via_context->via[VIA_DDRA]))
               | (via_context->via[VIA_PRA] & via_context->via[VIA_DDRA]);
    }

    switch (via1p->diskunit->parallel_cable) {
        case DRIVE_PC_STANDARD:
        case DRIVE_PC_21SEC_BACKUP:
        case DRIVE_PC_FORMEL64:
            byte = parallel_cable_drive_read(via1p->diskunit->parallel_cable,
                                             (((addr == VIA_PRA) && (via_context->via[VIA_PCR] & 0xe) == 0xa)) ? 1 : 0);
            break;
        default:
            byte = ((via_context->via[VIA_PRA] & via_context->via[VIA_DDRA])
                    | (0xff & ~(via_context->via[VIA_DDRA])));
            break;
    }

    return byte;
}

static uint8_t read_prb(via_context_t *via_context)
{
    uint8_t byte;
    uint8_t orval;
    drivevia1_context_t *via1p;

    via1p = (drivevia1_context_t *)(via_context->prv);

    /* 0 for drive0, 0x20 for drive 1 */
    orval = (via1p->number << 5);

    if (iecbus != NULL) {
        byte = (((via_context->via[VIA_PRB] & 0x1a)
                 | iecbus->drv_port) ^ 0x85) | orval;
    } else {
        byte = (((via_context->via[VIA_PRB] & 0x1a)
                 | iec_drive_read(via1p->number)) ^ 0x85) | orval;
    }

    DEBUG_IEC_DRV_READ(byte);

    DEBUG_IEC_BUS_READ(byte);

    return byte;
}

void via1d1541_init(diskunit_context_t *ctxptr)
{
    viacore_init(ctxptr->via1d1541, ctxptr->cpu->alarm_context,
                 ctxptr->cpu->int_status);
}

void via1d1541_setup_context(diskunit_context_t *ctxptr)
{
    drivevia1_context_t *via1p;
    via_context_t *via;

    /* Clear struct as snapshot code may write uninitialized values.  */
    ctxptr->via1d1541 = lib_calloc(1, sizeof(via_context_t));
    via = ctxptr->via1d1541;

    via->prv = lib_malloc(sizeof(drivevia1_context_t));
    via1p = (drivevia1_context_t *)(via->prv);
    via1p->number = ctxptr->mynumber;

    via->context = (void *)ctxptr;

    via->rmw_flag = &(ctxptr->cpu->rmw_flag);
    via->clk_ptr = ctxptr->clk_ptr;

    via->myname = lib_msprintf("1541Drive%uVia1", ctxptr->mynumber);
    via->my_module_name = lib_msprintf("1541VIA1D%u", ctxptr->mynumber);

    viacore_setup_context(via);

    via->my_module_name_alt1 = lib_msprintf("VIA1D%u", ctxptr->mynumber);
    via->my_module_name_alt2 = lib_msprintf("VIA1D1541");

    via->irq_line = IK_IRQ;

    via1p->drive = ctxptr->drives[0];
    via1p->diskunit = ctxptr;

    iecbus = iecbus_drive_port();

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
