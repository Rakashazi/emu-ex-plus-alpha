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

#include "vice.h"

#include <stdio.h>

#include "maincpu.h"
#include "mem.h"
#include "mos6510.h"
#include "serial-iec-bus.h"
/* Will be removed once serial.c is clean */
#include "serial-iec-device.h"
#include "serial-trap.h"
#include "serial.h"
#include "types.h"


/* Warning: these are only valid for the VIC20, C64 and C128, but *not* for
   the PET.  (FIXME?)  */
#define BSOUR 0x95 /* Buffered Character for IEEE Bus */


/* Address of serial TMP register.  */
static WORD tmp_in;

/* On which channel did listen happen to?  */
static BYTE TrapDevice;
static BYTE TrapSecondary;

/* Function to call when EOF happens in `serialreceivebyte()'.  */
static void (*eof_callback_func)(void);

/* Function to call when the `serialattention()' trap is called.  */
static void (*attention_callback_func)(void);

static unsigned int serial_truedrive;


static void serial_set_st(BYTE st)
{
    mem_store((WORD)0x90, (BYTE)(mem_read((WORD)0x90) | st));
}

static BYTE serial_get_st(void)
{
    return mem_read((WORD)0x90);
}


/* Command Serial Bus to TALK, LISTEN, UNTALK, or UNLISTEN, and send the
   Secondary Address to Serial Bus under Attention.  */
int serial_trap_attention(void)
{
    BYTE b;
    serial_t *p;

    /*
     * Which Secondary Address ?
     */
    b = mem_read(((BYTE)(BSOUR))); /* BSOUR - character for serial bus */

    if (((b & 0xf0) == 0x20) || ((b & 0xf0) == 0x40)) {
        if (serial_truedrive && ((b & 0x0f) != 4 ) && ((b & 0x0f) != 5)) {
            /* Set TrapDevice even if the trap is not taken; needed
               for other traps.  */
            TrapDevice = b;
            return 0;
        }
    } else {
        if (serial_truedrive && ((TrapDevice & 0x0f) != 4)
            && ((TrapDevice & 0x0f) != 5)) {
            return 0;
        }
    }

    /* do a flush if unlisten for close and command channel */
    if (b == 0x3f) {
        serial_iec_bus_unlisten(TrapDevice, TrapSecondary, serial_set_st);
    } else if (b == 0x5f) {
        serial_iec_bus_untalk(TrapDevice, TrapSecondary, serial_set_st);
    } else {
        switch (b & 0xf0) {
            case 0x20:
            case 0x40:
                TrapDevice = b;
                break;
            case 0x60:
                TrapSecondary = b;
                switch (TrapDevice & 0xf0) {
                    case 0x20:
                        serial_iec_bus_listen(TrapDevice, TrapSecondary, serial_set_st);
                        break;
                    case 0x40:
                        serial_iec_bus_talk(TrapDevice, TrapSecondary, serial_set_st);
                        break;
                }
                break;
            case 0xe0:
                TrapSecondary = b;
                serial_iec_bus_close(TrapDevice, TrapSecondary, serial_set_st);
                break;
            case 0xf0:
                TrapSecondary = b;
                serial_iec_bus_open(TrapDevice, TrapSecondary, serial_set_st);
                break;
        }
    }

    p = serial_device_get(TrapDevice & 0x0f);
    if (!(p->inuse)) {
        serial_set_st(0x80);
    }

    MOS6510_REGS_SET_CARRY(&maincpu_regs, 0);
    MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, 0);

    if (attention_callback_func) {
        attention_callback_func();
    }

    return 1;
}

/* Send one byte on the serial bus.  */
int serial_trap_send(void)
{
    BYTE data;

    if (serial_truedrive && ((TrapDevice & 0x0f) != 4) && ((TrapDevice & 0x0f) != 5)) {
        return 0;
    }

    data = mem_read(BSOUR); /* BSOUR - character for serial bus */

    serial_iec_bus_write(TrapDevice, TrapSecondary, data, serial_set_st);

    MOS6510_REGS_SET_CARRY(&maincpu_regs, 0);
    MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, 0);

    return 1;
}

/* Receive one byte from the serial bus.  */
int serial_trap_receive(void)
{
    BYTE data;

    if (serial_truedrive && ((TrapDevice & 0x0f) != 4) && ((TrapDevice & 0x0f) != 5)) {
        return 0;
    }

    data = serial_iec_bus_read(TrapDevice, TrapSecondary, serial_set_st);

    mem_store(tmp_in, data);

    /* If at EOF, call specified callback function.  */
    if ((serial_get_st() & 0x40) && eof_callback_func != NULL) {
        eof_callback_func();
    }

    /* Set registers like the Kernal routine does.  */
    MOS6510_REGS_SET_A(&maincpu_regs, data);
    MOS6510_REGS_SET_SIGN(&maincpu_regs, (data & 0x80) ? 1 : 0);
    MOS6510_REGS_SET_ZERO(&maincpu_regs, data ? 0 : 1);
    MOS6510_REGS_SET_CARRY(&maincpu_regs, 0);
    MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, 0);

    return 1;
}


/* Kernal loops serial-port (0xdd00) to see when serial is ready: fake it.
   EEA9 Get serial data and clk in (TKSA subroutine).  */

int serial_trap_ready(void)
{
    if (serial_truedrive && ((TrapDevice & 0x0f) != 4) && ((TrapDevice & 0x0f) != 5)) {
        return 0;
    }

    MOS6510_REGS_SET_A(&maincpu_regs, 1);
    MOS6510_REGS_SET_SIGN(&maincpu_regs, 0);
    MOS6510_REGS_SET_ZERO(&maincpu_regs, 0);
    MOS6510_REGS_SET_INTERRUPT(&maincpu_regs, 0);

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

void serial_trap_init(WORD tmpin)
{
    serial_iec_device_init();

    tmp_in = tmpin;
}

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

void serial_trap_truedrive_set(unsigned int flag)
{
    serial_truedrive = flag;
}
