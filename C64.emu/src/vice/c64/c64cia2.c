/*
 * c64cia2.c - Definitions for the second MOS6526 (CIA) chip in the C64
 * ($DD00).
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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
 * */


#include "vice.h"

#include <stdio.h>

#include "c64fastiec.h"
#include "c64-resources.h"
#include "c64.h"
#include "c64mem.h"
#include "c64iec.h"
#include "c64cia.h"
#include "c64gluelogic.h"
#include "c64parallel.h"
#include "cia.h"
#include "drive.h"
#include "iecbus.h"
#include "interrupt.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "printer.h"
#include "types.h"
#include "userport_digimax.h"
#include "userport_joystick.h"
#include "userport_rtc.h"
#include "vicii.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "rsuser.h"
#endif

/* Flag for recording port A DDR changes (for c64gluelogic) */
static int pa_ddr_change = 0;

void cia2_store(WORD addr, BYTE data)
{
    if (((addr & 0xf) == CIA_DDRA) && (machine_context.cia2->c_cia[CIA_DDRA] != data)) {
        pa_ddr_change = 1;
    } else {
        pa_ddr_change = 0;
    }

    digimax_userport_store(addr, data);
    ciacore_store(machine_context.cia2, addr, data);
}

BYTE cia2_read(WORD addr)
{
    return ciacore_read(machine_context.cia2, addr);
}

BYTE cia2_peek(WORD addr)
{
    return ciacore_peek(machine_context.cia2, addr);
}

static void cia_set_int_clk(cia_context_t *cia_context, int value, CLOCK clk)
{
    interrupt_set_nmi(maincpu_int_status, cia_context->int_num, value, clk);
}

static void cia_restore_int(cia_context_t *cia_context, int value)
{
    interrupt_restore_nmi(maincpu_int_status, cia_context->int_num, value);
}

void cia2_update_model(void)
{
    if (machine_context.cia2) {
        machine_context.cia2->model = cia2_model;
    }
}

#define MYCIA CIA2

/*************************************************************************
 * I/O
 */

/* Current video bank (0, 1, 2 or 3).  */
static int vbank;

static void do_reset_cia(cia_context_t *cia_context)
{
    printer_userport_write_strobe(1);
    printer_userport_write_data((BYTE)0xff);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl((BYTE)0xff);
    rsuser_set_tx_bit(1);
#endif

    vbank = 0;
    c64_glue_reset();
}

static void pre_store(void)
{
    vicii_handle_pending_alarms_external_write();
}

static void pre_read(void)
{
    vicii_handle_pending_alarms_external(0);
}

static void pre_peek(void)
{
    vicii_handle_pending_alarms_external(0);
}

static void store_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    if (cia_context->old_pa != byte) {
        BYTE tmp;
        int new_vbank;

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
        if (rsuser_enabled && ((cia_context->old_pa ^ byte) & 0x04)) {
            rsuser_set_tx_bit(byte & 4);
        }
#endif
        tmp = ~byte;
        new_vbank = tmp & 3;
        if (new_vbank != vbank) {
            vbank = new_vbank;
            c64_glue_set_vbank(new_vbank, pa_ddr_change);
        }
        (*iecbus_callback_write)((BYTE)tmp, maincpu_clk + !(cia_context->write_offset));
        printer_userport_write_strobe(tmp & 0x04);
    }
}

static void undump_ciapa(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (rsuser_enabled) {
        rsuser_set_tx_bit((int)(byte & 4));
    }
#endif
    vbank = (byte ^ 3) & 3;
    c64_glue_undump(vbank);

    iecbus_cpu_undump((BYTE)(byte ^ 0xff));
}


static void store_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    parallel_cable_cpu_write(DRIVE_PC_STANDARD, (BYTE)byte);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl((BYTE)byte);
#endif
    /* FIXME: in the upcoming userport system these calls needs to be conditional */
    userport_joystick_store_pbx(byte);
    userport_rtc_store(byte);
}

static void pulse_ciapc(cia_context_t *cia_context, CLOCK rclk)
{
    parallel_cable_cpu_pulse(DRIVE_PC_STANDARD);
    printer_userport_write_data((BYTE)(cia_context->old_pb));
}

/* FIXME! */
static inline void undump_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    parallel_cable_cpu_undump(DRIVE_PC_STANDARD, (BYTE)byte);
    printer_userport_write_data((BYTE)byte);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl((BYTE)byte);
#endif
    /* FIXME: in the upcoming userport system these calls needs to be conditional */
    userport_joystick_store_pbx(byte);
    userport_rtc_store(byte);
}

/* read_* functions must return 0xff if nothing to read!!! */
static BYTE read_ciapa(cia_context_t *cia_context)
{
    BYTE value;

    value = ((cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA])) & 0x3f) | (*iecbus_callback_read)(maincpu_clk);

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    value = userport_joystick_read_pa2(value);
    return value;
}

/* read_* functions must return 0xff if nothing to read!!! */
static BYTE read_ciapb(cia_context_t *cia_context)
{
    BYTE byte;
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (rsuser_enabled) {
        byte = rsuser_read_ctrl();
    } else
#endif
    byte = parallel_cable_cpu_read(DRIVE_PC_STANDARD);

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    byte = userport_joystick_read_pbx(byte);
    byte = userport_rtc_read(byte);

    byte = (byte & ~(cia_context->c_cia[CIA_DDRB])) | (cia_context->c_cia[CIA_PRB] & cia_context->c_cia[CIA_DDRB]);
    return byte;
}

static void read_ciaicr(cia_context_t *cia_context)
{
    if (burst_mod == BURST_MOD_CIA2) {
        drive_cpu_execute_all(maincpu_clk);
    }
    parallel_cable_cpu_execute(DRIVE_PC_STANDARD);
}

static void read_sdr(cia_context_t *cia_context)
{
    if (burst_mod == BURST_MOD_CIA2) {
        drive_cpu_execute_all(maincpu_clk);
    }
    /* FIXME: in the upcomming userport system this call needs to be conditional */
    cia_context->c_cia[CIA_SDR] = userport_joystick_read_sdr(cia_context->c_cia[CIA_SDR]);
}

static void store_sdr(cia_context_t *cia_context, BYTE byte)
{
    if (burst_mod == BURST_MOD_CIA2) {
        c64fastiec_fast_cpu_write((BYTE)byte);
    }
}

/* Temporary!  */
void cia2_set_flagx(void)
{
    ciacore_set_flag(machine_context.cia2);
}

void cia2_set_sdrx(BYTE received_byte)
{
    ciacore_set_sdr(machine_context.cia2, received_byte);
}

void cia2_init(cia_context_t *cia_context)
{
    ciacore_init(machine_context.cia2, maincpu_alarm_context, maincpu_int_status, maincpu_clk_guard);
}

void cia2_setup_context(machine_context_t *machine_context)
{
    cia_context_t *cia;

    machine_context->cia2 = lib_calloc(1, sizeof(cia_context_t));
    cia = machine_context->cia2;

    cia->prv = NULL;
    cia->context = NULL;

    cia->rmw_flag = &maincpu_rmw_flag;
    cia->clk_ptr = &maincpu_clk;

    cia2_set_timing(cia, C64_PAL_CYCLES_PER_SEC, 50);

    ciacore_setup_context(cia);

    if (machine_class == VICE_MACHINE_C64SC
        || machine_class == VICE_MACHINE_SCPU64) {
        cia->write_offset = 0;
    }

    cia->model = cia2_model;

    cia->debugFlag = 0;
    cia->irq_line = IK_NMI;
    cia->myname = lib_msprintf("CIA2");

    cia->undump_ciapa = undump_ciapa;
    cia->undump_ciapb = undump_ciapb;
    cia->store_ciapa = store_ciapa;
    cia->store_ciapb = store_ciapb;
    cia->store_sdr = store_sdr;
    cia->read_ciapa = read_ciapa;
    cia->read_ciapb = read_ciapb;
    cia->read_ciaicr = read_ciaicr;
    cia->read_sdr = read_sdr;
    cia->cia_set_int_clk = cia_set_int_clk;
    cia->cia_restore_int = cia_restore_int;
    cia->do_reset_cia = do_reset_cia;
    cia->pulse_ciapc = pulse_ciapc;
    cia->pre_store = pre_store;
    cia->pre_read = pre_read;
    cia->pre_peek = pre_peek;
}

void cia2_set_timing(cia_context_t *cia_context, int tickspersec, int powerfreq)
{
    cia_context->power_freq = powerfreq;
    cia_context->ticks_per_sec = tickspersec;
    cia_context->todticks = tickspersec / powerfreq;
    cia_context->power_tickcounter = 0;
    cia_context->power_ticks = 0;
}
