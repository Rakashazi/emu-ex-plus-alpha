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
#include "joyport.h"
#include "joystick.h"
#include "keyboard.h"
#include "lib.h"
#include "maincpu.h"
#include "tapeport.h"
#include "types.h"
#include "userport.h"
#include "via.h"
#include "vic.h"
#include "vic20.h"
#include "vic20-resources.h"
#include "vic20iec.h"
#include "vic20via.h"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#include "rsuser.h"
#endif

int vic20_vflihack_userport = 0xff;

void via2_store(uint16_t addr, uint8_t data)
{
    viacore_store(machine_context.via2, addr, data);
}

uint8_t via2_read(uint16_t addr)
{
    return viacore_read(machine_context.via2, addr);
}

uint8_t via2_peek(uint16_t addr)
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
static int tape_write_in = 0;
static int tape_motor_in = 0;

void via2_set_tape_sense(int v)
{
    tape_sense = v;
}

/* FIXME: find out how to set the write in and motor in lines */
void via2_set_tape_write_in(int v)
{
    tape_write_in = v;
}

void via2_set_tape_motor_in(int v)
{
    tape_motor_in = v;
}

static void via2_internal_lightpen_check(uint8_t pa)
{
    uint8_t b = read_joyport_dig(JOYPORT_1);

    b &= pa;

    vic_set_light_pen(maincpu_clk, !(b & 0x20));
}

void via2_check_lightpen(void)
{
    uint8_t pa = machine_context.via2->via[VIA_PRA] | ~(machine_context.via2->via[VIA_DDRA]);

    via2_internal_lightpen_check(pa);
}

static void undump_pra(via_context_t *via_context, uint8_t byte)
{
    iec_pa_write(byte);
}

static void store_pra(via_context_t *via_context, uint8_t byte, uint8_t myoldpa,
                      uint16_t addr)
{
    uint8_t joy_bits = 0;

    via2_internal_lightpen_check(byte);
    iec_pa_write(byte);

    joy_bits = ((byte & 0x20) >> 1) | ((byte & 0x1c) >> 2);
    store_joyport_dig(JOYPORT_1, joy_bits, 0x17);

    tapeport_set_sense_out(byte & 0x40 ? 1 : 0);
}

static void undump_prb(via_context_t *via_context, uint8_t byte)
{
    store_userport_pbx(byte);
}

static void store_prb(via_context_t *via_context, uint8_t byte, uint8_t myoldpb,
                      uint16_t addr)
{
    /* for mike's VFLI hack, PB0-PB3 are used as A10-A13 of the color ram */
    vic20_vflihack_userport = byte & 0x0f;

    store_userport_pbx(byte);

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl(byte);
#endif
}

static void undump_pcr(via_context_t *via_context, uint8_t byte)
{
}

static void reset(via_context_t *via_context)
{
    store_userport_pbx(0xff);
    store_userport_pa2(1);

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    rsuser_write_ctrl(0xff);
    rsuser_set_tx_bit(1);
#endif
}

static uint8_t store_pcr(via_context_t *via_context, uint8_t byte, uint16_t addr)
{
    /* FIXME: should use via_set_ca2() and via_set_cb2() */
    if (byte != via_context->via[VIA_PCR]) {
        register uint8_t tmp = byte;
        /* first set bit 1 and 5 to the real output values */
        if ((tmp & 0x0c) != 0x0c) {
            tmp |= 0x02;
        }
        if ((tmp & 0xc0) != 0xc0) {
            tmp |= 0x20;
        }

        tapeport_set_motor(!(byte & 0x02));

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
        /* switching userport strobe with CB2 */
        if (rsuser_enabled) {
            rsuser_set_tx_bit(byte & 0x20);
        }
#endif
        store_userport_pa2((uint8_t)((byte & 0x20) >> 5));
    }
    return byte;
}

static void undump_acr(via_context_t *via_context, uint8_t byte)
{
}

inline static void store_acr(via_context_t *via_context, uint8_t byte)
{
}

inline static void store_sr(via_context_t *via_context, uint8_t byte)
{
}

inline static void store_t2l(via_context_t *via_context, uint8_t byte)
{
}

inline static uint8_t read_pra(via_context_t *via_context, uint16_t addr)
{
    uint8_t byte;
    uint8_t joy_bits;

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
    joy_bits = read_joyport_dig(JOYPORT_1);
    joy_bits = ((joy_bits & 0x7) << 2) | ((joy_bits & 0x10) << 1);

    joy_bits |= tape_sense ? 0 : 0x40;

    /* We assume `iec_pa_read()' returns the non-IEC bits
       as zeroes. */
    byte = ((via_context->via[VIA_PRA] & via_context->via[VIA_DDRA])
            | ((iec_pa_read() | joy_bits) & ~(via_context->via[VIA_DDRA])));
    return byte;
}

inline static uint8_t read_prb(via_context_t *via_context)
{
    uint8_t byte = 0xff;
    byte = via_context->via[VIA_PRB] | ~(via_context->via[VIA_DDRB]);

    byte = read_userport_pbx((uint8_t)~via_context->via[VIA_DDRB], byte);

    /* The functions below will gradually be removed as the functionality is added to the new userport system. */
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (rsuser_enabled) {
        byte = rsuser_read_ctrl(byte);
    }
#endif

    return byte;
}

void via2_init(via_context_t *via_context)
{
    viacore_init(machine_context.via2, maincpu_alarm_context,
                 maincpu_int_status, maincpu_clk_guard);
}

void vic20via2_setup_context(machine_context_t *machinecontext)
{
    via_context_t *via;

    machinecontext->via2 = lib_malloc(sizeof(via_context_t));
    via = machinecontext->via2;

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
