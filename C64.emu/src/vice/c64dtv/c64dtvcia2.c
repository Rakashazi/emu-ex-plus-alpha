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

#include "c64.h"
#include "c64mem.h"
#include "c64iec.h"
#include "c64cia.h"
#include "c64parallel.h"
#include "c64dtv-resources.h"
#include "cia.h"
#include "drive.h"
#include "hummeradc.h"
#include "iecbus.h"
#include "interrupt.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "printer.h"
#include "ps2mouse.h"
#include "types.h"
#include "userport_joystick.h"
#include "vicii.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "rsuser.h"
#endif


void cia2_store(WORD addr, BYTE data)
{
    if ((addr & 0x1f) == 1) {
        /* FIXME: in the upcoming userport system this call needs to be conditional */
        userport_joystick_store_pbx(data);

        if (c64dtv_hummer_adc_enabled) {
            hummeradc_store(data);
        }
        if (ps2mouse_enabled) {
            ps2mouse_store(data);
        }
    }

    ciacore_store(machine_context.cia2, addr, data);
}

BYTE cia2_read(WORD addr)
{
    BYTE retval = 0xff;

    if ((addr & 0x1f) == 1) {
        /* FIXME: in the upcoming userport system this call needs to be conditional */
        retval = userport_joystick_read_pbx(retval);

        if (ps2mouse_enabled) {
            retval &= (ps2mouse_read() | 0x3f);
        }
        if (c64dtv_hummer_adc_enabled) {
            retval &= (hummeradc_read() | 0xf8);
        }
        return retval;
    }

    /* disable TOD & serial */
    if (((addr & 0xf) >= 8) && ((addr & 0xf) <= 0xc)) {
        return 0xff;
    }

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
    mem_set_vbank(vbank);
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
            mem_set_vbank(new_vbank);
        }
        (*iecbus_callback_write)((BYTE)tmp, maincpu_clk);
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
    mem_set_vbank(vbank);
    iecbus_cpu_undump((BYTE)(byte ^ 0xff));
}


static void store_ciapb(cia_context_t *cia_context, CLOCK rclk, BYTE byte)
{
    parallel_cable_cpu_write(DRIVE_PC_STANDARD, (BYTE)byte);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl((BYTE)byte);
#endif
}

static void pulse_ciapc(cia_context_t *cia_context, CLOCK rclk)
{
    parallel_cable_cpu_pulse(DRIVE_PC_STANDARD);
    printer_userport_write_data((BYTE)(cia_context->old_pb));
}

/* FIXME! */
static inline void undump_ciapb(cia_context_t *cia_context, CLOCK rclk,
                                BYTE byte)
{
    parallel_cable_cpu_undump(DRIVE_PC_STANDARD, (BYTE)byte);
    printer_userport_write_data((BYTE)byte);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl((BYTE)byte);
#endif
    /* in the upcoming userport system this call needs to be conditional */
    userport_joystick_store_pbx(byte);
}

/* read_* functions must return 0xff if nothing to read!!! */
static BYTE read_ciapa(cia_context_t *cia_context)
{
    return ((cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]))
            & 0x3f) | (*iecbus_callback_read)(maincpu_clk);
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

    byte = (byte & ~(cia_context->c_cia[CIA_DDRB]))
           | (cia_context->c_cia[CIA_PRB] & cia_context->c_cia[CIA_DDRB]);
    return byte;
}

static void read_ciaicr(cia_context_t *cia_context)
{
    parallel_cable_cpu_execute(DRIVE_PC_STANDARD);
}

static void read_sdr(cia_context_t *cia_context)
{
}

static void store_sdr(cia_context_t *cia_context, BYTE byte)
{
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
    ciacore_init(machine_context.cia2, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void cia2_set_timing(cia_context_t *cia_context, int tickspersec, int powerfreq)
{
    cia_context->power_freq = powerfreq;
    cia_context->ticks_per_sec = tickspersec;
    cia_context->todticks = 0;
    cia_context->power_tickcounter = 0;
    cia_context->power_ticks = 0;
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

    cia2_set_timing(cia, C64_PAL_CYCLES_PER_SEC, 0);

    ciacore_setup_context(cia);

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
