/*
 * petvia.c - VIA emulation in the PET.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "crtc.h"
#include "datasette.h"
#include "drive.h"
#include "interrupt.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "parallel.h"
#include "pet.h"
#include "petsound.h"
#include "petvia.h"
#include "printer.h"
#include "types.h"
#include "userport_dac.h"
#include "userport_joystick.h"
#include "via.h"


void via_store(WORD addr, BYTE data)
{
    viacore_store(machine_context.via, addr, data);
}

BYTE via_read(WORD addr)
{
    return viacore_read(machine_context.via, addr);
}

BYTE via_peek(WORD addr)
{
    return viacore_peek(machine_context.via, addr);
}

/* switching PET charrom with CA2 */
static void set_ca2(via_context_t *via_context, int state)
{
    crtc_set_chargen_offset(state ? 256 : 0);
}

/* switching userport strobe with CB2 */
static void set_cb2(via_context_t *via_context, int state)
{
    printer_userport_write_strobe(state);
}

static void set_int(via_context_t *via_context, unsigned int int_num,
                    int value, CLOCK rclk)
{
    interrupt_set_irq(maincpu_int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num, int value)
{
    interrupt_restore_irq(maincpu_int_status, int_num, value);
}

static void undump_pra(via_context_t *via_context, BYTE byte)
{
    printer_userport_write_data(byte);

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    userport_joystick_store_pbx(byte);

    userport_dac_store(byte);
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE myoldpa,
                      WORD addr)
{
    printer_userport_write_data(byte);

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    userport_joystick_store_pbx(byte);

    userport_dac_store(byte);
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
    parallel_cpu_set_nrfd((BYTE)(!(byte & 0x02)));
    parallel_cpu_restore_atn((BYTE)(!(byte & 0x04)));
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE myoldpb,
                      WORD addr)
{
    if ((addr == VIA_DDRB) && (via_context->via[addr] & 0x20)) {
        log_warning(via_context->log,
                    "PET: Killer POKE! might kill a real PET!\n");
    }
    parallel_cpu_set_nrfd((BYTE)(!(byte & 0x02)));
    parallel_cpu_set_atn((BYTE)(!(byte & 0x04)));
    if ((byte ^ myoldpb) & 0x8) {
        datasette_toggle_write_bit((~(via_context->via[VIA_DDRB]) | byte) & 0x8);
    }
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
#if 0
    register BYTE tmp = byte;
    /* first set bit 1 and 5 to the real output values */
    if ((tmp & 0x0c) != 0x0c) {
        tmp |= 0x02;
    }
    if ((tmp & 0xc0) != 0xc0) {
        tmp |= 0x20;
    }
    crtc_set_char(byte & 2); /* switching PET charrom with CA2 */
                             /* switching userport strobe with CB2 */
#endif
    petsound_store_manual((byte & 0xe0) == 0xe0);   /* Manual control of CB2 sound */
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
#if 0
    if (byte != via_context->via[VIA_PCR]) {
        register BYTE tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((tmp & 0x0c) != 0x0c) {
            tmp |= 0x02;
        }
        if ((tmp & 0xc0) != 0xc0) {
            tmp |= 0x20;
        }
        crtc_set_char(byte & 2); /* switching PET charrom with CA2 */
                                 /* switching userport strobe with CB2 */
        printer_userport_write_strobe(byte & 0x20);
    }
#endif
    petsound_store_manual((byte & 0xe0) == 0xe0);   /* Manual control of CB2 sound */
    return byte;
}

static void undump_acr(via_context_t *via_context, BYTE byte)
{
    petsound_store_onoff(via_context->via[VIA_T2LL]
                         ? (((byte & 0x1c) == 0x10) ? 1 : 0) : 0);
}

static void store_acr(via_context_t *via_context, BYTE byte)
{
    petsound_store_onoff(via_context->via[VIA_T2LL]
                         ? (((byte & 0x1c) == 0x10) ? 1 : 0) : 0);
}

static void store_sr(via_context_t *via_context, BYTE byte)
{
    petsound_store_sample(byte);
}

static void store_t2l(via_context_t *via_context, BYTE byte)
{
    petsound_store_rate(2 * byte + 4);
    if (!byte) {
        petsound_store_onoff(0);
    } else {
        petsound_store_onoff(((via_context->via[VIA_ACR] & 0x1c) == 0x10)
                             ? 1 : 0);
    }
}

static void reset(via_context_t *via_context)
{
    /* set IEC output lines */
    parallel_cpu_set_atn(0);
    parallel_cpu_set_nrfd(0);

    printer_userport_write_data(0xff);
    printer_userport_write_strobe(1);
}

inline static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    BYTE byte = 0xff;

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    userport_joystick_read_pbx(byte);

    /* joystick always pulls low, even if high output, so no
       masking with DDRA */
    /*return ((j & ~(via_context->via[VIA_DDRA]))
        | (via_context->via[VIA_PRA] & via_context->via[VIA_DDRA]));*/
    return byte;
}

static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    /* read parallel IEC interface line states */
    byte = 255
           - (parallel_nrfd ? 64 : 0)
           - (parallel_ndac ? 1 : 0)
           - (parallel_dav ? 128 : 0);
    /* vertical retrace */
    byte -= crtc_offscreen() ? 32 : 0;

    /* none of the load changes output register value -> std. masking */
    byte = ((byte & ~(via_context->via[VIA_DDRB]))
            | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]));
    return byte;
}

void via_init(via_context_t *via_context)
{
    viacore_init(machine_context.via, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void petvia_setup_context(machine_context_t *machine_context)
{
    via_context_t *via;

    machine_context->via = lib_malloc(sizeof(via_context_t));
    via = machine_context->via;

    via->prv = NULL;
    via->context = NULL;

    via->rmw_flag = &maincpu_rmw_flag;
    via->clk_ptr = &maincpu_clk;

    via->myname = lib_msprintf("Via");
    via->my_module_name = lib_msprintf("VIA");

    viacore_setup_context(via);

    via->irq_line = IK_IRQ;

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
