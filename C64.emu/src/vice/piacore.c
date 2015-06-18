/*
 * piacore.c -- PIA chip emulation.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#include "monitor.h"
#include "snapshot.h"


static BYTE pia_last_read = 0;

static unsigned int pia_int_num;


void mypia_init(void)
{
    mypia_log = log_open(MYPIA_NAME);

    pia_int_num = interrupt_cpu_status_int_new(mycpu_int_status, MYPIA_NAME);
}

void mypia_reset(void)
{
    /* clear _all_ internal registers */

    mypia.ctrl_a = 0;    /* PIA 1 Port A Control */
    mypia.ctrl_b = 0;    /* PIA 1 Port B Control */
    mypia.ddr_a = 0;     /* PIA 1 Port A DDR */
    mypia.ddr_b = 0;     /* PIA 1 Port B DDR */
    mypia.port_a = 255;  /* PIA 1 Port A input; nothing to read from keyboard */
    mypia.port_b = 255;  /* PIA 1 Port B input; nothing to read from keyboard */

    pia_reset();

    pia_set_ca2(1);
    mypia.ca_state = 1;
    pia_set_cb2(1);
    mypia.cb_state = 1;

    is_peek_access = 0;

    my_set_int(pia_int_num, 0);
}

static void mypia_update_irq(void)
{
    if (0
        || ((mypia.ctrl_a & 0x81) == 0x81)
        || ((mypia.ctrl_a & 0x68) == 0x48)
        || ((mypia.ctrl_b & 0x81) == 0x81)
        || ((mypia.ctrl_b & 0x68) == 0x48)
        ) {
        my_set_int(pia_int_num, 1);
    } else {
        my_set_int(pia_int_num, 0);
    }
}


/* control line flag support. Used for PET IRQ input.
 * this currently relies on each edge being called only once,
 * otherwise multiple IRQs could occur. */

void mypia_signal(int line, int edge)
{
    switch (line) {
        case PIA_SIG_CA1:
            if (((mypia.ctrl_a & 0x02) ? PIA_SIG_RISE : PIA_SIG_FALL) == edge) {
                mypia.ctrl_a |= 0x80;
                mypia_update_irq();
                if (IS_CA2_TOGGLE_MODE()) {
                    pia_set_ca2(1);
                    mypia.ca_state = 1;
                }
            }
        case PIA_SIG_CB1:
            if (((mypia.ctrl_b & 0x02) ? PIA_SIG_RISE : PIA_SIG_FALL) == edge) {
                mypia.ctrl_b |= 0x80;
                mypia_update_irq();
                if (IS_CB2_TOGGLE_MODE()) {
                    pia_set_cb2(1);
                    mypia.cb_state = 1;
                }
            }
            break;
    }
}


/* ------------------------------------------------------------------------- */
/* PIA */

void mypia_store(WORD addr, BYTE byte)
{
    if (mycpu_rmw_flag) {
        myclk--;
        mycpu_rmw_flag = 0;
        mypia_store(addr, pia_last_read);
        myclk++;
    }

    addr &= 3;

    switch (addr) {
        case P_PORT_A: /* port A */
            if (mypia.ctrl_a & 4) {
                mypia.port_a = byte;
            } else {
                mypia.ddr_a = byte;
            }
            byte = mypia.port_a | ~mypia.ddr_a;
            store_pa(byte);
            break;

        case P_PORT_B: /* port B */
            if (mypia.ctrl_b & 4) {
                mypia.port_b = byte;
            } else {
                mypia.ddr_b = byte;
            }
            byte = mypia.port_b | ~mypia.ddr_b;
            store_pb(byte);
            if (IS_CB2_HANDSHAKE()) {
                pia_set_cb2(0);
                mypia.cb_state = 0;
                if (IS_CB2_PULSE_MODE()) {
                    pia_set_cb2(1);
                    mypia.cb_state = 1;
                }
            }
            break;

        /* Control */

        case P_CTRL_A: /* Control A */
            if ((byte & 0x38) == 0x30) { /* set output low */
                pia_set_ca2(0);
                mypia.ca_state = 0;
            } else
            if ((byte & 0x38) == 0x38) { /* set output high */
                pia_set_ca2(1);
                mypia.ca_state = 1;
            } else                      /* change to toggle/pulse */
            if ((mypia.ctrl_a & 0x30) == 0x30) {
                pia_set_ca2(1);
                mypia.ca_state = 1;
            }

            mypia.ctrl_a = (mypia.ctrl_a & 0xc0) | (byte & 0x3f);

            if (mypia.ctrl_a & 0x20) {
                mypia.ctrl_a &= 0xbf;
            }

            mypia_update_irq();

            break;

        case P_CTRL_B: /* Control B */
            if ((byte & 0x38) == 0x30) { /* set output low */
                pia_set_cb2(0);
                mypia.cb_state = 0;
            } else
            if ((byte & 0x38) == 0x38) { /* set output high */
                pia_set_cb2(1);
                mypia.cb_state = 1;
            } else                      /* change to toggle/pulse */
            if ((mypia.ctrl_b & 0x30) == 0x30) {
                pia_set_cb2(1);
                mypia.cb_state = 1;
            }

            mypia.ctrl_b = (mypia.ctrl_b & 0xc0) | (byte & 0x3f);

            if (mypia.ctrl_b & 0x20) {
                mypia.ctrl_b &= 0xbf;
            }

            mypia_update_irq();

            break;
    }  /* switch */
}


/* ------------------------------------------------------------------------- */

BYTE mypia_read(WORD addr)
{
    static BYTE byte = 0xff;

    addr &= 3;

    switch (addr) {
        case P_PORT_A: /* port A */
            if (mypia.ctrl_a & 4) {
                if (!is_peek_access) {
                    mypia.ctrl_a &= 0x3f;       /* Clear CA1,CA2 IRQ */
                    mypia_update_irq();
                }
                /* WARNING: for output pins, this port reads the voltage of
                 * the output pins, not the ORA value as the other port.
                 * Value read might be different from what is expected due
                 * to excessive electrical load on the pin.
                 */
                byte = read_pa();
                pia_last_read = byte;
                return byte;
            }
            pia_last_read = (mypia.ddr_a);
            return pia_last_read;
            break;

        case P_PORT_B: /* port B */
            if (mypia.ctrl_b & 4) {
                if (!is_peek_access) {
                    mypia.ctrl_b &= 0x3f;       /* Clear CB1,CB2 IRQ */
                    mypia_update_irq();
                }

                /* WARNING: this port reads the ORB for output pins, not
                   the voltage on the pins as the other port. */
                byte = read_pb();
                pia_last_read = (byte & ~mypia.ddr_b)
                                | (mypia.port_b & mypia.ddr_b);
                return pia_last_read;
            }
            pia_last_read = (mypia.ddr_b);
            return pia_last_read;
            break;

        /* Control */

        case P_CTRL_A: /* Control A */
            pia_last_read = (mypia.ctrl_a);
            return pia_last_read;
            break;

        case P_CTRL_B: /* Control B */
            pia_last_read = (mypia.ctrl_b);
            return pia_last_read;
            break;
    }  /* switch */

    /* should never happen */
    return (0xFF);
}


BYTE mypia_peek(WORD addr)
{
    BYTE t;

    is_peek_access = 1;
    t = mypia_read(addr);
    is_peek_access = 0;

    return t;
}

int mypia_dump(void)
{
    mon_out("port_a: %02x  port_b: %02x   (written bits only)\n", mypia.port_a, mypia.port_b);
    mon_out(" ddr_a: %02x   ddr_b: %02x   (1 bits are outputs)\n", mypia.ddr_a, mypia.ddr_b);
    mon_out("ctrl_a: %02x  ctrl_b: %02x\n", mypia.ctrl_a, mypia.ctrl_b);
    mon_out("   ca2: %2x     cb2: %2x\n", mypia.ca_state, mypia.cb_state);
    mon_out("CA1 active transition: %d\n", (mypia.ctrl_a & 0x80) >> 7);
    mon_out("CA2 active transition: %d\n", (mypia.ctrl_a & 0x40) >> 6);

    if (mypia.ctrl_a & 0x20) {
        mon_out("CA2: out, ");
        if (mypia.ctrl_a & 0x10) {
            mon_out("manual, ");
            if (mypia.ctrl_a & 0x08) {
                mon_out("high\n");
            } else {
                mon_out("low\n");
            }
        } else {
            mon_out("handshake ");
            if (mypia.ctrl_a & 0x08) {
                mon_out("pulse\n");
            } else {
                mon_out("on read (to 0)/CA1 (to 1)\n");
            }
        }
    } else {
        mon_out("CA2: in, ");
        if (mypia.ctrl_a & 0x10) {
            mon_out("active high, ");
        } else {
            mon_out("active low, ");
        }
        if (mypia.ctrl_a & 0x08) {
            mon_out("IRQ on\n");
        } else {
            mon_out("IRQ off\n");
        }
    }
    if (mypia.ctrl_a & 0x04) {
        mon_out("IORA visible\n");
    } else {
        mon_out("DDRA visible\n");
    }
    if (mypia.ctrl_a & 0x02) {
        mon_out("CA1: active high, ");
    } else {
        mon_out("CA1: active low, ");
    }
    if (mypia.ctrl_a & 0x01) {
        mon_out("IRQ on\n");
    } else {
        mon_out("IRQ off\n");
    }

    mon_out("CB1 active transition: %d\n", (mypia.ctrl_b & 0x80) >> 7);
    mon_out("CB2 active transition: %d\n", (mypia.ctrl_b & 0x40) >> 6);
    if (mypia.ctrl_b & 0x20) {
        mon_out("CB2: out, ");
        if (mypia.ctrl_b & 0x10) {
            mon_out("manual, ");
            if (mypia.ctrl_b & 0x08) {
                mon_out("high\n");
            } else {
                mon_out("low\n");
            }
        } else {
            mon_out("handshake ");
            if (mypia.ctrl_b & 0x08) {
                mon_out("pulse\n");
            } else {
                mon_out("on write (to 0)/CB1 (to 1)\n");
            }
        }
    } else {
        mon_out("CB2: in, ");
        if (mypia.ctrl_b & 0x10) {
            mon_out("active high, ");
        } else {
            mon_out("active low, ");
        }
        if (mypia.ctrl_b & 0x08) {
            mon_out("IRQ on\n");
        } else {
            mon_out("IRQ off\n");
        }
    }
    if (mypia.ctrl_b & 0x04) {
        mon_out("IORB visible\n");
    } else {
        mon_out("DDRB visible\n");
    }
    if (mypia.ctrl_b & 0x02) {
        mon_out("CB1: active high, ");
    } else {
        mon_out("CB1: active low, ");
    }
    if (mypia.ctrl_b & 0x01) {
        mon_out("IRQ on\n");
    } else {
        mon_out("IRQ off\n");
    }

    return 0;
}

/*------------------------------------------------------------------------*/

/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 0/0
 */

#define PIA_DUMP_VER_MAJOR      1
#define PIA_DUMP_VER_MINOR      0

static char snap_module_name[] = MYPIA_NAME;

/*
 * The dump data:
 *
 * UBYTE        ORA
 * UBYTE        DDRA
 * UBYTE        CTRLA
 * UBYTE        ORB
 * UBYTE        DDRB
 * UBYTE        CTRLB
 * UBYTE        CABSTATE        Bit 7 = state of CA2, Bit 6 = state of CB2
 *
 */

/* FIXME!!!  Error check.  */

int mypia_snapshot_write_module(snapshot_t * p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(p, snap_module_name,
                               PIA_DUMP_VER_MAJOR, PIA_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    SMW_B(m, mypia.port_a);
    SMW_B(m, mypia.ddr_a);
    SMW_B(m, mypia.ctrl_a);

    SMW_B(m, mypia.port_b);
    SMW_B(m, mypia.ddr_b);
    SMW_B(m, mypia.ctrl_b);

    SMW_B(m, (BYTE)((mypia.ca_state ? 0x80 : 0) | (mypia.cb_state ? 0x40 : 0)));

    snapshot_module_close(m);

    return 0;
}

int mypia_snapshot_read_module(snapshot_t * p)
{
    BYTE vmajor, vminor;
    BYTE byte;
    snapshot_module_t *m;

    my_restore_int(pia_int_num, 0);          /* just in case */

    m = snapshot_module_open(p, snap_module_name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != PIA_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    SMR_B(m, &mypia.port_a);
    SMR_B(m, &mypia.ddr_a);
    SMR_B(m, &mypia.ctrl_a);

    SMR_B(m, &mypia.port_b);
    SMR_B(m, &mypia.ddr_b);
    SMR_B(m, &mypia.ctrl_b);

    SMR_B(m, &byte);
    mypia.ca_state = (byte & 0x80) ? 1 : 0;
    mypia.cb_state = (byte & 0x80) ? 1 : 0;

    pia_set_ca2(mypia.ca_state);
    pia_set_cb2(mypia.cb_state);

    byte = mypia.port_a | ~mypia.ddr_a;
    undump_pa(byte);

    byte = mypia.port_b | ~mypia.ddr_b;
    undump_pb(byte);

    if (0
        || ((mypia.ctrl_a & 0x81) == 0x81)
        || ((mypia.ctrl_a & 0x68) == 0x48)
        || ((mypia.ctrl_b & 0x81) == 0x81)
        || ((mypia.ctrl_b & 0x68) == 0x48)
        ) {
        my_restore_int(pia_int_num, 1);
    }

    return snapshot_module_close(m);
}
