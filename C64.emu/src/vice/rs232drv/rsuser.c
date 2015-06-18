/*
 * rsuser.c - Daniel Dallmann's 9600 baud RS232 userport interface.
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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
/*
 * This is a very crude emulation. It does not check for a lot of things.
 * It simply tries to work with existing programs that work on the real
 * machine and does not try to catch rogue attempts...
 */

/* FIXME: Should keep its own log.  */

#include "vice.h"

#include <stdio.h>

#include "alarm.h"
#include "clkguard.h"
#include "cmdline.h"
#include "log.h"
#include "maincpu.h"
#include "resources.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "translate.h"
#include "types.h"

static int fd = -1;

static alarm_t *rsuser_alarm = NULL;

static int dtr;
static int rts;

static int rxstate;
static BYTE rxdata;
static BYTE txdata;
static BYTE txbit;

static long cycles_per_sec = 1000000;

static CLOCK clk_start_rx = 0;
static CLOCK clk_start_tx = 0;
static CLOCK clk_start_bit = 0;
static CLOCK clk_end_tx = 0;

static void (*start_bit_trigger)(void);
static void (*byte_rx_func)(BYTE);

static void clk_overflow_callback(CLOCK sub, void *data);

static void int_rsuser(CLOCK offset, void *data);

#undef DEBUG

#define RSUSER_TICKS    21111

#undef DEBUG_TIMING_SEND
#undef DEBUG_TIMING_RECV


#ifdef DEBUG
# define LOG_DEBUG(_xxx) log_debug _xxx
#else
# define LOG_DEBUG(_xxx)
#endif

#ifdef DEBUG_TIMING_SEND
# define LOG_DEBUG_TIMING_TX(_xxx) log_debug _xxx
#else
# define LOG_DEBUG_TIMING_TX(_xxx)
#endif

#ifdef DEBUG_TIMING_RECV
# define LOG_DEBUG_TIMING_RX(_xxx) log_debug _xxx
#else
# define LOG_DEBUG_TIMING_RX(_xxx)
#endif

/***********************************************************************
 * resource handling
 */

int rsuser_enabled = 0;                 /* enable flag */
static int rsuser_baudrate = 300;       /* saves the baud rate given */
static int char_clk_ticks = 0;          /* clk ticks per character */
static int bit_clk_ticks = 0;           /* clk ticks per character */

static int rsuser_device;

static void calculate_baudrate(void)
{
    if (rsuser_enabled) {
        char_clk_ticks = (int)(10.0 * cycles_per_sec / ((double)rsuser_baudrate));
    } else {
        char_clk_ticks = RSUSER_TICKS;
    }
    bit_clk_ticks = (int)((double)(char_clk_ticks) / 10.0);

    LOG_DEBUG(("RS232: %d cycles per char (cycles_per_sec=%ld).",
               char_clk_ticks, cycles_per_sec));
}

static int set_enable(int value, void *param)
{
    int newval = value ? 1 : 0;

    if (newval && !rsuser_enabled) {
        dtr = DTR_OUT;  /* inactive */
        rts = RTS_OUT;  /* inactive */
        fd = -1;
    }
    if (rsuser_enabled && !newval) {
        if (fd != -1) {
            /* if (clk_start_tx) rs232drv_putc(fd, rxdata); */
            rs232drv_close(fd);
        }
        if (rsuser_alarm) {
            alarm_unset(rsuser_alarm);
        }
        fd = -1;
    }

    rsuser_enabled = newval;

    calculate_baudrate();

    return 0;
}

static int set_baudrate(int val, void *param)
{
    if (val < 1) {
        return -1;
    }

    rsuser_baudrate = val;

    calculate_baudrate();

    return 0;
}

static int set_up_device(int val, void *param)
{
    if (val < 0 || val > 3) {
        return -1;
    }

    rsuser_device = val;

    if (fd != -1) {
        rs232drv_close(fd);
        fd = rs232drv_open(rsuser_device);
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "RsUserEnable", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &rsuser_enabled, set_enable, NULL },
    { "RsUserBaud", 300, RES_EVENT_NO, NULL,
      &rsuser_baudrate, set_baudrate, NULL },
    { "RsUserDev", 0, RES_EVENT_NO, NULL,
      &rsuser_device, set_up_device, NULL },
    { NULL }
};

int rsuser_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] = {
    { "-rsuser", SET_RESOURCE, 0,
      NULL, NULL, "RsUserEnable", (void *)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_RS232_USERPORT,
      NULL, NULL },
    { "+rsuser", SET_RESOURCE, 0,
      NULL, NULL, "RsUserEnable", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_RS232_USERPORT,
      NULL, NULL },
    { "-rsuserbaud", SET_RESOURCE, 1,
      NULL, NULL, "RsUserBaud", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_BAUD, IDCLS_SET_BAUD_RS232_USERPORT,
      NULL, NULL },
    { "-rsuserdev", SET_RESOURCE, 1,
      NULL, NULL, "RsUserDev", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SPECIFY_RS232_DEVICE_USERPORT,
      "<0-3>", NULL },
    { NULL }
};

int rsuser_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/*********************************************************************/

static unsigned char code[256];
static unsigned int buf;
static unsigned int valid;
static const unsigned int masks[] =
{
    1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800,
    0x1000, 0x2000, 0x4000, 0x8000
};

void rsuser_init(long cycles, void (*startfunc)(void), void (*bytefunc)(BYTE))
{
    int i, j;
    unsigned char c, d;

    rsuser_alarm = alarm_new(maincpu_alarm_context, "RSUser", int_rsuser, NULL);

    clk_guard_add_callback(maincpu_clk_guard, clk_overflow_callback, NULL);

    cycles_per_sec = cycles;
    calculate_baudrate();

    start_bit_trigger = startfunc;
    byte_rx_func = bytefunc;

    for (i = 0; i < 256; i++) {
        c = i; d = 0;
        for (j = 0; j < 8; j++) {
            d <<= 1;
            if (c & 1) {
                d |= 1;
            }
            c >>= 1;
        }
        code[i] = d;
    }

    dtr = DTR_OUT;      /* inactive */
    rts = RTS_OUT;      /* inactive */
    fd = -1;

    buf = (unsigned int)(~0); /* all 1s */
    valid = 0;
}

void rsuser_reset(void)
{
    rxstate = 0;
    clk_start_rx = 0;
    clk_start_tx = 0;
    clk_start_bit = 0;
    clk_end_tx = 0;
    if (fd != -1) {
        rs232drv_close(fd);
    }

    alarm_unset(rsuser_alarm);
    fd = -1;
}

static void rsuser_setup(void)
{
    rxstate = 0;
    clk_start_rx = 0;
    clk_start_tx = 0;
    clk_start_bit = 0;
    if (fd < 0) {
        fd = rs232drv_open(rsuser_device);
    }
    alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks / 8);
}

void rsuser_write_ctrl(BYTE b)
{
    int new_dtr = b & DTR_OUT;  /* = 0 is active, != 0 is inactive */
    int new_rts = b & RTS_OUT;  /* = 0 is active, != 0 is inactive */

    if (rsuser_enabled) {
        if (dtr && !new_dtr) {
            rsuser_setup();
        }
        if (new_dtr && !dtr && fd != -1) {
#if 0   /* This is a bug in the X-line handshake of the C64... */
            LOG_DEBUG(("switch rs232 off."));
            alarm_unset(rsuser_alarm);
            rs232drv_close(fd);
            fd = -1;
#endif
        }
    }

    dtr = new_dtr;
    rts = new_rts;
}

static void check_tx_buffer(void)
{
    BYTE c;

    while (valid >= 10 && (buf & masks[valid - 1])) {
        valid--;
    }

    if (valid >= 10) {     /* (valid-1)-th bit is not set = start bit! */
        if (!(buf & masks[valid - 10])) {
            log_error(LOG_DEFAULT, "Frame error!");
        } else {
            c = (buf >> (valid - 9)) & 0xff;
            if (fd != -1) {
                LOG_DEBUG(("\"%c\" (%02x).", code[c], code[c]));
                rs232drv_putc(fd, ((BYTE)(code[c])));
            }
        }
        valid -= 10;
    }
}

static void keepup_tx_buffer(void)
{
    if ((!clk_start_bit) || maincpu_clk < clk_start_bit) {
        return;
    }

    while (clk_start_bit < clk_end_tx) {
        LOG_DEBUG(("keepup: clk=%d, _bit=%d (%d), _tx=%d.",
                   maincpu_clk, clk_start_bit - clk_start_tx, clk_start_bit,
                   clk_start_tx));
        buf = buf << 1;
        if (txbit) {
            buf |= 1;
        }
        valid++;
        if (valid >= 10) {
            check_tx_buffer();
        }

        clk_start_bit += bit_clk_ticks;

        if (clk_start_bit >= maincpu_clk) {
            break;
        }
    }
    if (clk_start_bit >= clk_end_tx) {
        clk_start_tx = 0;
        clk_start_bit = 0;
        clk_end_tx = 0;
    }
}

void rsuser_set_tx_bit(int b)
{
    LOG_DEBUG(("rsuser_set_tx(clk=%d, clk_start_tx=%d, b=%d).",
               maincpu_clk, clk_start_tx, b));

    LOG_DEBUG_TIMING_TX(("rsuser_set_tx(clk=%d, clk_start_tx=%d, b=%d).",
                         maincpu_clk, clk_start_tx, b));

    if (fd == -1 || rsuser_baudrate > 2400) {
        clk_start_tx = 0;
        return;
    }

    /* feeds the output buffer with enough bits till clk */
    keepup_tx_buffer();
    txbit = b;

    if (!clk_start_tx && !b) {
        /* the clock where we start sampling - in the middle of the bit */
        clk_start_tx = maincpu_clk + (bit_clk_ticks / 2);
        clk_start_bit = clk_start_tx;
        /* note: bit_clk_ticks * 10 may not be the same as char_clk_ticks
         *       because of integer division.  So always compute it
         */
        clk_end_tx = clk_start_tx + (bit_clk_ticks * 10);
        txdata = 0;
    }
}

BYTE rsuser_get_rx_bit(void)
{
    int bit = 0, byte = 1;
    LOG_DEBUG_TIMING_RX(("rsuser_get_rx_bit(clk=%d, clk_start_rx=%d).",
                         maincpu_clk, clk_start_rx));

    if (clk_start_rx) {
        byte = 0;
        bit = (maincpu_clk - clk_start_rx) / (bit_clk_ticks);
        LOG_DEBUG(("read ctrl(_rx=%d, clk-start_rx=%d -> bit=%d)",
                   clk_start_rx, maincpu_clk - clk_start_rx, bit));
        if (!bit) {
            byte = 0;   /* start bit */
        } else
        if (bit < 9) {    /* 8 data bits */
            byte = rxdata & (1 << (bit - 1));
            if (byte) {
                byte = 1;
            }
        } else {        /* stop bits */
            byte = 1;
        }
    }
    return byte;
}

BYTE rsuser_read_ctrl(void)
{
    return rsuser_get_rx_bit() | CTS_IN | (rsuser_baudrate > 2400 ? 0 : DCD_IN);
}

void rsuser_tx_byte(BYTE b)
{
    buf = (buf << 8) | b;
    valid += 8;

    check_tx_buffer();
}

static void int_rsuser(CLOCK offset, void *data)
{
    CLOCK rclk = maincpu_clk - offset;

    keepup_tx_buffer();

    switch (rxstate) {
        case 0:
            if (fd != -1 && rs232drv_getc(fd, &rxdata)) {
                /* byte received, signal startbit on flag */
                rxstate++;
                if (start_bit_trigger) {
                    start_bit_trigger();
                }
                clk_start_rx = rclk;
            }
            alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks);
            break;
        case 1:
            /* now byte should be in shift register */
            if (byte_rx_func) {
                byte_rx_func((BYTE)(code[rxdata]));
            }
            rxstate = 0;
            clk_start_rx = 0;
            alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks / 8);
            break;
    }
}

static void clk_overflow_callback(CLOCK sub, void *data)
{
    if (clk_start_tx) {
        clk_start_tx -= sub;
    }
    if (clk_start_rx) {
        clk_start_rx -= sub;
    }
    if (clk_start_bit) {
        clk_start_bit -= sub;
    }
}
