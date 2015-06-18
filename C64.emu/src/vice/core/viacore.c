/*
 * viacore.c - Core functions for VIA emulation.
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
#include <string.h>

#include "alarm.h"
#include "clkguard.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "snapshot.h"
#include "types.h"
#include "via.h"


/*
 * 24jan97 a.fachat
 * new interrupt handling, hopefully according to the specs now.
 * All interrupts (note: not timer events (i.e. alarms) are put
 * into one interrupt flag.
 * if an interrupt condition changes, the function (i.e. cpp macro)
 * update_myviairq() id called, that checks the IRQ line state.
 * This is now possible, as ettore has decoupled A_* alarm events
 * from interrupts for performance reasons.
 *
 * A new function for signaling rising/falling edges on the
 * control lines is introduced:
 *      myvia_signal(VIA_SIG_[CA1|CA2|CB1|CB2], VIA_SIG_[RISE|FALL])
 * which signals the corresponding edge to the VIA. The constants
 * are defined in via.h.
 *
 * Except for shift register and input latching everything should be ok now.
 */

/* Timer debugging */
/*#define MYVIA_TIMER_DEBUG */
/* when PB7 is really used, set this
   to enable pulse output from the timer.
   Otherwise PB7 state is computed only
   when port B is read -
   not yet implemented */
#define MYVIA_NEED_PB7
/* When you really need latching, define this.
   It implies additional READ_PR* when
   writing the snapshot. When latching is
   enabled: it reads the port when enabling,
   and when an active C*1 transition occurs.
   It does not read the port when reading the
   port register. Side-effects beware! */
/* FIXME: this doesnt even work anymore */
/* #define MYVIA_NEED_LATCHING */

/*
 * local functions
 */

#define IS_CA2_OUTPUT()          ((via_context->via[VIA_PCR] & 0x0c) == 0x0c)
#define IS_CA2_INDINPUT()        ((via_context->via[VIA_PCR] & 0x0a) == 0x02)
#define IS_CA2_HANDSHAKE()       ((via_context->via[VIA_PCR] & 0x0c) == 0x08)
#define IS_CA2_PULSE_MODE()      ((via_context->via[VIA_PCR] & 0x0e) == 0x09)
#define IS_CA2_TOGGLE_MODE()     ((via_context->via[VIA_PCR] & 0x0e) == 0x08)

#define IS_CB2_OUTPUT()          ((via_context->via[VIA_PCR] & 0xc0) == 0xc0)
#define IS_CB2_INDINPUT()        ((via_context->via[VIA_PCR] & 0xa0) == 0x20)
#define IS_CB2_HANDSHAKE()       ((via_context->via[VIA_PCR] & 0xc0) == 0x80)
#define IS_CB2_PULSE_MODE()      ((via_context->via[VIA_PCR] & 0xe0) == 0x90)
#define IS_CB2_TOGGLE_MODE()     ((via_context->via[VIA_PCR] & 0xe0) == 0x80)

#define IS_PA_INPUT_LATCH()      (via_context->via[VIA_ACR] & 0x01)
#define IS_PB_INPUT_LATCH()      (via_context->via[VIA_ACR] & 0x02)

/*
 * 01apr98 a.fachat
 *
 * One-shot Timing (partly from 6522-VIA.txt):

                     +-+ +-+ +-+ +-+ +-+ +-+   +-+ +-+ +-+ +-+ +-+ +-+
                02 --+ +-+ +-+ +-+ +-+ +-+ +-#-+ +-+ +-+ +-+ +-+ +-+ +-
                       |   |                           |
                       +---+                           |
       WRITE T1C-H ----+   +-----------------#-------------------------
        ___                |                           |
        IRQ OUTPUT --------------------------#---------+
                           |                           +---------------
                           |                           |
        PB7 OUTPUT --------+                           +---------------
                           +-----------------#---------+
         T1                | N |N-1|N-2|N-3|     | 0 | -1|N  |N-1|N-2|
         T2                | N |N-1|N-2|N-3|     | 0 | -1| -2| -3| -4|
                           |                           |
                           |<---- N + 1.5 CYCLES ----->|<--- N + 2 cycles --->
                                                         +---+
 myviat*u* clk ------------------------------------------+   +--------
                                                     |
                                                     |
                                                  call of
                                                int_myvia*
                                                   here

   real myviatau value = myviatau* + TAUOFFSET
   myviatbu = myviatbu* + 0

 *
 * IRQ and PB7 are set/toggled at the low-high transition of Phi2,
 * but int_* is called a half-cycle before that. Does that matter?
 *
 * PB7 output is still to be implemented
 */

/* timer values do not depend on a certain value here, but PB7 does... */
#define TAUOFFSET       (-1)


static void viacore_intt1(CLOCK offset, void *data);
static void viacore_intt2(CLOCK offset, void *data);


static void via_restore_int(via_context_t *via_context, int value)
{
    (via_context->restore_int)(via_context, via_context->int_num, value);
}

inline static void update_myviairq(via_context_t *via_context)
{
    (via_context->set_int)(via_context, via_context->int_num,
                           (via_context->ifr & via_context->ier & 0x7f)
                           ? via_context->irq_line : 0, *(via_context->clk_ptr));
}

inline static void update_myviairq_rclk(via_context_t *via_context, CLOCK rclk)
{
    (via_context->set_int)(via_context, via_context->int_num,
                           (via_context->ifr & via_context->ier & 0x7f)
                           ? via_context->irq_line : 0, rclk);
}

/* the next two are used in myvia_read() */

inline static CLOCK myviata(via_context_t *via_context)
{
    if (*(via_context->clk_ptr) < via_context->tau - TAUOFFSET) {
        return via_context->tau - TAUOFFSET - *(via_context->clk_ptr) - 2;
    } else {
        return (via_context->tal - (*(via_context->clk_ptr) - via_context->tau
                                    + TAUOFFSET) % (via_context->tal + 2));
    }
}

inline static CLOCK myviatb(via_context_t *via_context)
{
    CLOCK t2;

    if (via_context->via[VIA_ACR] & 0x20) {
        t2 = (via_context->t2ch << 8) | via_context->t2cl;
    } else {
        t2 = via_context->tbu - *(via_context->clk_ptr) - 2;

        if (via_context->tbi) {
            BYTE t2hi = via_context->t2ch;

            if (*(via_context->clk_ptr) == via_context->tbi) {
                t2hi--;
            }

            t2 = (t2hi << 8) | (t2 & 0xff);
        }
    }

    return t2;
}

inline static void update_myviatal(via_context_t *via_context, CLOCK rclk)
{
    via_context->pb7x = 0;
    via_context->pb7xx = 0;

    if (rclk > via_context->tau) {
        int nuf = (via_context->tal + 1 + rclk - via_context->tau)
                  / (via_context->tal + 2);

        if (!(via_context->via[VIA_ACR] & 0x40)) {
            if (((nuf - via_context->pb7sx) > 1) || (!(via_context->pb7))) {
                via_context->pb7o = 1;
                via_context->pb7sx = 0;
            }
        }
        via_context->pb7 ^= (nuf & 1);

        via_context->tau = TAUOFFSET + via_context->tal + 2
                   + (rclk - (rclk - via_context->tau + TAUOFFSET)
                   % (via_context->tal + 2));
        if (rclk == via_context->tau - via_context->tal - 1) {
            via_context->pb7xx = 1;
        }
    }

    if (via_context->tau == rclk) {
        via_context->pb7x = 1;
    }

    via_context->tal = via_context->via[VIA_T1LL]
                       + (via_context->via[VIA_T1LH] << 8);
}

/* ------------------------------------------------------------------------- */
void viacore_disable(via_context_t *via_context)
{
    alarm_unset(via_context->t1_alarm);
    alarm_unset(via_context->t2_alarm);
    via_context->enabled = 0;
}

/*
 * according to Rockwell, all internal registers are cleared, except
 * for the Timer (1 and 2, counter and latches) and the shift register.
 */
void viacore_reset(via_context_t *via_context)
{
    int i;

    /* clear registers */
    for (i = 0; i < 4; i++) {
        via_context->via[i] = 0;
    }
    for (i = 4; i < 10; i++) {
        via_context->via[i] = 0xff;
    }
    for (i = 11; i < 16; i++) {
        via_context->via[i] = 0;
    }

    via_context->tal = 0xffff;
    via_context->t2cl = 0xff;
    via_context->t2ch = 0xff;
    via_context->tau = *(via_context->clk_ptr);
    via_context->tbu = *(via_context->clk_ptr);

    via_context->read_clk = 0;

    via_context->ier = 0;
    via_context->ifr = 0;

    via_context->pb7 = 0;
    via_context->pb7x = 0;
    via_context->pb7o = 0;
    via_context->pb7xx = 0;
    via_context->pb7sx = 0;

    via_context->shift_state = 0;

    /* disable vice interrupts */
    via_context->tai = 0;
    via_context->tbi = 0;
    alarm_unset(via_context->t1_alarm);
    alarm_unset(via_context->t2_alarm);
    update_myviairq(via_context);

    via_context->oldpa = 0xff;
    via_context->oldpb = 0xff;

    via_context->ca2_state = 1;
    via_context->cb2_state = 1;
    (via_context->set_ca2)(via_context, via_context->ca2_state);      /* input = high */
    (via_context->set_cb2)(via_context, via_context->cb2_state);      /* input = high */

    if (via_context && via_context->reset) {
        (via_context->reset)(via_context);
    }

    via_context->enabled = 1;
}

void viacore_signal(via_context_t *via_context, int line, int edge)
{
    switch (line) {
        case VIA_SIG_CA1:
            if ((edge ? 1 : 0) == (via_context->via[VIA_PCR] & 0x01)) {
                if (IS_CA2_TOGGLE_MODE() && !(via_context->ca2_state)) {
                    via_context->ca2_state = 1;
                    (via_context->set_ca2)(via_context, via_context->ca2_state);
                }
                via_context->ifr |= VIA_IM_CA1;
                update_myviairq(via_context);
#ifdef MYVIA_NEED_LATCHING
                if (IS_PA_INPUT_LATCH()) {
                    via_context->ila = (via_context->read_pra)(via_context, addr);
                }
#endif
            }
            break;
        case VIA_SIG_CA2:
            if (!(via_context->via[VIA_PCR] & 0x08)) {
                via_context->ifr |= (((edge << 2)
                                    ^ via_context->via[VIA_PCR]) & 0x04) ?
                                    0 : VIA_IM_CA2;
                update_myviairq(via_context);
            }
            break;
        case VIA_SIG_CB1:
            if ((edge ? 0x10 : 0) == (via_context->via[VIA_PCR] & 0x10)) {
                if (IS_CB2_TOGGLE_MODE() && !(via_context->cb2_state)) {
                    via_context->cb2_state = 1;
                    (via_context->set_cb2)(via_context, via_context->cb2_state);
                }
                via_context->ifr |= VIA_IM_CB1;
                update_myviairq(via_context);
#ifdef MYVIA_NEED_LATCHING
                if (IS_PB_INPUT_LATCH()) {
                    via_context->ilb = (via_context->read_prb)(via_context);
                }
#endif
            }
            break;
        case VIA_SIG_CB2:
            if (!(via_context->via[VIA_PCR] & 0x80)) {
                via_context->ifr |= (((edge << 6)
                                    ^ via_context->via[VIA_PCR]) & 0x40) ?
                                    0 : VIA_IM_CB2;
                update_myviairq(via_context);
            }
            break;
    }
}

void viacore_store(via_context_t *via_context, WORD addr, BYTE byte)
{
    CLOCK rclk;

    if (*(via_context->rmw_flag)) {
        (*(via_context->clk_ptr))--;
        *(via_context->rmw_flag) = 0;
        viacore_store(via_context, addr, via_context->last_read);
        (*(via_context->clk_ptr))++;
    }

    /* stores have a one-cycle offset if CLK++ happens before store */
    rclk = *(via_context->clk_ptr) - via_context->write_offset;

    addr &= 0xf;

    switch (addr) {
        /* these are done with saving the value */
        case VIA_PRA:           /* port A */
            via_context->ifr &= ~VIA_IM_CA1;
            if (!IS_CA2_INDINPUT()) {
                via_context->ifr &= ~VIA_IM_CA2;
            }
            if (IS_CA2_HANDSHAKE()) {
                via_context->ca2_state = 0;
                (via_context->set_ca2)(via_context, via_context->ca2_state);
                if (IS_CA2_PULSE_MODE()) {
                    via_context->ca2_state = 1;
                    (via_context->set_ca2)(via_context, via_context->ca2_state);
                }
            }
            if (via_context->ier & (VIA_IM_CA1 | VIA_IM_CA2)) {
                update_myviairq(via_context);
            }

        case VIA_PRA_NHS: /* port A, no handshake */
            via_context->via[VIA_PRA_NHS] = byte;
            addr = VIA_PRA;
        case VIA_DDRA:
            via_context->via[addr] = byte;
            byte = via_context->via[VIA_PRA] | ~(via_context->via[VIA_DDRA]);
            (via_context->store_pra)(via_context, byte, via_context->oldpa, addr);
            via_context->oldpa = byte;
            break;

        case VIA_PRB:           /* port B */
            via_context->ifr &= ~VIA_IM_CB1;
            if ((via_context->via[VIA_PCR] & 0xa0) != 0x20) {
                via_context->ifr &= ~VIA_IM_CB2;
            }
            if (IS_CB2_HANDSHAKE()) {
                via_context->cb2_state = 0;
                (via_context->set_cb2)(via_context, via_context->cb2_state);
                if (IS_CB2_PULSE_MODE()) {
                    via_context->cb2_state = 1;
                    (via_context->set_cb2)(via_context, via_context->cb2_state);
                }
            }
            if (via_context->ier & (VIA_IM_CB1 | VIA_IM_CB2)) {
                update_myviairq(via_context);
            }

        case VIA_DDRB:
            via_context->via[addr] = byte;
            byte = via_context->via[VIA_PRB] | ~(via_context->via[VIA_DDRB]);
            (via_context->store_prb)(via_context, byte, via_context->oldpb, addr);
            via_context->oldpb = byte;
            break;

        case VIA_SR:            /* Serial Port output buffer */
            via_context->via[addr] = byte;

            /* shift state can only be reset once 8 bits are complete */
            if (via_context->ifr & VIA_IM_SR) {
                via_context->ifr &= ~VIA_IM_SR;
                update_myviairq(via_context);
                via_context->shift_state = 0;
            }
            (via_context->store_sr)(via_context, byte);
            break;

        /* Timers */

        case VIA_T1CL:
        case VIA_T1LL:
            via_context->via[VIA_T1LL] = byte;
            update_myviatal(via_context, rclk);
            break;

        case VIA_T1CH:  /* Write timer A high */
            via_context->via[VIA_T1LH] = byte;
            update_myviatal(via_context, rclk);
            /* load counter with latch value */
            via_context->tau = rclk + via_context->tal + 3 + TAUOFFSET;
            via_context->tai = rclk + via_context->tal + 2;
            alarm_set(via_context->t1_alarm, via_context->tai);

            /* set pb7 state */
            via_context->pb7 = 0;
            via_context->pb7o = 0;

            /* Clear T1 interrupt */
            via_context->ifr &= ~VIA_IM_T1;
            update_myviairq(via_context);
            break;

        case VIA_T1LH:          /* Write timer A high order latch */
            via_context->via[addr] = byte;
            update_myviatal(via_context, rclk);
            /* IF: Does not change T1 interrupt, see Synertek notes */
            break;

        case VIA_T2LL:          /* Write timer 2 low latch */
            via_context->via[VIA_T2LL] = byte;
            (via_context->store_t2l)(via_context, byte);
            break;

        case VIA_T2CH:            /* Write timer 2 high counter/latch */
            /* update counter and latch values */
            via_context->via[VIA_T2LH] = byte;
            via_context->t2cl = via_context->via[VIA_T2LL];
            via_context->t2ch = byte;

            /* start T2 only in timer mode, leave unchanged in pulse counting mode */
            if (!(via_context->via[VIA_ACR] & 0x20)) {
                /* set the next alarm to the low latch value as timer cascading mode change 
                matters at each underflow of the T2 low counter */
                via_context->tbu = rclk + via_context->t2cl + 3;
                via_context->tbi = rclk + via_context->t2cl + 2;
                alarm_set(via_context->t2_alarm, via_context->tbi);
            }

            /* Clear T2 interrupt */
            via_context->ifr &= ~VIA_IM_T2;
            update_myviairq(via_context);
            break;

        /* Interrupts */

        case VIA_IFR:           /* 6522 Interrupt Flag Register */
            via_context->ifr &= ~byte;
            update_myviairq(via_context);

            /* FIXME: clearing any timer interrupt should set the relevant timer alarm */
            break;

        case VIA_IER:           /* Interrupt Enable Register */
            if (byte & VIA_IM_IRQ) {
                /* set interrupts */
                via_context->ier |= byte & 0x7f;
            } else {
                /* clear interrupts */
                via_context->ier &= ~byte;
            }
            update_myviairq(via_context);
            break;

        /* Control */

        case VIA_ACR:
            /* bit 7 timer 1 output to PB7 */
            update_myviatal(via_context, rclk);
            if ((via_context->via[VIA_ACR] ^ byte) & 0x80) {
                if (byte & 0x80) {
                    via_context->pb7 = 1 ^ via_context->pb7x;
                }
            }
            if ((via_context->via[VIA_ACR] ^ byte) & 0x40) {
                via_context->pb7 ^= via_context->pb7sx;
                if ((byte & 0x40)) {
                    if (via_context->pb7x || via_context->pb7xx) {
                        if (via_context->tal) {
                            via_context->pb7o = 1;
                        } else {
                            via_context->pb7o = 0;
                            if ((via_context->via[VIA_ACR] & 0x80)
                                && via_context->pb7x
                                && (!(via_context->pb7xx))) {
                                via_context->pb7 ^= 1;
                            }
                        }
                    }
                }
            }
            via_context->pb7sx = via_context->pb7x;

            /* bit 1, 0  latch enable port B and A */
#ifdef MYVIA_NEED_LATCHING
            /* switch on port A latching - FIXME: is this ok? */
            if ((!(via_context->via[addr] & 1)) && (byte & 1)) {
                via_context->ila = (via_context->read_pra)(via_context, addr);
            }
            /* switch on port B latching - FIXME: is this ok? */
            if ((!(via_context->via[addr] & 2)) && (byte & 2)) {
                via_context->ilb = (via_context->read_prb)(via_context);
            }
#endif

            /* switch between timer and pulse counting mode if bit 5 changes */
            if ((via_context->via[VIA_ACR] ^ byte) & 0x20) {
                if (byte & 0x20) {
                    /* Pulse counting mode: set t2 to the current T2 value; 
                    PB6 should always update t2 and update irq on underflow */
                    CLOCK stop = myviatb(via_context);
                    via_context->t2cl = (BYTE)(stop & 0xff);
                    via_context->t2ch = (BYTE)((stop >> 8) & 0xff);

                    /* stop alarm to prevent t2 and T2 updates */
                    alarm_unset(via_context->t2_alarm);
                    via_context->tbi = 0;
                } else {
                    /* Timer mode; set the next alarm to the low latch value as timer cascading mode change 
                    matters at each underflow of the T2 low counter */
                    via_context->tbu = rclk + via_context->t2cl + 3;
                    via_context->tbi = rclk + via_context->t2cl + 2;
                    alarm_set(via_context->t2_alarm, via_context->tbi);
                }
            }

            /* bit 4, 3, 2 shift register control */

            via_context->via[addr] = byte;
            (via_context->store_acr)(via_context, byte);

            break;

        case VIA_PCR:

            /* bit 7, 6, 5  CB2 handshake/interrupt control */
            /* bit 4  CB1 interrupt control */

            /* bit 3, 2, 1  CA2 handshake/interrupt control */
            /* bit 0  CA1 interrupt control */

            if ((byte & 0x0e) == 0x0c) { /* set output low */
                via_context->ca2_state = 0;
            } else
            if ((byte & 0x0e) == 0x0e) { /* set output high */
                via_context->ca2_state = 1;
            } else {                    /* set to toggle/pulse/input */
                /* FIXME: is this correct if handshake is already active? */
                via_context->ca2_state = 1;
            }
            (via_context->set_ca2)(via_context, via_context->ca2_state);

            if ((byte & 0xe0) == 0xc0) { /* set output low */
                via_context->cb2_state = 0;
            } else
            if ((byte & 0xe0) == 0xe0) { /* set output high */
                via_context->cb2_state = 1;
            } else {                    /* set to toggle/pulse/input */
                /* FIXME: is this correct if handshake is already active? */
                via_context->cb2_state = 1;
            }
            (via_context->set_cb2)(via_context, via_context->cb2_state);

            (via_context->store_pcr)(via_context, byte, addr);

            via_context->via[addr] = byte;

            break;

        default:
            via_context->via[addr] = byte;
    }                           /* switch */
}


/* ------------------------------------------------------------------------- */

BYTE viacore_read(via_context_t *via_context, WORD addr)
{
#ifdef MYVIA_TIMER_DEBUG
    BYTE viacore_read_(via_context_t *via_context, WORD);
    BYTE retv = myvia_read_(via_context, addr);
    addr &= 0x0f;
    if ((addr > 3 && addr < 10) || app_resources.debugFlag) {
        log_message(via_context->log,
                    "myvia_read(%x) -> %02x, clk=%d", addr, retv,
                    *(via_context->clk_ptr));
    }
    return retv;
}
BYTE viacore_read_(via_context_t *via_context, WORD addr)
{
#endif
    BYTE byte = 0xff;
    CLOCK rclk;

    addr &= 0xf;

    via_context->read_clk = *(via_context->clk_ptr);
    via_context->read_offset = 0;
    rclk = *(via_context->clk_ptr);

    if (addr >= VIA_T1CL && addr <= VIA_IER) {
        if (via_context->tai && (via_context->tai < *(via_context->clk_ptr))) {
            viacore_intt1(*(via_context->clk_ptr) - via_context->tai,
                          (void *)via_context);
        }
        if (via_context->tbi && (via_context->tbi < *(via_context->clk_ptr))) {
            viacore_intt2(*(via_context->clk_ptr) - via_context->tbi,
                          (void *)via_context);
        }
    }

    switch (addr) {
        case VIA_PRA:           /* port A */
            via_context->ifr &= ~VIA_IM_CA1;
            if ((via_context->via[VIA_PCR] & 0x0a) != 0x02) {
                via_context->ifr &= ~VIA_IM_CA2;
            }
            if (IS_CA2_HANDSHAKE()) {
                via_context->ca2_state = 0;
                (via_context->set_ca2)(via_context, via_context->ca2_state);
                if (IS_CA2_PULSE_MODE()) {
                    via_context->ca2_state = 1;
                    (via_context->set_ca2)(via_context, via_context->ca2_state);
                }
            }
            if (via_context->ier & (VIA_IM_CA1 | VIA_IM_CA2)) {
                update_myviairq(via_context);
            }

        case VIA_PRA_NHS: /* port A, no handshake */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value as the other port. Value read might be different
               from what is expected due to excessive load. */
#ifdef MYVIA_NEED_LATCHING
            if (IS_PA_INPUT_LATCH()) {
                byte = via_context->ila;
            } else {
                byte = (via_context->read_pra)(via_context, addr);
            }
#else
            byte = (via_context->read_pra)(via_context, addr);
#endif
            via_context->ila = byte;
            via_context->last_read = byte;
            return byte;

        case VIA_PRB:           /* port B */
            via_context->ifr &= ~VIA_IM_CB1;
            if ((via_context->via[VIA_PCR] & 0xa0) != 0x20) {
                via_context->ifr &= ~VIA_IM_CB2;
            }
            if (via_context->ier & (VIA_IM_CB1 | VIA_IM_CB2)) {
                update_myviairq(via_context);
            }

            /* WARNING: this pin reads the ORA for output pins, not
               the voltage on the pins as the other port. */
#ifdef MYVIA_NEED_LATCHING
            if (IS_PB_INPUT_LATCH()) {
                byte = via_context->ilb;
            } else {
                byte = (via_context->read_prb)(via_context);
            }
#else
            byte = (via_context->read_prb)(via_context);
#endif
            via_context->ilb = byte;
            byte = (byte & ~(via_context->via[VIA_DDRB]))
                   | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]);

            if (via_context->via[VIA_ACR] & 0x80) {
                update_myviatal(via_context, rclk);
                byte = (byte & 0x7f)
                       | (((via_context->pb7 ^ via_context->pb7x)
                           | via_context->pb7o) ? 0x80 : 0);
            }
            via_context->last_read = byte;
            return byte;

        /* Timers */

        case VIA_T1CL /*TIMER_AL */:    /* timer A low */
            via_context->ifr &= ~VIA_IM_T1;
            update_myviairq(via_context);
            via_context->last_read = (BYTE)(myviata(via_context) & 0xff);
            return via_context->last_read;

        case VIA_T1CH /*TIMER_AH */:    /* timer A high */
            via_context->last_read = (BYTE)((myviata(via_context) >> 8) & 0xff);
            return via_context->last_read;

        case VIA_T2CL /*TIMER_BL */:    /* timer B low */
            via_context->ifr &= ~VIA_IM_T2;
            update_myviairq(via_context);
            via_context->last_read = (BYTE)(myviatb(via_context) & 0xff);
            return via_context->last_read;

        case VIA_T2CH /*TIMER_BH */:    /* timer B high */
            via_context->last_read = (BYTE)((myviatb(via_context) >> 8) & 0xff);
            return via_context->last_read;

        case VIA_SR:            /* Serial Port Shift Register */
            /* shift state can only be reset once 8 bits are complete */
            if (via_context->ifr & VIA_IM_SR) {
                via_context->ifr &= ~VIA_IM_SR;
                update_myviairq(via_context);
                via_context->shift_state = 0;
            }
            via_context->last_read = via_context->via[addr];
            return via_context->last_read;

        /* Interrupts */

        case VIA_IFR:           /* Interrupt Flag Register */
            {
                BYTE t = via_context->ifr;
                if (via_context->ifr & via_context->ier /*[VIA_IER] */) {
                    t |= 0x80;
                }
                via_context->last_read = t;
                return (t);
            }

        case VIA_IER:           /* 6522 Interrupt Control Register */
            via_context->last_read = (via_context->ier /*[VIA_IER] */ | 0x80);
            return via_context->last_read;
    }                           /* switch */

    via_context->last_read = via_context->via[addr];

    return via_context->via[addr];
}

BYTE viacore_peek(via_context_t *via_context, WORD addr)
{
    CLOCK rclk = *(via_context->clk_ptr);

    addr &= 0xf;

    if (via_context->tai && (via_context->tai <= *(via_context->clk_ptr))) {
        viacore_intt1(*(via_context->clk_ptr) - via_context->tai,
                      (void *)via_context);
    }
    if (via_context->tbi && (via_context->tbi <= *(via_context->clk_ptr))) {
        viacore_intt2(*(via_context->clk_ptr) - via_context->tbi,
                      (void *)via_context);
    }

    switch (addr) {
        case VIA_PRA:
            return viacore_read(via_context, VIA_PRA_NHS);

        case VIA_PRB:           /* port B */
            {
                BYTE byte;
#ifdef MYVIA_NEED_LATCHING
                if (IS_PB_INPUT_LATCH()) {
                    byte = via_context->ilb;
                } else {
                    byte = (via_context->read_prb)(via_context);
                }
#else
                byte = (via_context->read_prb)(via_context);
#endif
                byte = (byte & ~(via_context->via[VIA_DDRB]))
                       | (via_context->via[VIA_PRB] & via_context->via[VIA_DDRB]);
                if (via_context->via[VIA_ACR] & 0x80) {
                    update_myviatal(via_context, rclk);
                    byte = (byte & 0x7f) | (((via_context->pb7 ^ via_context->pb7x)
                                             | via_context->pb7o) ? 0x80 : 0);
                }
                return byte;
            }

        /* Timers */

        case VIA_T1CL /*TIMER_AL */:    /* timer A low */
            return (BYTE)(myviata(via_context) & 0xff);

        case VIA_T2CL /*TIMER_BL */:    /* timer B low */
            return (BYTE)(myviatb(via_context) & 0xff);

        default:
            break;
    }                           /* switch */

    return viacore_read(via_context, addr);
}


/* ------------------------------------------------------------------------- */

static void viacore_intt1(CLOCK offset, void *data)
{
    CLOCK rclk;
    via_context_t *via_context = (via_context_t *)data;

    rclk = *(via_context->clk_ptr) - offset;


#ifdef MYVIA_TIMER_DEBUG
    if (app_resources.debugFlag) {
        log_message(via_context->log, "myvia timer A interrupt");
    }
#endif

    if (!(via_context->via[VIA_ACR] & 0x40)) {     /* one-shot mode */
#ifdef MYVIA_TIMER_DEBUG
        log_message(via_context->log,
                    "MYVIA Timer A interrupt -- one-shot mode: next int won't happen");
#endif
        alarm_unset(via_context->t1_alarm);
        via_context->tai = 0;
    } else {                    /* continuous mode */
        /* load counter with latch value */
        via_context->tai += via_context->tal + 2;
        alarm_set(via_context->t1_alarm, via_context->tai);

        /* Let tau also keep up with the cpu clock
           this should avoid "% (via_context->tal + 2)" case */
        via_context->tau += via_context->tal + 2;
    }
    via_context->ifr |= VIA_IM_T1;
    update_myviairq_rclk(via_context, rclk);

    /* TODO: toggle PB7? */
    /*(viaier & VIA_IM_T1) ? 1:0; */
}

/* T2 can be switched between 8 and 16 bit modes ad-hoc, any time, by setting the shifter to be 
   controlled by T2 via selecting the relevant ACR shift register operating mode. 
   This change affects how the next T2 low underflow is handled */
static void viacore_intt2(CLOCK offset, void *data)
{
    CLOCK rclk;
    int next_alarm;
    via_context_t *via_context = (via_context_t *)data;

    rclk = *(via_context->clk_ptr) - offset;

#ifdef MYVIA_TIMER_DEBUG
    if (app_resources.debugFlag) {
        log_message(via_context->log, "MYVIA timer B interrupt.");
    }
#endif

    /* If the shifter is under T2 control, the T2 timer works differently, and have a period of T2 low.
       T2 high is still cascaded though and decreases at each T2 low underflow */
    if ((via_context->via[VIA_ACR] & 0x0c) == 0x04) {
        /* 8 bit timer mode; reload T2 low from latch */
        via_context->t2cl = via_context->via[VIA_T2LL];

        /* set next alarm to T2 low period */
        next_alarm = via_context->via[VIA_T2LL] + 2;

        /* T2 acts as a pulse generator for CB1
           every second underflow is a pulse updating the shift register, until all 8 bits are complete */
        if (via_context->shift_state < 16) {
            /* FIXME: CB1 should be toggled, and interrupt flag set according to edge detection in PCR */
            if (via_context->shift_state & 1) {
                if (via_context->via[VIA_ACR] & 0x10) {
                    /* FIXME: shift out */
                } else {
                    /* shift in */
                    /* FIXME: we should read CB2 here instead of 1, but CB2 state must not be controlled by PCR
                       until the signalling function is correct with shifter active, just use 1 instead */
                    via_context->via[VIA_SR] = (via_context->via[VIA_SR] << 1 ) | 1;
                }
            }

            /* next shifter bit; set SR interrupt if 8 bits are complete */
            if (++via_context->shift_state == 16) {
                via_context->ifr |= VIA_IM_SR;
                update_myviairq_rclk(via_context, rclk);
            }
        }
    } else {
        /* 16 bit timer mode; it is guaranteed that T2 low is in underflow */
        via_context->t2cl = 0xff;

        /* set next alarm to 256 cycles later, until t2 high underflow */
        next_alarm = (via_context->t2ch) ? 256 : 0;
    }

    /* T2 low count underflow always decreases T2 high count */
    via_context->t2ch--;

    /* set the next T2 low underflow alarm, or turn off the alarm */
    if (next_alarm) {
        via_context->tbu += next_alarm;
        via_context->tbi += next_alarm;
        alarm_set(via_context->t2_alarm, via_context->tbi);
    } else {
        alarm_unset(via_context->t2_alarm);
        via_context->tbi = 0;
    }

    /* 16 bit timer underflow generates an interrupt */
    /* FIXME: does 16 bit underflow generate an IRQ in 8 bit mode? 8 bit underflow does not */
    if (via_context->t2ch == 0xff) {
        via_context->ifr |= VIA_IM_T2;
        update_myviairq_rclk(via_context, rclk);
    }
}

static void viacore_clk_overflow_callback(CLOCK sub, void *data)
{
    via_context_t *via_context;

    via_context = (via_context_t *)data;

    if (via_context->enabled == 0) {
        return;
    }

#if 0
    via_context->tau = via_context->tal + 2 -
                       ((*(via_context->clk_ptr) + sub - via_context->tau)
                        % (via_context->tal + 2));

    via_context->tbu = via_context->tbl + 2 -
                       ((*(via_context->clk_ptr) + sub - via_context->tbu)
                        % (via_context->tbl + 2));
#else
    via_context->tau -= sub;
    via_context->tbu -= sub;
#endif

    if (via_context->tai) {
        via_context->tai -= sub;
    }

    if (via_context->tbi) {
        via_context->tbi -= sub;
    }

    if (via_context->read_clk > sub) {
        via_context->read_clk -= sub;
    } else {
        via_context->read_clk = 0;
    }
}

void viacore_setup_context(via_context_t *via_context)
{
    via_context->read_clk = 0;
    via_context->read_offset = 0;
    via_context->last_read = 0;
    via_context->log = LOG_ERR;

    via_context->my_module_name_alt1 = NULL;
    via_context->my_module_name_alt2 = NULL;

    via_context->write_offset = 1;
}

void viacore_init(via_context_t *via_context, alarm_context_t *alarm_context,
                  interrupt_cpu_status_t *int_status, clk_guard_t *clk_guard)
{
    char *buffer;

    if (via_context->log == LOG_ERR) {
        via_context->log = log_open(via_context->my_module_name);
    }

    buffer = lib_msprintf("%sT1", via_context->myname);
    via_context->t1_alarm = alarm_new(alarm_context, buffer, viacore_intt1,
                                      via_context);
    lib_free(buffer);

    buffer = lib_msprintf("%sT2", via_context->myname);
    via_context->t2_alarm = alarm_new(alarm_context, buffer, viacore_intt2,
                                      via_context);
    lib_free(buffer);

    via_context->int_num = interrupt_cpu_status_int_new(int_status,
                                                        via_context->myname);
    clk_guard_add_callback(clk_guard, viacore_clk_overflow_callback,
                           via_context);
}

void viacore_shutdown(via_context_t *via_context)
{
    lib_free(via_context->prv);
    lib_free(via_context->myname);
    lib_free(via_context->my_module_name);
    lib_free(via_context->my_module_name_alt1);
    lib_free(via_context->my_module_name_alt2);
    lib_free(via_context);
}

void viacore_set_sr(via_context_t *via_context, BYTE data)
{
    if (!(via_context->via[VIA_ACR] & 0x10) && (via_context->via[VIA_ACR] & 0x0c)) {
        via_context->via[VIA_SR] = data;
        via_context->ifr |= VIA_IM_SR;
        update_myviairq(via_context);
    }
}

/*------------------------------------------------------------------------*/

/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 0/0
 */

/* The name of the modul must be defined before including this file.  */
#define VIA_DUMP_VER_MAJOR      1
#define VIA_DUMP_VER_MINOR      1

/*
 * The dump data:
 *
 * UBYTE        ORA
 * UBYTE        DDRA
 * UBYTE        ORB
 * UBYTE        DDRB
 * UWORD        T1L
 * UWORD        T1C
 * UBYTE        T2LL
 * UBYTE        T2LH
 * UBYTE        T2CL
 * UBYTE        T2CH
 * UWORD        T2C
 * UBYTE        SR
 * UBYTE        ACR
 * UBYTE        PCR
 * UBYTE        IFR              active interrupts
 * UBYTE        IER              interrupt masks
 * UBYTE        PB7              bit 7 = pb7 state
 * UBYTE        SRHBITS          shift register state helper
 * UBYTE        CABSTATE         bit 7 = ca2 state, bi 6 = cb2 state
 * UBYTE        ILA              input latch port A
 * UBYTE        ILB              input latch port B
 */

/* FIXME!!!  Error check.  */

int viacore_snapshot_write_module(via_context_t *via_context, snapshot_t *s)
{
    snapshot_module_t *m;

    if (via_context->tai && (via_context->tai <= *(via_context->clk_ptr))) {
        viacore_intt1(*(via_context->clk_ptr) - via_context->tai,
                      (void *)via_context);
    }
    if (via_context->tbi && (via_context->tbi <= *(via_context->clk_ptr))) {
        viacore_intt2(*(via_context->clk_ptr) - via_context->tbi,
                      (void *)via_context);
    }

    m = snapshot_module_create(s, via_context->my_module_name,
                               VIA_DUMP_VER_MAJOR, VIA_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    SMW_B(m, via_context->via[VIA_PRA]);
    SMW_B(m, via_context->via[VIA_DDRA]);
    SMW_B(m, via_context->via[VIA_PRB]);
    SMW_B(m, via_context->via[VIA_DDRB]);

    SMW_W(m, (WORD)(via_context->tal));
    SMW_W(m, (WORD)myviata(via_context));
    SMW_B(m, via_context->via[VIA_T2LL]);
    SMW_B(m, via_context->via[VIA_T2LH]);
    SMW_B(m, via_context->t2cl);
    SMW_B(m, via_context->t2ch);
    SMW_W(m, (WORD)myviatb(via_context));

    SMW_B(m, (BYTE)((via_context->tai ? 0x80 : 0) | (via_context->tbi ? 0x40 : 0)));

    SMW_B(m, via_context->via[VIA_SR]);
    SMW_B(m, via_context->via[VIA_ACR]);
    SMW_B(m, via_context->via[VIA_PCR]);

    SMW_B(m, (BYTE)(via_context->ifr));
    SMW_B(m, (BYTE)(via_context->ier));
    /* FIXME! */
    SMW_B(m, (BYTE)((((via_context->pb7 ^ via_context->pb7x)
                      | via_context->pb7o) ? 0x80 : 0)));

    SMW_B(m, (BYTE)via_context->shift_state);           /* SRHBITS */

    SMW_B(m, (BYTE)((via_context->ca2_state ? 0x80 : 0)
                    | (via_context->cb2_state ? 0x40 : 0)));

    SMW_B(m, via_context->ila);
    SMW_B(m, via_context->ilb);

    snapshot_module_close(m);

    return 0;
}

int viacore_snapshot_read_module(via_context_t *via_context, snapshot_t *s)
{
    BYTE vmajor, vminor;
    BYTE byte;
    WORD word;
    WORD addr;
    CLOCK rclk = *(via_context->clk_ptr);
    snapshot_module_t *m;

    m = snapshot_module_open(s, via_context->my_module_name, &vmajor, &vminor);

    if (m == NULL) {
        if (via_context->my_module_name_alt1 == NULL) {
            return -1;
        }

        m = snapshot_module_open(s, via_context->my_module_name_alt1,
                                 &vmajor, &vminor);
        if (m == NULL) {
            if (via_context->my_module_name_alt2 == NULL) {
                return -1;
            }

            m = snapshot_module_open(s, via_context->my_module_name_alt2,
                                     &vmajor, &vminor);
            if (m == NULL) {
                return -1;
            }
        }
    }

    if (vmajor != VIA_DUMP_VER_MAJOR) {
        log_error(via_context->log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  vmajor, vminor, VIA_DUMP_VER_MAJOR, VIA_DUMP_VER_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    alarm_unset(via_context->t1_alarm);
    alarm_unset(via_context->t2_alarm);

    via_context->tai = 0;
    via_context->tbi = 0;

    SMR_B(m, &(via_context->via[VIA_PRA]));
    SMR_B(m, &(via_context->via[VIA_DDRA]));
    SMR_B(m, &(via_context->via[VIA_PRB]));
    SMR_B(m, &(via_context->via[VIA_DDRB]));
    {
        addr = VIA_DDRA;
        byte = via_context->via[VIA_PRA] | ~(via_context->via[VIA_DDRA]);
        (via_context->undump_pra)(via_context, byte);
        via_context->oldpa = byte;

        addr = VIA_DDRB;
        byte = via_context->via[VIA_PRB] | ~(via_context->via[VIA_DDRB]);
        (via_context->undump_prb)(via_context, byte);
        via_context->oldpb = byte;
    }

    SMR_W(m, &word);
    via_context->tal = word;
    via_context->via[VIA_T1LL] = via_context->tal & 0xff;
    via_context->via[VIA_T1LH] = (via_context->tal >> 8) & 0xff;
    SMR_W(m, &word);
    via_context->tau = rclk + word + 2 /* 3 */ + TAUOFFSET;
    via_context->tai = rclk + word + 1;

    SMR_B(m, &(via_context->via[VIA_T2LL]));
    SMR_B(m, &(via_context->via[VIA_T2LH]));
    SMR_B(m, &(via_context->t2cl));
    SMR_B(m, &(via_context->t2ch));
    SMR_W(m, &word);
    via_context->tbu = rclk + word + 2 /* 3 */;
    via_context->tbi = rclk + word + 1;

    SMR_B(m, &byte);
    if (byte & 0x80) {
        alarm_set(via_context->t1_alarm, via_context->tai);
    } else {
        via_context->tai = 0;
    }
    if (byte & 0x40) {
        alarm_set(via_context->t2_alarm, via_context->tbi);
    } else {
        via_context->tbi = 0;
    }

    SMR_B(m, &(via_context->via[VIA_SR]));
    SMR_B(m, &(via_context->via[VIA_ACR]));
    SMR_B(m, &(via_context->via[VIA_PCR]));

    SMR_B(m, &byte);
    via_context->ifr = byte;
    SMR_B(m, &byte);
    via_context->ier = byte;

    via_restore_int(via_context, via_context->ifr & via_context->ier & 0x7f);

    /* FIXME! */
    SMR_B(m, &byte);
    via_context->pb7 = byte ? 1 : 0;
    via_context->pb7x = 0;
    via_context->pb7o = 0;
    SMR_B(m, &byte);        /* SRHBITS */
    via_context->shift_state = byte;

    SMR_B(m, &byte);        /* CABSTATE */
    via_context->ca2_state = byte & 0x80;
    via_context->cb2_state = byte & 0x40;

    /* undump_pcr also restores the ca2_state/cb2_state effects if necessary;
       i.e. calls set_c*2(c*2_state) if necessary */
    {
        addr = VIA_PCR;
        byte = via_context->via[addr];
        (via_context->undump_pcr)(via_context, byte);
    }
    {
        addr = VIA_SR;
        byte = via_context->via[addr];
        (via_context->store_sr)(via_context, byte);
    }
    {
        addr = VIA_ACR;
        byte = via_context->via[addr];
        (via_context->undump_acr)(via_context, byte);
    }

    SMR_B(m, &(via_context->ila));
    SMR_B(m, &(via_context->ilb));

    return snapshot_module_close(m);
}

int viacore_dump(via_context_t *via_context)
{
    /* FIXME: dump details using mon_out(). return 0 on success */
    return -1;
}
