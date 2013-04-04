/*
 * midi.c - MIDI (6850 UART) emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * Based on code by
 *  André Fachat <fachat@physik.tu-chemnitz.de>
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

#ifdef HAVE_MIDI
#include <stdio.h>

#include "alarm.h"
#include "archdep.h"
#include "clkguard.h"
#include "cmdline.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "midi.h"
#include "mididrv.h"
#include "snapshot.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "util.h"

#undef DEBUG

/******************************************************************/

/* - Control register */
/* Receive Interrupt Enable */
#define MIDI_CTRL_RIE   0x80
/* Transmit Control */
#define MIDI_CTRL_TC2   0x40
#define MIDI_CTRL_TC1   0x20
#define MIDI_CTRL_TC(x)  (((MIDI_CTRL_TC2 | MIDI_CTRL_TC1) & x) >> 5)
/* Word Select */
#define MIDI_CTRL_WS3   0x10
#define MIDI_CTRL_WS2   0x08
#define MIDI_CTRL_WS1   0x04
#define MIDI_CTRL_WS(x)  (((MIDI_CTRL_WS3 | MIDI_CTRL_WS2 | MIDI_CTRL_WS1) & x) >> 2)
/* Counter Divide Select */
#define MIDI_CTRL_CD2   0x02
#define MIDI_CTRL_CD1   0x01
#define MIDI_CTRL_CD(x)  ((MIDI_CTRL_CD2 | MIDI_CTRL_CD1) & x)
#define MIDI_CTRL_RESET 0x03
/* Defaults after reset */
#define MIDI_CTRL_DEFAULT   (MIDI_CTRL_RESET)

/* - Status register */
/* Interrupt Request */
#define MIDI_STATUS_IRQ  0x80
/* Parity Error */
#define MIDI_STATUS_PE   0x40
/* Receiver Overrun */
#define MIDI_STATUS_OVRN 0x20
/* Framing Error */
#define MIDI_STATUS_FE   0x10
/* Clear to Send */
#define MIDI_STATUS_CTS  0x08
/* Data Carrier Detect */
#define MIDI_STATUS_DCD  0x04
/* Transmit Data Register Empty */
#define MIDI_STATUS_TDRE 0x02
/* Receive Data Register Full */
#define MIDI_STATUS_RDRF 0x01
/* Defaults after reset */
#define MIDI_STATUS_DEFAULT  (MIDI_STATUS_TDRE)

/******************************************************************/

int midi_enabled = 0;

static int fd_in = -1;
static int fd_out = -1;

static alarm_t *midi_alarm = NULL;
static unsigned int midi_int_num;

static int midi_ticks = 0; /* number of clock ticks per char */
static int intx = 0;    /* indicates that a transmit is currently ongoing */
static int irq = 0;
static BYTE ctrl;       /* control register */
static BYTE status;     /* status register */
static BYTE rxdata;     /* data that has been received last */
static BYTE txdata;     /* data prepared to send */
static int alarm_active = 0;    /* if alarm is set or not */

static log_t midi_log = LOG_ERR;

static void int_midi(CLOCK offset, void *data);

static BYTE midi_last_read = 0;  /* the byte read the last time (for RMW) */

/******************************************************************/

static CLOCK midi_alarm_clk = 0;

static int midi_irq = IK_NONE;
static int midi_irq_res;
int midi_mode = 0;

/******************************************************************/

static void midi_set_int(int midiirq, unsigned int int_num, int value)
{
    if (midiirq == IK_IRQ) {
        maincpu_set_irq(int_num, value);
    }
    if (midiirq == IK_NMI) {
        maincpu_set_nmi(int_num, value);
    }
}

static int midi_set_irq(int new_irq_res, void *param)
{
    int new_irq;
    static const int irq_tab[] = { IK_NONE, IK_IRQ, IK_NMI };

    if (new_irq_res < 0 || new_irq_res > 2) {
        return -1;
    }

    new_irq = irq_tab[new_irq_res];

    if (midi_irq != new_irq) {
        midi_set_int(midi_irq, midi_int_num, IK_NONE);
        if (irq) {
            midi_set_int(new_irq, midi_int_num, new_irq);
        }
    }
    midi_irq = new_irq;
    midi_irq_res = new_irq_res;

    return 0;
}

static int get_midi_ticks(void)
{
    return (int)(machine_get_cycles_per_second() / 31250);
}

int midi_set_mode(int new_mode, void *param)
{
    if (new_mode < 0 || new_mode > 4) {
        return -1;
    }

    if (midi_set_irq(midi_interface[new_mode].irq_type, 0)) {
        return -1;
    }

    midi_mode = new_mode;

    midi_ticks = get_midi_ticks();

    return 0;
}

int midi_resources_init(void)
{
    return mididrv_resources_init();
}

void midi_resources_shutdown(void)
{
    mididrv_resources_shutdown();
}

static const cmdline_option_t cmdline_options[] = {
    { "-midi", SET_RESOURCE, 0,
      NULL, NULL, "MIDIEnable", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_MIDI_EMU,
      NULL, NULL },
    { "+midi", SET_RESOURCE, 0,
      NULL, NULL, "MIDIEnable", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_MIDI_EMU,
      NULL, NULL },
    { NULL }
};

int midi_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return mididrv_cmdline_options_init();
}

/******************************************************************/

static void clk_overflow_callback(CLOCK sub, void *var)
{
    if (alarm_active) {
        midi_alarm_clk -= sub;
    }
}

void midi_init(void)
{
    midi_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "MIDI");

    midi_alarm = alarm_new(maincpu_alarm_context, "MIDI", int_midi, NULL);

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    if (midi_log == LOG_ERR) {
        midi_log = log_open("MIDI");
    }
    mididrv_init();
    midi_reset();
}


static void midi_suspend(void)
{
#ifdef DEBUG
    log_message(midi_log, "suspend");
#endif
    status = MIDI_STATUS_DEFAULT;
    intx = 0;

    if (fd_in >= 0) {
        mididrv_in_close();
    }
    fd_in = -1;

    if (fd_out >= 0) {
        mididrv_out_close();
    }
    fd_out = -1;

    alarm_unset(midi_alarm);
    alarm_active = 0;
    intx = 0;

    midi_set_int(midi_irq, midi_int_num, 0);
    irq = 0;
}

void midi_reset(void)
{
#ifdef DEBUG
    log_message(midi_log, "reset");
#endif
    ctrl = MIDI_CTRL_DEFAULT;
    midi_ticks = get_midi_ticks();
    midi_suspend();
}

static void midi_activate(void)
{
#ifdef DEBUG
    log_message(midi_log, "activate");
#endif
    fd_in = mididrv_in_open();
    fd_out = mididrv_out_open();
    if (!intx) {
        midi_alarm_clk = maincpu_clk + 1;
        alarm_set(midi_alarm, midi_alarm_clk);
        alarm_active = 1;
    }
}

void midi_store(WORD a, BYTE b)
{
#ifdef DEBUG
    log_message(midi_log, "store(%x,%02x)", a, b);
#endif
    if (maincpu_rmw_flag) {
        maincpu_clk--;
        maincpu_rmw_flag = 0;
        midi_store(a, midi_last_read);
        maincpu_clk++;
    }

    a &= midi_interface[midi_mode].mask;

    if (a == midi_interface[midi_mode].ctrl_addr) {
#ifdef DEBUG
        log_message(midi_log, "store ctrl: %02x", b);
#endif
        ctrl = b;
        midi_ticks = get_midi_ticks();

        if (MIDI_CTRL_CD(ctrl) == midi_interface[midi_mode].midi_cd) {
            /* TODO check WS */
            midi_activate();
        } else if (MIDI_CTRL_CD(ctrl) == MIDI_CTRL_RESET) {
            midi_reset();
        } else {
            midi_suspend();
        }
    } else if (a == midi_interface[midi_mode].tx_addr) {
        status &= ~MIDI_STATUS_IRQ;
#ifdef DEBUG
        log_message(midi_log, "store tx: %02x", b);
#endif
        if ((status & MIDI_STATUS_TDRE) && !(MIDI_CTRL_CD(ctrl) == MIDI_CTRL_RESET)) {
            status &= ~MIDI_STATUS_TDRE;
            txdata = b;
            if (!intx) {
                midi_alarm_clk = maincpu_clk + 1;
                alarm_set(midi_alarm, midi_alarm_clk);
                alarm_active = 1;
                intx = 2;
            } else {
                if (intx == 1) {
                    intx++;
                }
            }
        }
    }
}

BYTE midi_read(WORD a)
{
#ifdef DEBUG
    log_message(midi_log, "read(%x)", a);
#endif
    midi_last_read = 0xff;
    a &= midi_interface[midi_mode].mask;

    if (a == midi_interface[midi_mode].status_addr) {
#ifdef DEBUG
        log_message(midi_log, "read status: %02x", status);
#endif
        midi_last_read = status;
    } else if (a == midi_interface[midi_mode].rx_addr) {
#ifdef DEBUG
        log_message(midi_log, "read rx: %02x (%02x)", rxdata, status);
#endif
        status &= ~MIDI_STATUS_OVRN;
        if (irq) {
            status &= ~MIDI_STATUS_IRQ;
            midi_set_int(midi_irq, midi_int_num, 0);
            irq = 0;
        }
        if (status & MIDI_STATUS_RDRF) {
            status &= ~MIDI_STATUS_RDRF;
            midi_last_read = rxdata;
        }
    }

    return midi_last_read;
}

BYTE midi_peek(WORD a)
{
    a &= midi_interface[midi_mode].mask;

    /* If the read and write registers are mapped to
       the same addresses, prefer the readable registers */
    if (a == midi_interface[midi_mode].status_addr) {
        return status;
    } else if (a == midi_interface[midi_mode].rx_addr) {
        return rxdata;
    } else if (a == midi_interface[midi_mode].ctrl_addr) {
        return ctrl;
    } else if (a == midi_interface[midi_mode].tx_addr) {
        return txdata;
    }

    return 0;
}

int midi_test_read(WORD a)
{
    a &= midi_interface[midi_mode].mask;

    return (a == midi_interface[midi_mode].status_addr) || (a == midi_interface[midi_mode].rx_addr);
}

int midi_test_peek(WORD a)
{
    a &= midi_interface[midi_mode].mask;

    return midi_test_read(a)
           || (a == midi_interface[midi_mode].ctrl_addr)
           || (a == midi_interface[midi_mode].tx_addr);
}

static void int_midi(CLOCK offset, void *data)
{
    int rxirq = 0;

#if 0 /*def DEBUG */
    log_message(midi_log, "int_midi(offset=%ld, clk=%d", (long int)offset, (int)maincpu_clk);
#endif
    if ((intx == 2) && (fd_out >= 0)) {
        mididrv_out(txdata);
    }

    if (intx) {
        intx--;
    }

    if ((fd_in >= 0) && (!(status & MIDI_STATUS_RDRF)) && (mididrv_in(&rxdata) == 1)) {
        status |= MIDI_STATUS_RDRF;
        rxirq = 1;
#ifdef DEBUG
        log_message(midi_log, "int got %02x", rxdata);
#endif
    }

    if (rxirq && (ctrl & MIDI_CTRL_RIE)) {
        midi_set_int(midi_irq, midi_int_num, 1);
        status |= MIDI_STATUS_IRQ;
        irq = 1;
#ifdef DEBUG
        log_message(midi_log, "int_midi IRQ offset=%ld, clk=%d", (long int)offset, (int)maincpu_clk);
#endif
    }

    if (!(status & MIDI_STATUS_TDRE)) {
        status |= MIDI_STATUS_TDRE;
        /* TODO: TX IRQ */
    }

    midi_alarm_clk = maincpu_clk + midi_ticks;
    alarm_set(midi_alarm, midi_alarm_clk);
    alarm_active = 1;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "MIDI"

/* FIXME: implement snapshot support */
int midi_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int midi_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}
#endif
