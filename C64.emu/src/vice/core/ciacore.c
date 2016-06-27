/*
 * ciacore.c - Template file for MOS6526 (CIA) emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * Patches and improvements by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Alexander Bluhm <mam96ehy@studserv.uni-leipzig.de>
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

/* #define DEBUG_CIA */

#ifdef DEBUG_CIA
#define DBG(_x_)        log_debug _x_
#else
#define DBG(_x_)
#endif

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "cia.h"
#include "clkguard.h"
#include "ciatimer.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"


#define STORE_OFFSET 1
#define READ_OFFSET 0

#define CIAT_STOPPED    0
#define CIAT_RUNNING    1
#define CIAT_COUNTTA    2

static void ciacore_intta(CLOCK offset, void *data);
static void ciacore_inttb(CLOCK offset, void *data);


/* The following is an attempt in rewriting the interrupt defines into
   static inline functions. This should not hurt, but I still kept the
   define below, to be able to compare speeds.
   The semantics of the call has changed, the interrupt number is
   not needed anymore (because it's known to my_set_int(). Actually
   one could also remove MYCIA_INT as it is also known... */

/* new semantics and as inline function, value can be replaced by 0/1 */
static inline void my_set_int(cia_context_t *cia_context, int value,
                              CLOCK rclk)
{
#ifdef CIA_TIMER_DEBUG
    if (cia_context->debugFlag) {
        log_message(cia_context->log, "set_int(rclk=%d, d=%d pc=).",
                    rclk, (value));
    }
#endif
    if ((value)) {
        /* cia_context->irqflags |= 0x80; */
        (cia_context->cia_set_int_clk)(cia_context, cia_context->irq_line,
                                       (rclk));
        cia_context->irq_enabled = 1;
    } else {
        (cia_context->cia_set_int_clk)(cia_context, 0, (rclk));
        cia_context->irq_enabled = 0;
    }
}


/* ------------------------------------------------------------------------- */
/* cia */


inline static void check_ciatodalarm(cia_context_t *cia_context, CLOCK rclk)
{
    if (!memcmp(cia_context->todalarm, cia_context->c_cia + CIA_TOD_TEN,
                   sizeof(cia_context->todalarm))) {
        cia_context->irqflags |= CIA_IM_TOD;
        if (cia_context->c_cia[CIA_ICR] & CIA_IM_TOD) {
            cia_context->irqflags |= 0x80;
            my_set_int(cia_context, cia_context->irq_line,
                       *(cia_context->clk_ptr));
        }
    }
}

/* ------------------------------------------------------------------------- */
/*
 * ciat_update return the number of underflows
 * FIXME: SDR count, etc
 */

static void cia_do_update_ta(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_update(cia_context->ta, rclk))) {
        cia_context->irqflags |= CIA_IM_TA;
        cia_context->tat = (cia_context->tat + n) & 1;
    }
}

static void cia_do_update_tb(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_update(cia_context->tb, rclk))) {
        cia_context->irqflags |= CIA_IM_TB;
        if (cia_context->model == CIA_MODEL_6526
            && cia_context->rdi == rclk - 1) {
            /* flag the timer B bug */
            cia_context->irqflags |= CIA_IM_TBB;
        } else {
            cia_context->irqflags &= ~CIA_IM_TBB;
        }
        cia_context->tbt = (cia_context->tbt + n) & 1;
    }
}

static void cia_do_step_tb(cia_context_t *cia_context, CLOCK rclk)
{
    int n;

    if ((n = ciat_single_step(cia_context->tb, rclk))) {
        cia_context->irqflags |= CIA_IM_TB;
        cia_context->tbt = (cia_context->tbt + n) & 1;
    }
}

/*
 * Those functions are called everywhere but in the alarm functions.
 */


static void cia_update_ta(cia_context_t *cia_context, CLOCK rclk)
{
    CLOCK tmp, last_tmp;

    last_tmp = 0;
    tmp = ciat_alarm_clk(cia_context->ta);
    while (tmp <= rclk) {
        ciacore_intta(*(cia_context->clk_ptr) - tmp, (void *)cia_context);
        last_tmp = tmp;
        tmp = ciat_alarm_clk(cia_context->ta);
    }

    if (last_tmp != rclk) {
        cia_do_update_ta(cia_context, rclk);
    }
}

static void cia_update_tb(cia_context_t *cia_context, CLOCK rclk)
{
    CLOCK tmp, last_tmp;

    if ((cia_context->c_cia[CIA_CRB] & 0x41) == 0x41) {
        cia_update_ta(cia_context, rclk);
    }

    last_tmp = 0;
    tmp = ciat_alarm_clk(cia_context->tb);
    while (tmp <= rclk) {
        ciacore_inttb(*(cia_context->clk_ptr) - tmp, (void *)cia_context);
        last_tmp = tmp;
        tmp = ciat_alarm_clk(cia_context->tb);
    }

    if (last_tmp != rclk) {
        cia_do_update_tb(cia_context, rclk);
    }
}

/*
 * set interrupt line
 */
static void cia_do_set_int(cia_context_t *cia_context, CLOCK rclk)
{
#if 0
    if ((cia_context->model == CIA_MODEL_6526)
        && (cia_context->rdi == rclk - 1)
        && (cia_context->irq_line != IK_NMI)) {
        /* FIXME explanation */
        return;
    }

#endif

    if ((cia_context->rdi == rclk - 1)
        && (cia_context->model == CIA_MODEL_6526A)) {
        /* Interrupt delayed by 1/2 cycle if acknowledged on assert */
        rclk++;
    }

    if (!(cia_context->irqflags & cia_context->c_cia[CIA_ICR] & 0x7f)) {
        /* no interrupts */
        return;
    }

    if ((cia_context->model != CIA_MODEL_6526A) && (cia_context->rdi == rclk)) {
        /* FIXME explanation */
        return;
    }

    if (cia_context->model != CIA_MODEL_6526A) {
        /* interrupts are delayed by 1 clk on old CIAs */
        rclk++;
    }

    if (cia_context->irqflags & CIA_IM_TBB) {
        /* timer b bug */
        cia_context->irqflags &= ~(CIA_IM_TBB | CIA_IM_TB);
    }

    my_set_int(cia_context, cia_context->irq_line, rclk);
    cia_context->irqflags |= 0x80;
}

/* ------------------------------------------------------------------------- */

static void ciacore_clk_overflow_callback(CLOCK sub, void *data)
{
    cia_context_t *cia_context;

    cia_context = (cia_context_t *)data;

    if (cia_context->enabled == 0) {
        return;
    }

    /* we assume that sub has already been substracted from myclk */
    cia_update_ta(cia_context, *(cia_context->clk_ptr) + sub);
    cia_update_tb(cia_context, *(cia_context->clk_ptr) + sub);

    ciat_prevent_clock_overflow(cia_context->ta, sub);
    ciat_prevent_clock_overflow(cia_context->tb, sub);

    if (cia_context->rdi > sub) {
        cia_context->rdi -= sub;
    } else {
        cia_context->rdi = 0;
    }

    if (cia_context->read_clk > sub) {
        cia_context->read_clk -= sub;
    } else {
        cia_context->read_clk = 0;
    }

    if (cia_context->todclk) {
        cia_context->todclk -= sub;
    }
}

/* -------------------------------------------------------------------------- */
void ciacore_disable(cia_context_t *cia_context)
{
#ifdef USE_IDLE_CALLBACK
    alarm_unset(cia_context->idle_alarm);
#endif
    alarm_unset(cia_context->ta_alarm);
    alarm_unset(cia_context->tb_alarm);
    alarm_unset(cia_context->tod_alarm);
    cia_context->enabled = 0;
}


void ciacore_reset(cia_context_t *cia_context)
{
    int i;

    for (i = 0; i < 16; i++) {
        cia_context->c_cia[i] = 0;
    }

    cia_context->rdi = 0;
    cia_context->sr_bits = 0;
    cia_context->read_clk = 0;

    ciat_reset(cia_context->ta, *(cia_context->clk_ptr));
    ciat_reset(cia_context->tb, *(cia_context->clk_ptr));

    cia_context->sdr_valid = 0;

    memset(cia_context->todalarm, 0, sizeof(cia_context->todalarm));
    cia_context->todlatched = 0;
    cia_context->todstopped = 1;
    cia_context->c_cia[CIA_TOD_HR] = 1;          /* the most common value */
    memcpy(cia_context->todlatch, cia_context->c_cia + CIA_TOD_TEN, sizeof(cia_context->todlatch));
    cia_context->todclk = *(cia_context->clk_ptr) + cia_context->todticks;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);
    cia_context->todtickcounter = 0;

    cia_context->irqflags = 0;
    cia_context->irq_enabled = 0;

    my_set_int(cia_context, 0, *(cia_context->clk_ptr));

    cia_context->old_pa = 0xff;
    cia_context->old_pb = 0xff;

    (cia_context->do_reset_cia)(cia_context);
    cia_context->enabled = 1;
}


static void ciacore_store_internal(cia_context_t *cia_context, WORD addr, BYTE byte)
{
    CLOCK rclk;

    addr &= 0xf;

    /* stores have a one-cycle offset if CLK++ happens before store */
    rclk = *(cia_context->clk_ptr) - cia_context->write_offset;

#ifdef CIA_TIMER_DEBUG
    if (cia_context->debugFlag) {
        log_message(cia_context->log, "store cia[%02x] %02x @ clk=%d",
                    (int) addr, (int) byte, rclk);
    }
#endif

    switch (addr) {
        case CIA_PRA:           /* port A */
        case CIA_DDRA:
            cia_context->c_cia[addr] = byte;
            byte = cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]);
            if (byte != cia_context->old_pa) {
                (cia_context->store_ciapa)(cia_context, *(cia_context->clk_ptr),
                                           byte);
                cia_context->old_pa = byte;
            }
            break;

        case CIA_PRB:           /* port B */
        case CIA_DDRB:
            cia_context->c_cia[addr] = byte;
            byte = cia_context->c_cia[CIA_PRB] | ~(cia_context->c_cia[CIA_DDRB]);
            if ((cia_context->c_cia[CIA_CRA]
                 | cia_context->c_cia[CIA_CRB]) & 0x02) {
                if (cia_context->c_cia[CIA_CRA] & 0x02) {
                    cia_update_ta(cia_context, rclk);
                    byte &= 0xbf;
                    if (((cia_context->c_cia[CIA_CRA] & 0x04) ? cia_context->tat
                         : ciat_is_underflow_clk(cia_context->ta, rclk))) {
                        byte |= 0x40;
                    }
                }
                if (cia_context->c_cia[CIA_CRB] & 0x02) {
                    cia_update_tb(cia_context, rclk);
                    byte &= 0x7f;
                    if (((cia_context->c_cia[CIA_CRB] & 0x04) ? cia_context->tbt
                         : ciat_is_underflow_clk(cia_context->tb, rclk))) {
                        byte |= 0x80;
                    }
                }
            }
            if (byte != cia_context->old_pb) {
                (cia_context->store_ciapb)(cia_context, *(cia_context->clk_ptr),
                                           byte);
                cia_context->old_pb = byte;
            }
            if (addr == CIA_PRB) {
                (cia_context->pulse_ciapc)(cia_context, rclk);
            }
            break;

        case CIA_TAL:
            cia_update_ta(cia_context, rclk);
            ciat_set_latchlo(cia_context->ta, rclk, (BYTE)byte);
            break;
        case CIA_TBL:
            cia_update_tb(cia_context, rclk);
            ciat_set_latchlo(cia_context->tb, rclk, (BYTE)byte);
            break;
        case CIA_TAH:
            cia_update_ta(cia_context, rclk);
            ciat_set_latchhi(cia_context->ta, rclk, (BYTE)byte);
            break;
        case CIA_TBH:
            cia_update_tb(cia_context, rclk);
            ciat_set_latchhi(cia_context->tb, rclk, (BYTE)byte);
            break;

        /*
         * TOD clock is stopped by writing Hours, and restarted
         * upon writing Tenths of Seconds.
         *
         * REAL:  TOD register + (wallclock - ciatodrel)
         * VIRT:  TOD register + (cycles - begin)/cycles_per_sec
         */
        case CIA_TOD_TEN: /* Time Of Day clock 1/10 s */
        case CIA_TOD_HR:  /* Time Of Day clock hour */
        case CIA_TOD_SEC: /* Time Of Day clock sec */
        case CIA_TOD_MIN: /* Time Of Day clock min */
            if (addr == CIA_TOD_HR) {
                /* force bits 6-5 = 0 */
                byte &= 0x9f;
                /* Flip AM/PM on hour 12  */
                /* Flip AM/PM only when writing time, not when writing alarm */
                if ((byte & 0x1f) == 0x12 && !(cia_context->c_cia[CIA_CRB] & 0x80)) {
                    byte ^= 0x80;
                }
            } else if (addr == CIA_TOD_MIN) {
                byte &= 0x7f;
            } else if (addr == CIA_TOD_SEC) {
                byte &= 0x7f;
            } else if (addr == CIA_TOD_TEN) {
                byte &= 0x0f;
            }

            {
                char changed;
                if (cia_context->c_cia[CIA_CRB] & 0x80) {
                    /* set alarm */
                    changed = cia_context->todalarm[addr - CIA_TOD_TEN] != byte;
                    cia_context->todalarm[addr - CIA_TOD_TEN] = byte;
                } else {
                    /* set time */
                    if (addr == CIA_TOD_TEN) {
                        /* apparently the tickcounter is reset to 0 when the clock
                           is not running and then restarted by writing to the 10th
                           seconds register */
                        if (cia_context->todstopped) {
                            cia_context->todtickcounter = 0;
                        }
                        cia_context->todstopped = 0;
                    }
                    if (addr == CIA_TOD_HR) {
                        cia_context->todstopped = 1;
                    }
                    changed = cia_context->c_cia[addr] != byte;
                    cia_context->c_cia[addr] = byte;
                }
                if (changed) {
                    check_ciatodalarm(cia_context, rclk);
                }
            }
            break;

        case CIA_SDR:           /* Serial Port output buffer */
            cia_context->c_cia[addr] = byte;
            if ((cia_context->c_cia[CIA_CRA] & 0x40) == 0x40) {
                cia_context->sdr_valid = 1;
                cia_update_ta(cia_context, rclk);
                ciat_set_alarm(cia_context->ta, rclk);
#if 0
                if (cia_context->sr_bits <= 8) {
/*
                if (!(cia_context->sr_bits)) {
                    (cia_context->store_sdr)(cia[CIA_SDR]);
                }
*/
                    if (cia_context->sr_bits < 8) {
                        /* switch timer A alarm on again, if necessary */
/* FIXME
                    update_cia(rclk);
                    if (cia_tau) {
                        my_set_tai_clk(cia_tau + 1);
                    }
*/
                    }

                    cia_context->sr_bits += 8;
#if defined (CIA_TIMER_DEBUG)
                    if (cia_context->debugFlag) {
                        log_message(cia_context->log, "start SDR rclk=%d.",
                                    rclk);
                    }
#endif
                }
#endif
            }
            break;

        /* Interrupts */

        case CIA_ICR:           /* Interrupt Control Register */

            CIAT_LOGIN(("store_icr: rclk=%d, byte=%02x", rclk, byte));

            cia_update_ta(cia_context, rclk);
            cia_update_tb(cia_context, rclk);

#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log, "cia set CIA_ICR: 0x%x.", byte);
            }
#endif

            if (byte & CIA_IM_SET) {
                cia_context->c_cia[CIA_ICR] |= (byte & 0x7f);
            } else {
                cia_context->c_cia[CIA_ICR] &= ~(byte & 0x7f);
            }

#if defined(CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log,
                            "    set icr: ifr & ier & 0x7f -> %02x, int=%02x.",
                            cia_context->c_cia[CIA_ICR] & cia_context->irqflags
                            & 0x7f, cia_context->irqflags);
            }
#endif
            if (cia_context->c_cia[CIA_ICR] & cia_context->irqflags & 0x7f) {
                cia_do_set_int(cia_context, rclk + 1);
            }

            if (cia_context->c_cia[CIA_ICR] & CIA_IM_TA) {
                ciat_set_alarm(cia_context->ta, rclk);
            }
            if (cia_context->c_cia[CIA_ICR] & CIA_IM_TB) {
                ciat_set_alarm(cia_context->tb, rclk);
            }

            CIAT_LOGOUT((""));
            break;

        case CIA_CRA:           /* control register A */
            if ((byte & 1) && !(cia_context->c_cia[CIA_CRA] & 1)) {
                cia_context->tat = 1;
            }

            cia_update_ta(cia_context, rclk);

            ciat_set_ctrl(cia_context->ta, rclk, byte);

#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log,
                            "cia set CIA_CRA: 0x%x (clk=%d, pc=, tal=%u, tac=%u).",
                            byte, rclk, /*program_counter,*/ cia_tal, cia_tac);
            }
#endif

            /* bit 7 tod frequency */
            /* bit 6 serial port mode */

            /* bit 3 timer run mode */
            /* bit 2 & 1 timer output to PB6 */

            /* bit 0 start/stop timer */
            /* bit 5 timer count mode */

#if defined (CIA_TIMER_DEBUG)
            if (cia_context->debugFlag) {
                log_message(cia_context->log, "    -> tas=%d, tau=%d.", cia_tas,
                            cia_tau);
            }
#endif
            cia_context->c_cia[addr] = byte & 0xef;    /* remove strobe */

            break;

        case CIA_CRB:           /* control register B */
            if ((byte & 1) && !(cia_context->c_cia[CIA_CRB] & 1)) {
                cia_context->tbt = 1;
            }

            cia_update_ta(cia_context, rclk);
            cia_update_tb(cia_context, rclk);

            /* bit 5 is set when single-stepping is set */
            if (byte & 0x40) {
                /* we count ta - so we enable that */
                ciat_set_alarm(cia_context->ta, rclk);
                ciat_set_ctrl(cia_context->tb, rclk, (BYTE)(byte | 0x20));
            } else {
                ciat_set_ctrl(cia_context->tb, rclk, byte);
            }

#if 0 /* defined (CIA_TIMER_DEBUG)
        if (cia_context->debugFlag) */
            log_message(cia_context->log, "cia set CIA_CRB: 0x%x (clk=%d).",
                        byte, rclk);
#endif


            /* bit 7 set alarm/tod clock */
            /* bit 4 force load */

            /* bit 3 timer run mode */
            /* bit 2 & 1 timer output to PB6 */

            /* bit 0 stbrt/stop timer */
            /* bit 5 & 6 timer count mode */

            cia_context->c_cia[addr] = byte & 0xef;    /* remove strobe */
            break;

        default:
            cia_context->c_cia[addr] = byte;
    }                           /* switch */
}

void ciacore_store(cia_context_t *cia_context, WORD addr, BYTE byte)
{
    if (cia_context->pre_store != NULL) {
        (cia_context->pre_store)();
    }

    if (*(cia_context->rmw_flag)) {
        (*(cia_context->clk_ptr))--;
        ciacore_store_internal(cia_context, addr, cia_context->last_read);
        (*(cia_context->clk_ptr))++;
    }

    ciacore_store_internal(cia_context, addr, byte);
}

/* ------------------------------------------------------------------------- */


BYTE ciacore_read(cia_context_t *cia_context, WORD addr)
{
#if defined(CIA_TIMER_DEBUG)

    BYTE cia_read_(cia_context_t *, WORD addr);
    BYTE tmp = cia_read_(cia_context, addr);

    if (cia_context->debugFlag) {
        log_message(cia_context->log, "read cia[%x] returns %02x @ clk=%d.",
                    addr, tmp, *(cia_context->clk_ptr) - READ_OFFSET);
    }
    return tmp;
}

BYTE cia_read_(cia_context_t *cia_context, WORD addr)
{
#endif

    BYTE byte = 0xff;
    CLOCK rclk;

    addr &= 0xf;

    if (cia_context->pre_read != NULL) {
        (cia_context->pre_read)();
    }

    cia_context->read_clk = *(cia_context->clk_ptr);
    cia_context->read_offset = 0;
    rclk = *(cia_context->clk_ptr) - READ_OFFSET;

    if (cia_context->pre_read != NULL) {
        (cia_context->pre_read)();
    }

    switch (addr) {
        case CIA_PRA:           /* port A */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value. Value read might be different from what is
               expected due to excessive load. */
            cia_context->last_read = (cia_context->read_ciapa)(cia_context);
            return cia_context->last_read;
            break;

        case CIA_PRB:           /* port B */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value. Value read might be different from what is
               expected due to excessive load. */
            byte = (cia_context->read_ciapb)(cia_context);
            (cia_context->pulse_ciapc)(cia_context, rclk);
            if ((cia_context->c_cia[CIA_CRA]
                 | cia_context->c_cia[CIA_CRB]) & 0x02) {
                if (cia_context->c_cia[CIA_CRA] & 0x02) {
                    cia_update_ta(cia_context, rclk);
                    byte &= 0xbf;
                    if (((cia_context->c_cia[CIA_CRA] & 0x04) ? cia_context->tat
                         : ciat_is_underflow_clk(cia_context->ta, rclk))) {
                        byte |= 0x40;
                    }
                }
                if (cia_context->c_cia[CIA_CRB] & 0x02) {
                    cia_update_tb(cia_context, rclk);
                    byte &= 0x7f;
                    if (((cia_context->c_cia[CIA_CRB] & 0x04) ? cia_context->tbt
                         : ciat_is_underflow_clk(cia_context->tb, rclk))) {
                        byte |= 0x80;
                    }
                }
            }
            cia_context->last_read = byte;
            return byte;
            break;

        /* Timers */
        case CIA_TAL:           /* timer A low */
            cia_update_ta(cia_context, rclk);
            cia_context->last_read = ciat_read_timer(cia_context->ta, rclk)
                                     & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TAH:           /* timer A high */
            cia_update_ta(cia_context, rclk);
            cia_context->last_read = (ciat_read_timer(cia_context->ta, rclk)
                                      >> 8) & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TBL:           /* timer B low */
            cia_update_tb(cia_context, rclk);
            cia_context->last_read = ciat_read_timer(cia_context->tb, rclk)
                                     & 0xff;
            return cia_context->last_read;
            break;

        case CIA_TBH:           /* timer B high */
            cia_update_tb(cia_context, rclk);
            cia_context->last_read = (ciat_read_timer(cia_context->tb, rclk)
                                      >> 8) & 0xff;
            return cia_context->last_read;
            break;

        /*
         * TOD clock is latched by reading Hours, and released
         * upon reading Tenths of Seconds. The counter itself
         * keeps ticking all the time.
         * Also note that this latching is different from the input one.
         */
        case CIA_TOD_TEN: /* Time Of Day clock 1/10 s */
        case CIA_TOD_SEC: /* Time Of Day clock sec */
        case CIA_TOD_MIN: /* Time Of Day clock min */
        case CIA_TOD_HR:  /* Time Of Day clock hour */
            if (!(cia_context->todlatched)) {
                memcpy(cia_context->todlatch, cia_context->c_cia + CIA_TOD_TEN,
                       sizeof(cia_context->todlatch));
            }
            if (addr == CIA_TOD_TEN) {
                cia_context->todlatched = 0;
            }
            if (addr == CIA_TOD_HR) {
                cia_context->todlatched = 1;
            }
            cia_context->last_read = cia_context->todlatch[addr - CIA_TOD_TEN];
            return cia_context->last_read;
            break;

        case CIA_SDR:           /* Serial Port Shift Register */
            (cia_context->read_sdr)(cia_context);
            cia_context->last_read = cia_context->c_cia[CIA_SDR];
            return cia_context->last_read;
            break;

        /* Interrupts */

        case CIA_ICR:           /* Interrupt Flag Register */
            {
                BYTE t = 0;

                CIAT_LOGIN(("read_icr: rclk=%d, rdi=%d", rclk, cia_context->rdi));

                cia_context->rdi = rclk;

                cia_update_ta(cia_context, rclk);
                cia_update_tb(cia_context, rclk);

                (cia_context->read_ciaicr)(cia_context);

#ifdef CIA_TIMER_DEBUG
                if (cia_context->debugFlag) {
                    log_message(cia_context->log,
                                "cia read intfl: rclk=%d, alarm_ta=%d, alarm_tb=%d, ciaint=%02x",
                                rclk, cia_tai, cia_tbi,
                                (int)(cia_context->irqflags));
                }
#endif

                ciat_set_alarm(cia_context->ta, rclk);
                ciat_set_alarm(cia_context->tb, rclk);

                CIAT_LOG(("read_icr -> ta alarm at %d, tb at %d",
                          ciat_alarm_clk(cia_context->ta),
                          ciat_alarm_clk(cia_context->tb)));

                if (cia_context->irqflags & CIA_IM_TBB) {
                    /* timer b bug */
                    cia_context->irqflags &= ~(CIA_IM_TBB | CIA_IM_TB);
                }

                t = cia_context->irqflags;

                CIAT_LOG(("read intfl gives ciaint=%02x -> %02x "
                          "sr_bits=%d, rclk=%d",
                          cia_context->irqflags, t, cia_context->sr_bits, rclk));

                cia_context->irqflags = 0;
                my_set_int(cia_context, 0, rclk);

                CIAT_LOGOUT((""));

                cia_context->last_read = t;

                return t;
            }
            break;

        case CIA_CRA:           /* Control Register A */
            cia_update_ta(cia_context, rclk);
            cia_context->last_read = (cia_context->c_cia[CIA_CRA] & 0xfe)
                                     | ciat_is_running(cia_context->ta, rclk);
            return cia_context->last_read;
            break;

        case CIA_CRB:           /* Control Register B */
            cia_update_tb(cia_context, rclk);
            cia_context->last_read = (cia_context->c_cia[CIA_CRB] & 0xfe)
                                     | ciat_is_running(cia_context->tb, rclk);
            return cia_context->last_read;
            break;
    }                           /* switch */

    cia_context->last_read = cia_context->c_cia[addr];
    return (cia_context->c_cia[addr]);
}

BYTE ciacore_peek(cia_context_t *cia_context, WORD addr)
{
    /* This code assumes that update_cia is a projector - called at
     * the same cycle again it doesn't change anything. This way
     * it does not matter if we call it from peek first in the monitor
     * and probably the same cycle again when the CPU runs on...
     */
    CLOCK rclk;
    BYTE byte;

    addr &= 0xf;

    if (cia_context->pre_peek != NULL) {
        (cia_context->pre_peek)();
    }

    rclk = *(cia_context->clk_ptr) - READ_OFFSET;

    switch (addr) {
        case CIA_PRB:           /* port B */
            /* WARNING: this pin reads the voltage of the output pins, not
               the ORA value. Value read might be different from what is
               expected due to excessive load. */
            byte = (cia_context->read_ciapb)(cia_context);
            /* (cia_context->pulse_ciapc)(rclk); */
            if ((cia_context->c_cia[CIA_CRA] | cia_context->c_cia[CIA_CRB])
                & 0x02) {
                if (cia_context->c_cia[CIA_CRA] & 0x02) {
                    cia_update_ta(cia_context, rclk);
                    byte &= 0xbf;
                    if (((cia_context->c_cia[CIA_CRA] & 0x04) ? cia_context->tat
                         : ciat_is_underflow_clk(cia_context->ta, rclk))) {
                        byte |= 0x40;
                    }
                }
                if (cia_context->c_cia[CIA_CRB] & 0x02) {
                    cia_update_tb(cia_context, rclk);
                    byte &= 0x7f;
                    if (((cia_context->c_cia[CIA_CRB] & 0x04) ? cia_context->tbt
                         : ciat_is_underflow_clk(cia_context->tb, rclk))) {
                        byte |= 0x80;
                    }
                }
            }
            return byte;
            break;

        /*
         * TOD clock is latched by reading Hours, and released
         * upon reading Tenths of Seconds. The counter itself
         * keeps ticking all the time.
         * Also note that this latching is different from the input one.
         */
        case CIA_TOD_TEN: /* Time Of Day clock 1/10 s */
        case CIA_TOD_SEC: /* Time Of Day clock sec */
        case CIA_TOD_MIN: /* Time Of Day clock min */
        case CIA_TOD_HR: /* Time Of Day clock hour */
            if (!(cia_context->todlatched)) {
                memcpy(cia_context->todlatch, cia_context->c_cia + CIA_TOD_TEN,
                       sizeof(cia_context->todlatch));
            }
            return cia_context->c_cia[addr];

        /* Interrupts */

        case CIA_ICR:           /* Interrupt Flag Register */
            {
                BYTE t = 0;

                CIAT_LOGIN(("peek_icr: rclk=%d, rdi=%d", rclk, cia_context->rdi));

                /* cia_context->rdi = rclk; */

                cia_update_ta(cia_context, rclk);
                cia_update_tb(cia_context, rclk);

                /* read_ciaicr(); */

#ifdef CIA_TIMER_DEBUG
                if (cia_context->debugFlag) {
                    log_message(cia_context->log,
                                "cia read intfl: rclk=%d, alarm_ta=%d, alarm_tb=%d, ciaint=%02x",
                                rclk, cia_tai, cia_tbi, (int)(cia_context->irqflags));
                }
#endif

                ciat_set_alarm(cia_context->ta, rclk);
                ciat_set_alarm(cia_context->tb, rclk);

                CIAT_LOG(("peek_icr -> ta alarm at %d, tb at %d",
                          ciat_alarm_clk(cia_context->ta),
                          ciat_alarm_clk(cia_context->tb)));

                t = cia_context->irqflags;

                CIAT_LOG(("peek intfl gives ciaint=%02x -> %02x "
                          "sr_bits=%d, rclk=%d",
                          cia_context->irqflags, t, cia_context->sr_bits, rclk));
/*
            cia_context->irqflags = 0;
            my_set_int(0, rclk + 1);
*/
                CIAT_LOGOUT((""));

                return (t);
            }
        default:
            break;
    }                           /* switch */

    return ciacore_read(cia_context, addr);
}

/* ------------------------------------------------------------------------- */


static void ciacore_intta(CLOCK offset, void *data)
{
    CLOCK rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    CIAT_LOGIN(("ciaTimerA ciacore_intta: myclk=%d rclk=%d",
                *(cia_context->clk_ptr), rclk));

#if 0
    if ((n = ciat_update(cia_context->ta, rclk))
        && (cia_context->rdi != rclk - 1)) {
        cia_context->irqflags |= CIA_IM_TA;
        cia_context->tat = (cia_context->tat + n) & 1;
    }
#else
    cia_do_update_ta(cia_context, rclk);
#endif

    ciat_ack_alarm(cia_context->ta, rclk);

    CIAT_LOG((
                 "ciacore_intta(rclk = %u, tal = %u, cra=%02x, int=%02x, ier=%02x.",
                 rclk, ciat_read_latch(cia_context->ta, rclk), cia_context->c_cia[CIA_CRA],
                 cia_context->irqflags, cia_context->c_cia[CIA_ICR]));

    /* cia_context->tat = (cia_context->tat + 1) & 1; */

    if ((cia_context->c_cia[CIA_CRA] & 0x29) == 0x01) {
        /* if we do not need alarm, no PB6, no shift register, and not timer B
           counting timer A, then we can savely skip alarms... */
        if (((cia_context->c_cia[CIA_ICR] & CIA_IM_TA) &&
             (!(cia_context->irqflags & 0x80)))
            || (cia_context->c_cia[CIA_CRA] & 0x42)
            || (cia_context->c_cia[CIA_CRB] & 0x40)) {
            ciat_set_alarm(cia_context->ta, rclk);
        }
    }

    if (cia_context->c_cia[CIA_CRA] & 0x40) {
        if (cia_context->sr_bits) {
            CIAT_LOG(("rclk=%d SDR: timer A underflow, bits=%d",
                      rclk, cia_context->sr_bits));

            if (!(--(cia_context->sr_bits))) {
                cia_context->irqflags |= CIA_IM_SDR;
                /*printf("%s: rclk=%d, store_sdr(%02x, '%c'\n",
                cia_context->myname,
                rclk, cia_context->shifter);*/
                (cia_context->store_sdr)(cia_context, cia_context->shifter);
            }
        }
        if ((!(cia_context->sr_bits)) && cia_context->sdr_valid) {
            cia_context->shifter = cia_context->c_cia[CIA_SDR];
            cia_context->sdr_valid = 0;
            cia_context->sr_bits = 14;
        }
    }
    if ((cia_context->c_cia[CIA_CRB] & 0x41) == 0x41) {
        cia_update_tb(cia_context, rclk);
        cia_do_step_tb(cia_context, rclk);
    }

    cia_do_set_int(cia_context, rclk);

    CIAT_LOGOUT((""));
}


/*
 * Timer B can run in 2 (4) modes
 * cia[f] & 0x60 == 0x00   count system 02 pulses
 * cia[f] & 0x60 == 0x40   count timer A underflows
 * cia[f] & 0x60 == 0x20 | 0x60 count CNT pulses => counter stops
 */


static void ciacore_inttb(CLOCK offset, void *data)
{
    CLOCK rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    CIAT_LOGIN(("ciaTimerB int_myciatb: myclk=%d, rclk=%d",
                *(cia_context->clk_ptr), rclk));

    cia_do_update_tb(cia_context, rclk);

    ciat_ack_alarm(cia_context->tb, rclk);

    CIAT_LOG((
                 "timer B ciacore_inttb(rclk=%d, crb=%d, int=%02x, ier=%02x).",
                 rclk, cia_context->c_cia[CIA_CRB], cia_context->irqflags,
                 cia_context->c_cia[CIA_ICR]));

    /* cia_context->tbt = (cia_context->tbt + 1) & 1; */

    /* running and continous, then next alarm */
    if ((cia_context->c_cia[CIA_CRB] & 0x69) == 0x01) {
        /* if no interrupt flag we can safely skip alarms */
        if (cia_context->c_cia[CIA_ICR] & CIA_IM_TB) {
            ciat_set_alarm(cia_context->tb, rclk);
        }
    }

    cia_do_set_int(cia_context, rclk);

    CIAT_LOGOUT((""));
}

/* ------------------------------------------------------------------------- */

void ciacore_set_flag(cia_context_t *cia_context)
{
    cia_context->irqflags |= CIA_IM_FLG;
    if (cia_context->c_cia[CIA_ICR] & CIA_IM_FLG) {
        cia_context->irqflags |= 0x80;
        my_set_int(cia_context, cia_context->irq_line,
                   *(cia_context->clk_ptr));
    }
}

void ciacore_set_sdr(cia_context_t *cia_context, BYTE data)
{
    if (!(cia_context->c_cia[CIA_CRA] & 0x40)) {
        cia_context->c_cia[CIA_SDR] = data;
        cia_context->irqflags |= CIA_IM_SDR;
        if (cia_context->c_cia[CIA_ICR] & CIA_IM_SDR) {
            cia_context->irqflags |= 0x80;
            my_set_int(cia_context, cia_context->irq_line,
                       *(cia_context->clk_ptr));
        }
    }
}

/* ------------------------------------------------------------------------- */

/* when defined, randomize the power frequency a bit instead of linearly
   meandering around the correct value */
#define TODRANDOM

static void ciacore_inttod(CLOCK offset, void *data)
{
    int t0, t1, t2, t3, t4, t5, t6, pm, update = 0;
    CLOCK rclk, tclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    rclk = *(cia_context->clk_ptr) - offset;

    if (cia_context->power_freq == 0) {
        /* if power frequency is not initialized, or not present, return immediatly.
           check again in about 1/10th second */
        cia_context->todclk = *(cia_context->clk_ptr) + 100000;
        alarm_set(cia_context->tod_alarm, cia_context->todclk);
        return;
    }

    /* set up new int 
       the time between power ticks should be ticks_per_sec / power_freq
       in reality the deviation can be quite large in small time frames, but is
       very accurate in longer time frames. we try to maintain a stable tick
       frequency that is as close as possible to the wanted 50/60 Hz. we should
       also introduce some kind of randomness to mimic realistic behaviour.

       for some details (german) http://www.netzfrequenzmessung.de/
     */
    cia_context->todticks = cia_context->ticks_per_sec / cia_context->power_freq;
    tclk = ((cia_context->power_tickcounter * cia_context->ticks_per_sec) / cia_context->power_freq);
    if (cia_context->power_ticks < tclk) {
#ifdef TODRANDOM
          cia_context->todticks += lib_unsigned_rand(0, 3);
#else
          /* cia_context->todticks += (((tclk - cia_context->power_ticks) * 3) / 2); */
          cia_context->todticks++;
#endif
    } else if (cia_context->power_ticks > tclk) {
#ifdef TODRANDOM
          cia_context->todticks -= lib_unsigned_rand(0, 3);
#else
          /* cia_context->todticks -= (((cia_context->power_ticks - tclk) * 3) / 2); */
          cia_context->todticks--;
#endif
    }
    cia_context->power_tickcounter++;
    if (cia_context->power_tickcounter >= cia_context->power_freq) {
        cia_context->todticks = cia_context->ticks_per_sec - cia_context->power_ticks;
        cia_context->power_tickcounter = 0;
        cia_context->power_ticks = 0;
    } else {
        cia_context->power_ticks += cia_context->todticks;
    }

    cia_context->todclk = *(cia_context->clk_ptr) + cia_context->todticks;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);

    if (!(cia_context->todstopped)) {
        /* count 50/60 hz ticks */
        cia_context->todtickcounter++;
        /* wild assumption: counter is 3 bits and is not reset elsewhere */
        /* FIXME: this doesnt seem to be 100% correct - apparently it is reset
                  in some cases */
        cia_context->todtickcounter &= 7;

        /* if the counter matches the TOD frequency ... */
        if (cia_context->todtickcounter ==
            ((cia_context->c_cia[CIA_CRA] & 0x80) ? 5 : 6)) {
            /* reset the counter and update the timer */
            cia_context->todtickcounter = 0;
            update = 1;
        }
    }

    if (update) {
        /* advance the counters.
         * - individual counters are all 4 bit
         */
        t0 = cia_context->c_cia[CIA_TOD_TEN] & 0x0f;
        t1 = cia_context->c_cia[CIA_TOD_SEC] & 0x0f;
        t2 = (cia_context->c_cia[CIA_TOD_SEC] >> 4) & 0x0f;
        t3 = cia_context->c_cia[CIA_TOD_MIN] & 0x0f;
        t4 = (cia_context->c_cia[CIA_TOD_MIN] >> 4) & 0x0f;
        t5 = cia_context->c_cia[CIA_TOD_HR] & 0x0f;
        t6 = (cia_context->c_cia[CIA_TOD_HR] >> 4) & 0x01;
        pm = cia_context->c_cia[CIA_TOD_HR] & 0x80;

        /* tenth seconds (0-9) */
        t0 = (t0 + 1) & 0x0f;
        if (t0 == 10) {
            t0 = 0;
            /* seconds (0-59) */
            t1 = (t1 + 1) & 0x0f; /* x0...x9 */
            if (t1 == 10) {
                t1 = 0;
                t2 = (t2 + 1) & 0x07; /* 0x...5x */
                if (t2 == 6) {
                    t2 = 0;
                    /* minutes (0-59) */
                    t3 = (t3 + 1) & 0x0f; /* x0...x9 */
                    if (t3 == 10) {
                        t3 = 0;
                        t4 = (t4 + 1) & 0x07; /* 0x...5x */
                        if (t4 == 6) {
                            t4 = 0;
                            /* hours (1-12) */
                            t5 = (t5 + 1) & 0x0f;
                            if (t6) {
                                /* toggle the am/pm flag when going from 11 to 12 (!) */
                                if (t5 == 2) {
                                    pm ^= 0x80;
                                }
                                /* wrap 12h -> 1h (FIXME: when hour became x3 ?) */
                                if (t5 == 3) {
                                    t5 = 1;
                                    t6 = 0;
                                }
                            } else {
                                if (t5 == 10) {
                                    t5 = 0;
                                    t6 = 1;
                                }
                            }
                        }
                    }
                }
            }
        }

        DBG(("ciacore_inttod [%s %02x:%02x:%02x.%x0]->[%s %x%x:%x%x:%x%x.%x0]\n",
             cia_context->c_cia[CIA_TOD_HR] & 0x80 ? "pm" : "am",
             cia_context->c_cia[CIA_TOD_HR] & 0x7f,
             cia_context->c_cia[CIA_TOD_MIN],
             cia_context->c_cia[CIA_TOD_SEC],
             cia_context->c_cia[CIA_TOD_TEN],
             pm ? "pm" : "am", t6, t5, t4, t3, t2, t1, t0));

        cia_context->c_cia[CIA_TOD_TEN] = t0;
        cia_context->c_cia[CIA_TOD_SEC] = t1 | (t2 << 4);
        cia_context->c_cia[CIA_TOD_MIN] = t3 | (t4 << 4);
        cia_context->c_cia[CIA_TOD_HR] = t5 | (t6 << 4) | pm;

        /* check alarm */
        check_ciatodalarm(cia_context, rclk);
    }
}
#undef TODRANDOM

void ciacore_setup_context(cia_context_t *cia_context)
{
    cia_context->log = LOG_ERR;
    cia_context->read_clk = 0;
    cia_context->read_offset = 0;
    cia_context->last_read = 0;
    cia_context->write_offset = 1;
    cia_context->model = 0;
}

#define USE_IDLE_CALLBACK

#ifdef USE_IDLE_CALLBACK
/*
    we must take care to choose a value which is small enough so the counters do
    not fall behind too much to cause a significant peak in cpu usage, and one
    that is big enough so the overall performance impact is not too big.
    it seems reasonable to also consider how peaks in cpu usage interact with
    automatic framerate adjustment, so choosing a value that makes sure a more
    or less constant amount of cpu time per frame is consumed is a good idea.
    (about 20000 cycles are a full PAL frame on the C64, making sure that we do
    not fall behind one frame at all seems a good idea.)
 */
#define CIA_MAX_IDLE_CYCLES     5000
/*
    this callback takes care of the problem that when ciat_update has to catch
    up with an excessive amount of clock cycles it will consume a lot of cpu
    time, in the worst case leading to a noticeable stall of the entire emulation
    (see bug #3424428)

    FIXME: maybe other stuff must be handled here
 */
static void ciacore_idle(CLOCK offset, void *data)
{
    CLOCK clk, rclk;
    cia_context_t *cia_context = (cia_context_t *)data;

    clk = *(cia_context->clk_ptr);
    rclk = clk - offset;

/* printf("ciacore_idle: clk=%d rclk=%d\n", clk, rclk); */

    cia_update_ta(cia_context, rclk);
    cia_update_tb(cia_context, rclk);

    alarm_set(cia_context->idle_alarm, rclk + CIA_MAX_IDLE_CYCLES);
}
#endif

void ciacore_init(cia_context_t *cia_context, alarm_context_t *alarm_context,
                  interrupt_cpu_status_t *int_status, clk_guard_t *clk_guard)
{
    char *buffer;

    cia_context->ta = lib_calloc(1, sizeof(ciat_t));
    cia_context->tb = lib_calloc(1, sizeof(ciat_t));

    ciat_init_table();

    cia_context->log = log_open(cia_context->myname);
#ifdef USE_IDLE_CALLBACK
    buffer = lib_msprintf("%s_IDLE", cia_context->myname);
    cia_context->idle_alarm = alarm_new(alarm_context, buffer, ciacore_idle,
                                        (void *)cia_context);
    lib_free(buffer);
    alarm_set(cia_context->idle_alarm, *(cia_context->clk_ptr) + CIA_MAX_IDLE_CYCLES);
#endif
    buffer = lib_msprintf("%s_TA", cia_context->myname);
    cia_context->ta_alarm = alarm_new(alarm_context, buffer, ciacore_intta,
                                      (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TB", cia_context->myname);
    cia_context->tb_alarm = alarm_new(alarm_context, buffer, ciacore_inttb,
                                      (void *)cia_context);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TOD", cia_context->myname);
    cia_context->tod_alarm = alarm_new(alarm_context, buffer, ciacore_inttod,
                                       (void *)cia_context);
    lib_free(buffer);

    cia_context->int_num
        = interrupt_cpu_status_int_new(int_status, cia_context->myname);

    clk_guard_add_callback(clk_guard, ciacore_clk_overflow_callback,
                           cia_context);

    buffer = lib_msprintf("%s_TA", cia_context->myname);
    ciat_init(cia_context->ta, buffer, *(cia_context->clk_ptr),
              cia_context->ta_alarm);
    lib_free(buffer);

    buffer = lib_msprintf("%s_TB", cia_context->myname);
    ciat_init(cia_context->tb, buffer, *(cia_context->clk_ptr),
              cia_context->tb_alarm);
    lib_free(buffer);
}

void ciacore_shutdown(cia_context_t *cia_context)
{
    lib_free(cia_context->prv);
    lib_free(cia_context->ta);
    lib_free(cia_context->tb);
    lib_free(cia_context->myname);
    lib_free(cia_context);
}

/* -------------------------------------------------------------------------- */
/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 2.2
 */

#define CIA_DUMP_VER_MAJOR      2
#define CIA_DUMP_VER_MINOR      2

/*
 * The dump data:
 *
 * UBYTE        ORA
 * UBYTE        ORB
 * UBYTE        DDRA
 * UBYTE        DDRB
 * UWORD        TA
 * UWORD        TB
 * UBYTE        TOD_TEN         current value
 * UBYTE        TOD_SEC
 * UBYTE        TOD_MIN
 * UBYTE        TOD_HR
 * UBYTE        SDR
 * UBYTE        IER             enabled interrupts mask
 * UBYTE        CRA
 * UBYTE        CRB
 *
 * UWORD        TAL             latch value
 * UWORD        TBL             latch value
 * UBYTE        IFR             interrupts currently active
 * UBYTE        PBSTATE         bit6/7 reflect PB6/7 toggle state
 *                              bit 2/3 reflect port bit state
 * UBYTE        SRHBITS         number of half-bits to still shift in/out SDR
 * UBYTE        ALARM_TEN
 * UBYTE        ALARM_SEC
 * UBYTE        ALARM_MIN
 * UBYTE        ALARM_HR
 *
 * UBYTE        READICR         clk - when ICR was read last + 128
 * UBYTE        TODLATCHED      0=running, 1=latched, 2=stopped (writing)
 * UBYTE        TODL_TEN                latch value
 * UBYTE        TODL_SEC
 * UBYTE        TODL_MIN
 * UBYTE        TODL_HR
 * DWORD        TOD_TICKS       clk ticks till next tenth of second
 *
 * UBYTE        IRQ             0=IRQ line inactive, 1=IRQ line active
 *
 *                              These bits have been added in V1.1
 *
 * WORD         TA              Timer A state bits (see ciatimer.h)
 * WORD         TB              Timer B state bits (see ciatimer.h)
 *
 *                              These bits have been added in V1.2
 *
 * BYTE         SHIFTER         actual shift register (original data)
 * BYTE         SDR_VALID       data in SDR valid to be shifted
 *
 * BYTE         irq_enabled     IRQ enabled
 *
 * BYTE         todtickcounter  TOD tick counter
 */

/* FIXME!!!  Error check.  */
int ciacore_snapshot_write_module(cia_context_t *cia_context, snapshot_t *s)
{
    snapshot_module_t *m;
    int byte;

    cia_update_ta(cia_context, *(cia_context->clk_ptr));
    cia_update_tb(cia_context, *(cia_context->clk_ptr));

    m = snapshot_module_create(s, cia_context->myname,
                               (BYTE)CIA_DUMP_VER_MAJOR,
                               (BYTE)CIA_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }


#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                *(cia_context->clk_ptr), cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
    log_message(cia_context->log, "write ciaint=%02x, ciaier=%02x",
                cia_context->irqflags,
                cia_context->c_cia[CIA_ICR]);
#endif

    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_PRA]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_PRB]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_DDRA]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_DDRB]));
    SMW_W(m, ciat_read_timer(cia_context->ta, *(cia_context->clk_ptr)));
    SMW_W(m, ciat_read_timer(cia_context->tb, *(cia_context->clk_ptr)));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_TOD_TEN]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_TOD_SEC]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_TOD_MIN]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_TOD_HR]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_SDR]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_ICR]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_CRA]));
    SMW_B(m, (BYTE)(cia_context->c_cia[CIA_CRB]));

    SMW_W(m, ciat_read_latch(cia_context->ta, *(cia_context->clk_ptr)));
    SMW_W(m, ciat_read_latch(cia_context->tb, *(cia_context->clk_ptr)));
    SMW_B(m, ciacore_peek(cia_context, CIA_ICR));

    /* Bits 2 & 3 are compatibility to snapshot format v1.0 */
    SMW_B(m, (BYTE)((cia_context->tat ? 0x40 : 0)
                    | (cia_context->tbt ? 0x80 : 0)
                    | (ciat_is_underflow_clk(cia_context->ta,
                                             *(cia_context->clk_ptr)) ? 0x04 : 0)
                    | (ciat_is_underflow_clk(cia_context->tb, *(cia_context->clk_ptr))
                       ? 0x08 : 0)));
    SMW_B(m, (BYTE)cia_context->sr_bits);
    SMW_B(m, cia_context->todalarm[0]);
    SMW_B(m, cia_context->todalarm[1]);
    SMW_B(m, cia_context->todalarm[2]);
    SMW_B(m, cia_context->todalarm[3]);

    if (cia_context->rdi) {
        if ((*(cia_context->clk_ptr) - cia_context->rdi) > 120) {
            byte = 0;
        } else {
            byte = *(cia_context->clk_ptr) + 128 - cia_context->rdi;
        }
    } else {
        byte = 0;
    }
    SMW_B(m, (BYTE)(byte));

    SMW_B(m, (BYTE)((cia_context->todlatched ? 1 : 0)
                    | (cia_context->todstopped ? 2 : 0)));
    SMW_B(m, cia_context->todlatch[0]);
    SMW_B(m, cia_context->todlatch[1]);
    SMW_B(m, cia_context->todlatch[2]);
    SMW_B(m, cia_context->todlatch[3]);

    SMW_DW(m, (cia_context->todclk - *(cia_context->clk_ptr)));

    ciat_save_snapshot(cia_context->ta, *(cia_context->clk_ptr), m,
                       (CIA_DUMP_VER_MAJOR << 8) | CIA_DUMP_VER_MINOR);
    ciat_save_snapshot(cia_context->tb, *(cia_context->clk_ptr), m,
                       (CIA_DUMP_VER_MAJOR << 8) | CIA_DUMP_VER_MINOR);

    SMW_B(m, cia_context->shifter);
    SMW_B(m, (BYTE)(cia_context->sdr_valid));
    /* This has to be tested */
    SMW_B(m, cia_context->irq_enabled);

    SMW_B(m, cia_context->todtickcounter);

    snapshot_module_close(m);

    return 0;
}

int ciacore_snapshot_read_module(cia_context_t *cia_context, snapshot_t *s)
{
    BYTE vmajor, vminor;
    BYTE byte;
    DWORD dword;
    CLOCK rclk = *(cia_context->clk_ptr);
    snapshot_module_t *m;
    WORD cia_tal, cia_tbl, cia_tac, cia_tbc;

    m = snapshot_module_open(s, cia_context->myname, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    if (vmajor != CIA_DUMP_VER_MAJOR) {
        log_error(cia_context->log,
                  "Snapshot module version (%d.%d) newer than %d.%d.",
                  vmajor, vminor, CIA_DUMP_VER_MAJOR, CIA_DUMP_VER_MINOR);
        snapshot_module_close(m);
        return -1;
    }

    ciacore_reset(cia_context);

    /* stop timers, just in case */
    ciat_set_ctrl(cia_context->ta, *(cia_context->clk_ptr), 0);
    ciat_set_ctrl(cia_context->tb, *(cia_context->clk_ptr), 0);
    alarm_unset(cia_context->tod_alarm);

    {
        SMR_B(m, &(cia_context->c_cia[CIA_PRA]));
        SMR_B(m, &(cia_context->c_cia[CIA_PRB]));
        SMR_B(m, &(cia_context->c_cia[CIA_DDRA]));
        SMR_B(m, &(cia_context->c_cia[CIA_DDRB]));

        byte = cia_context->c_cia[CIA_PRA] | ~(cia_context->c_cia[CIA_DDRA]);
        cia_context->old_pa = byte ^ 0xff;    /* all bits change? */
        (cia_context->undump_ciapa)(cia_context, rclk, byte);
        cia_context->old_pa = byte;

        byte = cia_context->c_cia[CIA_PRB] | ~(cia_context->c_cia[CIA_DDRB]);
        cia_context->old_pb = byte ^ 0xff;    /* all bits change? */
        (cia_context->undump_ciapb)(cia_context, rclk, byte);
        cia_context->old_pb = byte;
    }

    SMR_W(m, &cia_tac);
    SMR_W(m, &cia_tbc);
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_TEN]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_SEC]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_MIN]));
    SMR_B(m, &(cia_context->c_cia[CIA_TOD_HR]));
    SMR_B(m, &(cia_context->c_cia[CIA_SDR]));

    SMR_B(m, &(cia_context->c_cia[CIA_ICR]));
    SMR_B(m, &(cia_context->c_cia[CIA_CRA]));
    SMR_B(m, &(cia_context->c_cia[CIA_CRB]));

    SMR_W(m, &cia_tal);
    SMR_W(m, &cia_tbl);

    SMR_B(m, &byte);
    cia_context->irqflags = byte;

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log,
                "read ciaint=%02x, ciaier=%02x.", cia_context->irqflags,
                cia_context->c_cia[CIA_ICR]);
#endif

    SMR_B(m, &byte);
    cia_context->tat = (byte & 0x40) ? 1 : 0;
    cia_context->tbt = (byte & 0x80) ? 1 : 0;

    SMR_B(m, &byte);
    cia_context->sr_bits = byte;

    SMR_B(m, &(cia_context->todalarm[0]));
    SMR_B(m, &(cia_context->todalarm[1]));
    SMR_B(m, &(cia_context->todalarm[2]));
    SMR_B(m, &(cia_context->todalarm[3]));

    SMR_B(m, &byte);
    if (byte) {
        cia_context->rdi = *(cia_context->clk_ptr) + 128 - byte;
    } else {
        cia_context->rdi = 0;
    }
#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "snap read rdi=%02x", byte);
    log_message(cia_context->log, "snap setting rdi to %d (rclk=%d)",
                cia_context->rdi,
                *(cia_context->clk_ptr));
#endif

    SMR_B(m, &byte);
    cia_context->todlatched = byte & 1;
    cia_context->todstopped = byte & 2;
    SMR_B(m, &(cia_context->todlatch[0]));
    SMR_B(m, &(cia_context->todlatch[1]));
    SMR_B(m, &(cia_context->todlatch[2]));
    SMR_B(m, &(cia_context->todlatch[3]));

    SMR_DW(m, &dword);
    cia_context->todclk = *(cia_context->clk_ptr) + dword;
    alarm_set(cia_context->tod_alarm, cia_context->todclk);

    /* timer switch-on code from store_cia[CIA_CRA/CRB] */

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                *(cia_context->clk_ptr), cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
#endif

    ciat_load_snapshot(cia_context->ta, rclk, cia_tac, cia_tal,
                       cia_context->c_cia[CIA_CRA], m, (vmajor << 8) | vminor);
    ciat_load_snapshot(cia_context->tb, rclk, cia_tbc, cia_tbl,
                       cia_context->c_cia[CIA_CRB], m, (vmajor << 8) | vminor);

    if (vminor > 1) {
        SMR_B(m, &(cia_context->shifter));
        SMR_B(m, &byte);
        cia_context->sdr_valid = byte;
    }

#ifdef cia_DUMP_DEBUG
    log_message(cia_context->log, "clk=%d, cra=%02x, crb=%02x, tas=%d, tbs=%d",
                *(cia_context->clk_ptr), cia[CIA_CRA], cia[CIA_CRB], cia_tas,
                cia_tbs);
    log_message(cia_context->log, "tai=%d, tau=%d, tac=%04x, tal=%04x",
                cia_tai, cia_tau, cia_tac, cia_tal);
    log_message(cia_context->log, "tbi=%d, tbu=%d, tbc=%04x, tbl=%04x",
                cia_tbi, cia_tbu, cia_tbc, cia_tbl);
#endif

    if (SMR_B(m, &(cia_context->irq_enabled)) < 0) {
        /* old (buggy) way to restore interrupt */
        /* still enabled; will be fixed in 1.13.x */
        cia_context->irq_enabled = (cia_context->c_cia[CIA_ICR] & 0x80) ? 1 : 0;
    }

    if (cia_context->irq_enabled) {
        (cia_context->cia_restore_int)(cia_context, cia_context->irq_line);
    } else {
        (cia_context->cia_restore_int)(cia_context, 0);
    }

    SMR_B(m, &(cia_context->todtickcounter));

    if (snapshot_module_close(m) < 0) {
        return -1;
    }

    return 0;
}

int ciacore_dump(cia_context_t *cia_context)
{
    mon_out("ICR: %02x CTRLA: %02x CTRLB: %02x\n\n", ciacore_peek(cia_context, 0x0d), ciacore_peek(cia_context, 0x0e), ciacore_peek(cia_context, 0x0f));
    mon_out("ICR write: %02x Timer A IRQ: %s Timer B IRQ: %s TOD IRQ: %s Serial IRQ: %s Cassette IRQ: %s\n\n",
        cia_context->c_cia[CIA_ICR],
        (cia_context->c_cia[CIA_ICR] & 1) ? "on" : "off",
        (cia_context->c_cia[CIA_ICR] & (1<<1)) ? "on" : "off",
        (cia_context->c_cia[CIA_ICR] & (1<<2)) ? "on" : "off",
        (cia_context->c_cia[CIA_ICR] & (1<<3)) ? "on" : "off",
        (cia_context->c_cia[CIA_ICR] & (1<<4)) ? "on" : "off");
    mon_out("Port A:  %02x DDR: %02x\n", ciacore_peek(cia_context, 0x00), ciacore_peek(cia_context, 0x02));
    mon_out("Port B:  %02x DDR: %02x\n", ciacore_peek(cia_context, 0x01), ciacore_peek(cia_context, 0x03));
    mon_out("Timer A: %04x\n", ciacore_peek(cia_context, 0x04) + (ciacore_peek(cia_context, 0x05) << 8));
    mon_out("Timer B: %04x\n", ciacore_peek(cia_context, 0x06) + (ciacore_peek(cia_context, 0x07) << 8));
    mon_out("TOD Time:  %02x:%02x:%02x.%x (%s)\n", ciacore_peek(cia_context, 0x0b) & 0x7f, ciacore_peek(cia_context, 0x0a), ciacore_peek(cia_context, 0x09), ciacore_peek(cia_context, 0x08), ciacore_peek(cia_context, 0x0b) & 0x80 ? "pm" : "am");
    mon_out("TOD Alarm: %02x:%02x:%02x.%x (%s)\n", cia_context->todalarm[0x0b - CIA_TOD_TEN] & 0x7f, cia_context->todalarm[0x0a - CIA_TOD_TEN], cia_context->todalarm[0x09 - CIA_TOD_TEN], cia_context->todalarm[0x08 - CIA_TOD_TEN], cia_context->todalarm[0x0b - CIA_TOD_TEN] & 0x80 ? "pm" : "am");
    mon_out("\nSynchronous Serial I/O Data Buffer: %02x\n", ciacore_peek(cia_context, 0x0c));
    return 0;
}
