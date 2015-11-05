/*
 * vic20via2.c - VIA2 emulation in the VIC20.
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
 *
 */

#include "vice.h"

#include <stdio.h>

#include "datasette.h"
#include "interrupt.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "maincpu.h"
#include "printer.h"
#include "types.h"
#include "userport_joystick.h"
#include "via.h"
#include "vic.h"
#include "vic20.h"
#include "vic20-resources.h"
#include "vic20iec.h"
#include "vic20via.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "rsuser.h"
#endif


void via2_store(WORD addr, BYTE data)
{
    viacore_store(machine_context.via2, addr, data);
}

BYTE via2_read(WORD addr)
{
    return viacore_read(machine_context.via2, addr);
}

BYTE via2_peek(WORD addr)
{
    return viacore_peek(machine_context.via2, addr);
}

static void set_ca2(via_context_t *via_context, int state)
{
}

static void set_cb2(via_context_t *via_context, int state)
{
}

static void set_int(via_context_t *via_context, unsigned int int_num, int value, CLOCK rclk)
{
    interrupt_set_nmi(maincpu_int_status, int_num, value, rclk);
}

static void restore_int(via_context_t *via_context, unsigned int int_num, int value)
{
    interrupt_restore_nmi(maincpu_int_status, int_num, value);
}

static int tape_sense = 0;

void via2_set_tape_sense(int v)
{
    tape_sense = v;
}

static void via2_internal_lightpen_check(BYTE pa)
{
    BYTE b = ~(joystick_value[1] | joystick_value[2]);

    b &= pa;

    vic_set_light_pen(maincpu_clk, !(b & 0x20));
}

void via2_check_lightpen(void)
{
    BYTE pa = machine_context.via2->via[VIA_PRA] | ~(machine_context.via2->via[VIA_DDRA]);

    via2_internal_lightpen_check(pa);
}

static void undump_pra(via_context_t *via_context, BYTE byte)
{
    iec_pa_write(byte);
}

static void store_pra(via_context_t *via_context, BYTE byte, BYTE myoldpa,
                      WORD addr)
{
    via2_internal_lightpen_check(byte);
    iec_pa_write(byte);
}

static void undump_prb(via_context_t *via_context, BYTE byte)
{
    printer_userport_write_data(byte);
}

static void store_prb(via_context_t *via_context, BYTE byte, BYTE myoldpb,
                      WORD addr)
{
    /* FIXME: in the upcoming userport system this call needs to be conditional */
    userport_joystick_store_pbx(byte);

    printer_userport_write_data(byte);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl(byte);
#endif
}

static void undump_pcr(via_context_t *via_context, BYTE byte)
{
}

static void reset(via_context_t *via_context)
{
/*    iec_pa_write(0xff);*/

    printer_userport_write_data(0xff);
    printer_userport_write_strobe(1);
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl(0xff);
    rsuser_set_tx_bit(1);
#endif
}

static BYTE store_pcr(via_context_t *via_context, BYTE byte, WORD addr)
{
    /* FIXME: should use via_set_ca2() and via_set_cb2() */
    if (byte != via_context->via[VIA_PCR]) {
        register BYTE tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((tmp & 0x0c) != 0x0c) {
            tmp |= 0x02;
        }
        if ((tmp & 0xc0) != 0xc0) {
            tmp |= 0x20;
        }

        datasette_set_motor(!(byte & 0x02));

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
        /* switching userport strobe with CB2 */
        if (rsuser_enabled) {
            rsuser_set_tx_bit(byte & 0x20);
        }
#endif
        printer_userport_write_strobe(byte & 0x20);
    }
    return byte;
}

static void undump_acr(via_context_t *via_context, BYTE byte)
{
}

inline static void store_acr(via_context_t *via_context, BYTE byte)
{
}

inline static void store_sr(via_context_t *via_context, BYTE byte)
{
}

inline static void store_t2l(via_context_t *via_context, BYTE byte)
{
}

inline static BYTE read_pra(via_context_t *via_context, WORD addr)
{
    BYTE byte;
    BYTE joy_bits;

    /*
        Port A is connected this way:

        bit 0  IEC clock
        bit 1  IEC data
        bit 2  joystick switch 0 (up)
        bit 3  joystick switch 1 (down)
        bit 4  joystick switch 2 (left)
        bit 5  joystick switch 4 (fire)
        bit 6  tape sense
        bit 7  IEC ATN
    */

    /* Setup joy bits (2 through 5).  Use the `or' of the values
       of both joysticks so that it works with every joystick
       setting.  This is a bit slow... we might think of a
       faster method.  */
    joy_bits = ~(joystick_value[1] | joystick_value[2]);
    joy_bits = ((joy_bits & 0x7) << 2) | ((joy_bits & 0x10) << 1);

    joy_bits |= tape_sense ? 0 : 0x40;

    /* We assume `iec_pa_read()' returns the non-IEC bits
       as zeroes. */
    byte = ((via_context->via[VIA_PRA] & via_context->via[VIA_DDRA])
            | ((iec_pa_read() | joy_bits) & ~(via_context->via[VIA_DDRA])));
    return byte;
}

inline static BYTE read_prb(via_context_t *via_context)
{
    BYTE byte;
    byte = via_context->via[VIA_PRB] | ~(via_context->via[VIA_DDRB]);

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (rsuser_enabled) {
        byte = rsuser_read_ctrl();
    }
#endif

    /* FIXME: in the upcoming userport system this call needs to be conditional */
    byte = userport_joystick_read_pbx(byte);

    return byte;
}

void via2_init(via_context_t *via_context)
{
    viacore_init(machine_context.via2, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void vic20via2_setup_context(machine_context_t *machine_context)
{
    via_context_t *via;

    machine_context->via2 = lib_malloc(sizeof(via_context_t));
    via = machine_context->via2;

    via->prv = NULL;
    via->context = NULL;

    via->rmw_flag = &maincpu_rmw_flag;
    via->clk_ptr = &maincpu_clk;

    via->myname = lib_msprintf("Via2");
    via->my_module_name = lib_msprintf("VIA2");

    viacore_setup_context(via);

    via->write_offset = 0;

    via->irq_line = IK_NMI;

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
