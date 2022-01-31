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
#include "cmdline.h"
#include "joyport.h"
#include "log.h"
#include "maincpu.h"
#include "resources.h"
#include "rs232.h"
#include "rs232drv.h"
#include "rsuser.h"
#include "types.h"
#include "userport.h"

static int fd = -1;

static alarm_t *rsuser_alarm = NULL;

static int rxstate;
static uint8_t rxdata;
static uint8_t txdata;
static uint8_t txbit;

static long cycles_per_sec = 1000000;

static CLOCK clk_start_rx = 0;
static CLOCK clk_start_tx = 0;
static CLOCK clk_start_bit = 0;
static CLOCK clk_end_tx = 0;

static void (*start_bit_trigger)(void);
static void (*byte_rx_func)(uint8_t);

static void int_rsuser(CLOCK offset, void *data);

#undef DEBUG

/* #define LOG_MODEM_STATUS */


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
 * control lines
 */

static int dtr = 0;
static int rts = 0;

/***********************************************************************
 * UP9600 detection variables
 */
static int old_dsr;
static unsigned int dsr_cnt;

/***********************************************************************
 * userport interface handling
 */

/* some prototypes are needed */
static void rsuser_tx_byte(uint8_t b);
static void rsuser_write_ctrl(uint8_t b, int pulse);
static uint8_t rsuser_read_ctrl(uint8_t orig);
static void rsuser_set_tx_bit(uint8_t b);
static int userport_rs232_enable(int value);
static void rsuser_reset(void);

static userport_device_t rs232_device = {
    "Userport RS232/Modem",     /* device name */
    JOYSTICK_ADAPTER_ID_NONE,   /* NOT a joystick adapter */
    USERPORT_DEVICE_TYPE_MODEM, /* device is a modem */
    userport_rs232_enable,      /* enable function */
    rsuser_read_ctrl,           /* read pb0-pb7 function */
    rsuser_write_ctrl,          /* store pb0-pb7 function */
    NULL,                       /* NO read pa2 pin function */
    rsuser_set_tx_bit,          /* store pa2 pin function */
    NULL,                       /* NO read pa3 pin function */
    NULL,                       /* NO store pa3 pin function */
    0,                          /* pc pin is NOT needed */
    rsuser_tx_byte,             /* store sp1 pin function */
    NULL,                       /* NO read sp1 pin function */
    NULL,                       /* NO store sp2 pin function */
    NULL,                       /* NO read sp2 pin function */
    rsuser_reset,               /* NO reset function */
    NULL,                       /* NO power toggle function */
    NULL,                       /* NO snapshot write function */
    NULL                        /* NO snapshot read function */
};

/***********************************************************************
 * resource handling
 */

static int rsuser_up9600 = 0;           /* UP9600 enable flag */
static int rsuser_enabled = 0;          /* enable flag */
static int rsuser_rtsinv = 0;           /* RTS invert flag - 0 = Normal Kernal level, 1 = Inverted level */
static int rsuser_ctsinv = 0;           /* CTS invert flag - 0 = Normal Kernal level, 1 = Inverted level */
static int rsuser_dsrinv = 0;           /* DSR invert flag - 0 = Normal Kernal level, 1 = Inverted level */
static int rsuser_dcdinv = 0;           /* DCD invert flag - 0 = Normal Kernal level, 1 = Inverted level */
static int rsuser_dtrinv = 0;           /* DTR invert flag - 0 = Normal Kernal level, 1 = Inverted level */
static int rsuser_baudrate = 300;       /* saves the baud rate given */
static int char_clk_ticks = 0;          /* clk ticks per character */
static int bit_clk_ticks = 0;           /* clk ticks per bit */

static int rsuser_device;

static void calculate_baudrate(void)
{
    if (rsuser_enabled) {
        char_clk_ticks = (int)(10.0 * cycles_per_sec / ((double)rsuser_baudrate));
    } else {
        char_clk_ticks = RSUSER_TICKS;
    }
    bit_clk_ticks = (int)((double)(char_clk_ticks) / 10.0);

    LOG_DEBUG(("RS232 calculate_baudrate: %d cycles per char (cycles_per_sec=%ld).",
               char_clk_ticks, cycles_per_sec));
}

static int userport_rs232_enable(int value)
{
    int newval = value ? 1 : 0;

    if (newval && !rsuser_enabled) {
        dtr = rsuser_dtrinv ? DTR_OUT : 0;  /* inactive */
        rts = rsuser_rtsinv ? RTS_OUT : 0;  /* inactive */
        fd = -1;
    }
    if (rsuser_enabled && !newval) {
        if (fd >= 0) {
            /* if (clk_start_tx) rs232drv_putc(fd, rxdata); */
            rs232drv_close(fd);
        }
        if (rsuser_alarm) {
            alarm_unset(rsuser_alarm);
        }
        fd = -1;
    }

    rsuser_enabled = newval;
    
    LOG_DEBUG(("RS232 set_enable: enabled:%d fd:%d dtr:%d rts:%d",
        rsuser_enabled, fd, dtr, rts
    ));

    calculate_baudrate();

    return 0;
}

static int set_up9600(int val, void *param)
{
    if (val >= 0){
        rsuser_up9600 = val;
    } else {
        rsuser_up9600 = 0;
    }

    if (rsuser_up9600 == true){
        old_dsr = 0;
        dsr_cnt = 0;
    }

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

    if (fd >= 0) {
        rs232drv_close(fd);
        fd = rs232drv_open(rsuser_device);
    }
    return 0;
}

static int set_rtsinv(int value, void *param)
{
    rsuser_rtsinv = value ? 1 : 0;
    /* Prevent the state machine to be left in a locked state if the software leaves RTS */
    /* in a fixed state, triggering the RTS disabled state on initialization.            */
    if (rxstate == 2){
        rxstate = 0;
    }
    return 0;
}

static int set_ctsinv(int value, void *param)
{
    rsuser_ctsinv = value ? 1 : 0;
    return 0;
}

static int set_dsrinv(int value, void *param)
{
    rsuser_dsrinv = value ? 1 : 0;
    return 0;
}

static int set_dcdinv(int value, void *param)
{
    rsuser_dcdinv = value ? 1 : 0;
    return 0;
}

static int set_dtrinv(int value, void *param)
{
    rsuser_dtrinv = value ? 1 : 0;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "RsUserUP9600", 0, RES_EVENT_NO, NULL,
      &rsuser_up9600, set_up9600, NULL},
    { "RsUserBaud", 2400, RES_EVENT_NO, NULL,
      &rsuser_baudrate, set_baudrate, NULL },
    { "RsUserDev", 0, RES_EVENT_NO, NULL,
      &rsuser_device, set_up_device, NULL },
    { "RsUserRTSInv", 0, RES_EVENT_NO, (resource_value_t)0,
      &rsuser_rtsinv, set_rtsinv, NULL },
    { "RsUserCTSInv", 0, RES_EVENT_NO, (resource_value_t)0,
      &rsuser_ctsinv, set_ctsinv, NULL },
    { "RsUserDSRInv", 0, RES_EVENT_NO, (resource_value_t)0,
      &rsuser_dsrinv, set_dsrinv, NULL },
    { "RsUserDCDInv", 0, RES_EVENT_NO, (resource_value_t)0,
      &rsuser_dcdinv, set_dcdinv, NULL },
    { "RsUserDTRInv", 0, RES_EVENT_NO, (resource_value_t)0,
      &rsuser_dtrinv, set_dtrinv, NULL },
    RESOURCE_INT_LIST_END
};

int rsuser_resources_init(void)
{
    if (userport_device_register(USERPORT_DEVICE_RS232_MODEM, &rs232_device) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-rsuserup9600", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserUP9600", (void *)1,
      NULL, "Enable UP9600 interface emulation."},
    { "+rsuserup9600", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserUP9600", (void *)0,
      NULL, "Disable UP9600 interface emulation."},
    { "-rsuserbaud", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RsUserBaud", NULL,
      "<baud>", "Set the baud rate of the RS232 userport emulation." },
    { "-rsuserdev", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RsUserDev", NULL,
      "<0-3>", "Specify VICE RS232 device for userport" },
    { "-rsuserrtsinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserRTSInv", (void *)1,
      NULL, "Invert RS232 userport emulation RTS line" },
    { "+rsuserrtsinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserRTSInv", (void *)0,
      NULL, "Do not invert RS232 userport emulation RTS line" },
    { "-rsuserctsinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserCTSInv", (void *)1,
      NULL, "Invert RS232 userport emulation CTS line" },
    { "+rsuserctsinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserCTSInv", (void *)0,
      NULL, "Do not invert RS232 userport emulation CTS line" },
    { "-rsuserdsrinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDSRInv", (void *)1,
      NULL, "Invert RS232 userport emulation DSR line" },
    { "+rsuserdsrinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDSRInv", (void *)0,
      NULL, "Do not invert RS232 userport emulation DSR line" },
    { "-rsuserdcdinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDCDInv", (void *)1,
      NULL, "Invert RS232 userport emulation DCD line" },
    { "+rsuserdcdinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDCDInv", (void *)0,
      NULL, "Do not invert RS232 userport emulation DCD line" },
    { "-rsuserdtrinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDTRInv", (void *)1,
      NULL, "Invert RS232 userport emulation DTR line" },
    { "+rsuserdtrinv", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RsUserDTRInv", (void *)0,
      NULL, "Do not invert RS232 userport emulation DTR line" },
    CMDLINE_LIST_END
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

void rsuser_change_timing(long cycles)
{
    cycles_per_sec = cycles;
    calculate_baudrate();
}

void rsuser_init(long cycles, void (*startfunc)(void), void (*bytefunc)(uint8_t))
{
    int i, j;
    unsigned char c, d;

    rsuser_alarm = alarm_new(maincpu_alarm_context, "RSUser", int_rsuser, NULL);

    rsuser_change_timing(cycles);

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

    dtr = rsuser_dtrinv ? DTR_OUT : 0;      /* inactive */
    rts = rsuser_dtrinv ? RTS_OUT : 0;      /* inactive */
    fd = -1;



    LOG_DEBUG(("rsuser_init: fd:%d dtr:%d rts:%d", fd, dtr, rts));
    
    buf = (unsigned int)(~0); /* all 1s */
    valid = 0;
}

static void rsuser_reset(void)
{
    rxstate = 0;
    clk_start_rx = 0;
    clk_start_tx = 0;
    clk_start_bit = 0;
    clk_end_tx = 0;
    dsr_cnt = 0;

    if (fd >= 0) {
        rs232drv_close(fd);
        fd = -1;
    }

    alarm_unset(rsuser_alarm);
}

static void rsuser_setup(void)
{
    rxstate = 0;
    clk_start_rx = 0;
    clk_start_tx = 0;
    clk_start_bit = 0;
    dsr_cnt = 0;
    if (fd < 0 && rsuser_enabled) {
        fd = rs232drv_open(rsuser_device);
    }
    alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks / 10);
}

/* called by VIA/CIA when cpu writes to user port */
static void rsuser_write_ctrl(uint8_t status, int pulse)
{
    enum rs232handshake_out modem_status = 0;
    int new_dtr = status & DTR_OUT;  /* != 0 is active, = 0 is inactive for Kernal */
    int new_rts = status & RTS_OUT;  /* != 0 is active, = 0 is inactive for Kernal */
    int new_dsr = status & DSR_IN;   /* Written to by UP9600 install routine */
#ifdef LOG_MODEM_STATUS
    static int oldstatus = -1;
#endif

#ifdef LOG_MODEM_STATUS
    if (status != oldstatus) {
        printf("rsuser_write_ctrl(fd:%d) 1: status:%02x dtr:%d rts:%d\n", 
               fd, status, dtr ? 1 : 0, rts ? 1 : 0);
    }
#endif
    
    if (rsuser_enabled) {
        if (fd < 0) {
            if (new_dtr == (rsuser_dtrinv ? 0: DTR_OUT)) {
                rsuser_setup();
            }
        }
        if (fd >= 0) {
            modem_status |= (new_dtr == (rsuser_dtrinv ? 0 : DTR_OUT)) ? RS232_HSO_DTR : 0;
            rs232drv_set_status(fd, modem_status);
        }

#if 0
        if (dtr && !new_dtr) {
            /* DTR low->high transition, set up userport, set DTR active */
            rsuser_setup();
            if (fd != -1) {
                /* all handshake lines are inverted by the RS232 interface */
                modem_status |= dtr ? 0 : RS232_HSO_DTR;
                rs232drv_set_status(fd, modem_status);
            }
        }
#endif
        if (rts != new_rts && fd >= 0) {
            /* RTS handling */
            if ((rsuser_rtsinv ? 0 : RTS_OUT) != new_rts) {
                rxstate = 2;
            } else {
                rxstate = 0;
            }
        }
#if 0
        if (new_dtr && !dtr && fd != -1) {
#if 0   /* This is a bug in the X-line handshake of the C64... */
            LOG_DEBUG(("switch rs232 off."));
            alarm_unset(rsuser_alarm);
            rs232drv_close(fd);
            fd = -1;
#endif
            /* all handshake lines are inverted by the RS232 interface */
            modem_status |= dtr ? 0 : RS232_HSO_DTR;
            rs232drv_set_status(fd, modem_status);
        }
#endif
        if (rsuser_up9600){
            if (!old_dsr && new_dsr) {
                /* DSR low-high transition, UP9600 install routine pulsing PB7*/
                dsr_cnt++;
                if (dsr_cnt == 8){
                    /* 8 PB7 pulses: Write a dummy byte to the shift register input  */
                    /* this will set the SDR IRQ flag and UP9600 detection will pass */
                    byte_rx_func(0x00);
                    dsr_cnt = 0;
                }
            } /*else if (old_dsr == new_dsr) { */
              /* Reset count if two consecutive writes have the same DSR state
                 ---- This isn't working, as rsuser_write_ctrl is being called twice per write */
              /* dsr_cnt = 0; */
            /* } */
        }
    }

    dtr = new_dtr;
    rts = new_rts;
    old_dsr = new_dsr;

#ifdef LOG_MODEM_STATUS
    if (status != oldstatus) {
        printf("rsuser_write_ctrl(fd:%d) 2: status:%02x dtr:%d rts:%d modem_status:%02x cts:%d dsr:%d dcd:%d ri:%d\n", 
               fd, status, dtr ? 1 : 0, rts ? 1 : 0, modem_status, 
               modem_status & RS232_HSI_CTS ? 1 : 0,
               modem_status & RS232_HSI_DSR ? 1 : 0,
               modem_status & RS232_HSI_DCD ? 1 : 0,
               modem_status & RS232_HSI_RI ? 1 : 0
              );
        oldstatus = status;
    }
#endif
}

static void check_tx_buffer(void)
{
    uint8_t c;

    while (valid >= 10 && (buf & masks[valid - 1])) {
        valid--;
    }

    if (valid >= 10) {     /* (valid-1)-th bit is not set = start bit! */
        if (!(buf & masks[valid - 10])) {
            log_error(LOG_DEFAULT, "rsuser: framing mismatch - outgoing baudrates ok?");
        } else {
            c = (buf >> (valid - 9)) & 0xff;
            if (fd >= 0) {
                LOG_DEBUG(("\"%c\" (%02x).", code[c], code[c]));
                rs232drv_putc(fd, ((uint8_t)(code[c])));
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

static void rsuser_set_tx_bit(uint8_t b)
{
    LOG_DEBUG(("rsuser_set_tx(clk=%d, clk_start_tx=%d, b=%d).",
               maincpu_clk, clk_start_tx, b));

    LOG_DEBUG_TIMING_TX(("rsuser_set_tx(clk=%d, clk_start_tx=%d, b=%d).",
                         maincpu_clk, clk_start_tx, b));

    if (fd < 0 || rsuser_up9600) {
        clk_start_tx = 0;
        return;
    }

    /* feeds the output buffer with enough bits till clk */
    keepup_tx_buffer();
    txbit = b << 2;

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

static uint8_t rsuser_get_rx_bit(void)
{
    CLOCK bit = 0;
    int byte = 1;
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

/* called by VIA/CIA when cpu reads from user port */
static uint8_t rsuser_read_ctrl(uint8_t orig)
{
    uint8_t b = orig;
    enum rs232handshake_in modem_status = rs232drv_get_status(fd);
    uint8_t status = 0;
#ifdef LOG_MODEM_STATUS
    static int oldstatus = -1;
#endif    
#if 0
    if (status != oldstatus) {
        printf("rsuser_read_ctrl(fd:%d) 1: mask:%02x modem_status:%02x cts:%d dsr:%d dcd:%d ri:%d\n", 
               fd, b, modem_status, modem_status & RS232_HSI_CTS ? 1 : 0,
               modem_status & RS232_HSI_DSR ? 1 : 0,
               modem_status & RS232_HSI_DCD ? 1 : 0,
               modem_status & RS232_HSI_RI ? 1 : 0
              );
    }
#endif
    if ((rsuser_ctsinv ? 0 : RS232_HSI_CTS) == (modem_status & RS232_HSI_CTS)) {
        status |= CTS_IN;
    }
    if ((rsuser_dcdinv ? 0 : RS232_HSI_DCD) == (modem_status & RS232_HSI_DCD)) {
        if (!rsuser_up9600) {
            status |= DCD_IN;
        }
    }
    /* DSR line is not connected in the UP9600 interface */
    if (((rsuser_dsrinv ? 0 : RS232_HSI_DSR) == (modem_status & RS232_HSI_DSR)) && !rsuser_up9600) {
        status |= DSR_IN;
    }
#ifdef LOG_MODEM_STATUS
    if (status != oldstatus) {
        printf("rsuser_read_ctrl(fd:%d): mask:%02x status:%02x cts:%d dsr:%d dcd:%d ri:%d\n", 
               fd, b, status, modem_status & RS232_HSI_CTS ? 1 : 0,
               modem_status & RS232_HSI_DSR ? 1 : 0,
               modem_status & RS232_HSI_DCD ? 1 : 0,
               modem_status & RS232_HSI_RI ? 1 : 0
              );
        oldstatus = status;
    }
#endif
    /* Reset UP9600 detection counter */
    dsr_cnt = 0;

    return b & (rsuser_get_rx_bit() | status);
}

static void rsuser_tx_byte(uint8_t b)
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
            if ((rsuser_rtsinv ? 0 : RTS_OUT) == rts && fd >= 0 && rs232drv_getc(fd, &rxdata)) {
                /* byte received, signal startbit on flag */
                rxstate++;
                if (start_bit_trigger) {
                    start_bit_trigger();
                }
                clk_start_rx = rclk;
            }
            alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks - bit_clk_ticks); /* Wait one char - 1 stop bit */
            break;
        case 1:
            /* now byte should be in shift register */
            if (byte_rx_func && rsuser_up9600) {
                byte_rx_func((uint8_t)(code[rxdata]));
            }
            rxstate = 0;
            clk_start_rx = 0;
            alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks / 10);
            break;
        case 2:
            /* RTS disabled */
            alarm_set(rsuser_alarm, maincpu_clk + char_clk_ticks / 10);
            break;
    }
}
