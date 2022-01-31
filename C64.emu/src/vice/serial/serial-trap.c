/*
 * serial-trap.c
 *
 * Written by
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

/* #define DEBUG_SERIAL */

#include "vice.h"

#include <stdio.h> /* for NULL */

#include "iecbus.h"
#include "maincpu.h"
#include "mem.h"
#include "serial-iec-bus.h"
/* Will be removed once serial.c is clean */
#include "serial-iec-device.h"
#include "serial-trap.h"
#include "serial.h"
#include "types.h"

#ifdef DEBUG_SERIAL
#include "log.h"
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#define MAX_DEVICES 15  /* FIXME: is there another constant we can use instead? */
extern int traps_enabled_device[MAX_DEVICES]; /* FIXME */

/* Warning: these are only valid for the VIC20, C64 and C128, but *not* for
   the PET.  (FIXME?)  */
#define BSOUR 0x95 /* Buffered Character for IEEE Bus */

/* FIXME: code here assumes 4 bits for device number; should be 5? */
#define DEVNR_MASK      0x0F    /* should be 0x1F */
#define SA_MASK         0x0F

/* bits 7=0 6,5 dev=4,3,2,1,0 */
#define LISTEN          0x20
#define TALK            0x40
#define SECONDARY       0x60

/* bits 7=1 6,5,4 sec=3,2,1,0 */
#define CLOSE           0xE0
#define OPEN            0xF0

/* bits 7=0 6,5 dev=4,3,2,1,0=31 */
#define UNLISTEN        0x3f    /* LISTEN with dev=31 correct?*/
#define UNTALK          0x5f    /* TALK with dev=31 correct?*/

/* Address of serial TMP register.  */
static uint16_t tmp_in;

/* On which channel did listen happen to?  */
static uint8_t TrapDevice;
static uint8_t TrapSecondary;

static int ActiveDevice = -1;

/* Function to call when EOF happens in `serialreceivebyte()'.  */
static void (*eof_callback_func)(void);

/* Function to call when the `serialattention()' trap is called.  */
static void (*attention_callback_func)(void);

static unsigned int serial_truedrive[IECBUS_NUM];

#define IS_PRINTER(d)   (((d) & DEVNR_MASK) >= 4 && ((d) & DEVNR_MASK) <= 7)

static void serial_set_st(uint8_t st)
{
    mem_store((uint16_t)0x90, (uint8_t)(mem_read((uint16_t)0x90) | st));
}

static uint8_t serial_get_st(void)
{
    return mem_read((uint16_t)0x90);
}

/*
 * Send LISTEN/TALK and the secondary address.
 */
static void send_listen_talk_secondary(uint8_t b)
{
    TrapSecondary = b;
    switch (TrapDevice & 0xf0) {
        case LISTEN:
            serial_iec_bus_listen(TrapDevice, TrapSecondary, serial_set_st);
            break;
        case TALK:
            serial_iec_bus_talk(TrapDevice, TrapSecondary, serial_set_st);
            break;
    }
}

/* FIXME: the printers are strange */
static int device_uses_serial_traps(int device)
{
    if ((device < 4) || (device > 11) || /* only devices 4 to 11 can use traps */
        !traps_enabled_device[device] ||
        (serial_truedrive[device] && !IS_PRINTER(device))) {
        return 0;
    }
    return 1;
}

/* Command Serial Bus to TALK, LISTEN, UNTALK, or UNLISTEN, and send the
   Secondary Address to Serial Bus under Attention.  */
int serial_trap_attention(void)
{
    uint8_t iecdata;
    serial_t *p;

    iecdata = mem_read(((uint8_t)(BSOUR))); /* BSOUR - character for serial bus */

    if ((iecdata == UNLISTEN) || (iecdata == UNTALK)) {
        /* transfer ends */
        DBG(("serial_trap_attention unlisten/untalk for device %d", ActiveDevice));
    } else if (((iecdata & 0xf0) == OPEN) || ((iecdata & 0xf0) == CLOSE)) {
        DBG(("serial_trap_attention open/close for device %d", ActiveDevice));
    } else if (((iecdata & 0xf0) == LISTEN) || ((iecdata & 0xf0) == TALK)) {
        ActiveDevice = iecdata & DEVNR_MASK;
        DBG(("serial_trap_attention listen/talk for device %d", ActiveDevice));
    }

    if (!device_uses_serial_traps(ActiveDevice)) {
        DBG(("serial_trap_attention aborted (dev %d)", ActiveDevice));
        if ((iecdata == UNLISTEN) || (iecdata == UNTALK)) {
            ActiveDevice = 0;
        }
        return 0;
    }

    DBG(("serial_trap_attention 0x%02x", iecdata));

    /* do a flush if unlisten for close and command channel */
    if (iecdata == UNLISTEN) {
        DBG(("serial_trap_attention unlisten TrapDevice: 0x%02x", TrapDevice));
        serial_iec_bus_unlisten(TrapDevice, TrapSecondary, serial_set_st);
        ActiveDevice = 0;
    } else if (iecdata == UNTALK) {
        DBG(("serial_trap_attention untalk TrapDevice: 0x%02x", TrapDevice));
        serial_iec_bus_untalk(TrapDevice, TrapSecondary, serial_set_st);
        ActiveDevice = 0;
    } else {
        switch (iecdata & 0xf0) {
            case LISTEN:
            case TALK:
                TrapDevice = iecdata;
                TrapSecondary = 0;
                DBG(("serial_trap_attention listen/talk TrapDevice: 0x%02x TrapSecondary: 0x%02x", TrapDevice, TrapSecondary));
                break;
            case SECONDARY:
                DBG(("serial_trap_attention secondary TrapDevice: 0x%02x iecdata: 0x%02x", TrapDevice, iecdata));
                send_listen_talk_secondary(iecdata);
                break;
            case CLOSE:
                TrapSecondary = iecdata;
                DBG(("serial_trap_attention close TrapDevice: 0x%02x TrapSecondary: 0x%02x", TrapDevice, TrapSecondary));
                serial_iec_bus_close(TrapDevice, TrapSecondary, serial_set_st);
                break;
            case OPEN:
                TrapSecondary = iecdata;
                DBG(("serial_trap_attention open TrapDevice: 0x%02x TrapSecondary: 0x%02x", TrapDevice, TrapSecondary));
                serial_iec_bus_open(TrapDevice, TrapSecondary, serial_set_st);
                break;
        }
    }

    p = serial_device_get(TrapDevice & DEVNR_MASK);
    DBG(("serial_trap_attention p:%p inuse:%d", p, p->inuse));
    if (!(p->inuse)) {
        serial_set_st(0x80);
    }

    maincpu_set_carry(0);
    maincpu_set_interrupt(0);

    if (attention_callback_func) {
        attention_callback_func();
    }

    return 1;
}

/* Send one byte on the serial bus.  */
int serial_trap_send(void)
{
    uint8_t iecdata;

    if (!device_uses_serial_traps(ActiveDevice)) {
        DBG(("serial_trap_send aborted (dev %d) no traps", ActiveDevice));
        return 0;
    }

    DBG(("serial_trap_send (TrapDevice 0x%02x)", TrapDevice));

    /*
     * If no secondary address was sent, it means that no LISTEN was
     * sent either. Do both now with SA = 0.
     */
    if (TrapSecondary == 0) {
        send_listen_talk_secondary(SECONDARY + 0);
    }

    iecdata = mem_read(BSOUR); /* BSOUR - character for serial bus */

    serial_iec_bus_write(TrapDevice, TrapSecondary, iecdata, serial_set_st);

    maincpu_set_carry(0);
    maincpu_set_interrupt(0);

    return 1;
}

/* Receive one byte from the serial bus.  */
int serial_trap_receive(void)
{
    uint8_t data;

    if (!device_uses_serial_traps(ActiveDevice)) {
        DBG(("serial_trap_receive aborted (dev %d) no traps", ActiveDevice));
        return 0;
    }

    DBG(("serial_trap_receive (TrapDevice 0x%02x)", TrapDevice));

    /*
     * If no secondary address was sent, it means that no TALK was
     * sent either. Do both now with SA = 0.
     */
    if (TrapSecondary == 0) {
        send_listen_talk_secondary(SECONDARY + 0);
    }
    data = serial_iec_bus_read(TrapDevice, TrapSecondary, serial_set_st);

    mem_store(tmp_in, data);

    /* If at EOF, call specified callback function.  */
    if ((serial_get_st() & 0x40) && eof_callback_func != NULL) {
        eof_callback_func();
    }

    /* Set registers like the Kernal routine does.  */
    maincpu_set_a(data);
    maincpu_set_sign((data & 0x80) ? 1 : 0);
    maincpu_set_zero(data ? 0 : 1);
    maincpu_set_carry(0);
    maincpu_set_interrupt(0);

    return 1;
}


/* Kernal loops serial-port (0xdd00) to see when serial is ready: fake it.
   EEA9 Get serial data and clk in (TKSA subroutine).  */

int serial_trap_ready(void)
{
    if (!device_uses_serial_traps(ActiveDevice)) {
        DBG(("serial_trap_ready aborted (dev %d) no traps", ActiveDevice));
        return 0;
    }

    DBG(("serial_trap_ready (TrapDevice 0x%02x)", TrapDevice));

    maincpu_set_a(1);
    maincpu_set_sign(0);
    maincpu_set_zero(0);
    maincpu_set_interrupt(0);
    return 1;
}

/* Initializing the IEC bus and IEC device will move once serial.c is not
   referenced by PET and CBM2 anymore. */
int serial_resources_init(void)
{
    return serial_iec_device_resources_init();
}

int serial_cmdline_options_init(void)
{
    return serial_iec_device_cmdline_options_init();
}

void serial_trap_init(uint16_t tmpin)
{
    serial_iec_device_init();

    tmp_in = tmpin;
}

/* FIXME: bad name, this function is basically the main/top entry point for
          doing a IEC reset that distributes to all drives. It does however NOT
          reset the true-drive emulated drive CPUs (that is done in drive_reset)
*/
/* called by machine_specific_reset() */
void serial_traps_reset(void)
{
    serial_iec_bus_reset();
    serial_iec_device_reset();
}

/* Specify a function to call when EOF happens in `serialreceivebyte()'.  */
void serial_trap_eof_callback_set(void (*func)(void))
{
    eof_callback_func = func;
}

/* Specify a function to call when the `serialattention()' trap is called.  */
void serial_trap_attention_callback_set(void (*func)(void))
{
    attention_callback_func = func;
}

void serial_trap_truedrive_set(unsigned int unit, unsigned int flag)
{
    DBG(("serial_trap_truedrive_set unit: %u flag: %u", unit, flag));
    serial_truedrive[unit] = flag;
}
