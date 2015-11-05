/*
 * petpia1.c -- PIA#1 chip emulation.
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

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "crtc.h"
#include "datasette.h"
#include "drive.h"
#include "interrupt.h"
#include "keyboard.h"
#include "maincpu.h"
#include "parallel.h"
#include "pets.h"
#include "petmem.h"
#include "petpia.h"
#include "piacore.h"
#include "resources.h"
#include "translate.h"
#include "types.h"

/* ------------------------------------------------------------------------- */
/* Renaming exported functions */

#define MYPIA_NAME      "PIA1"

#define mypia_init pia1_init
#define mypia_reset pia1_reset
#define mypia_store pia1_store
#define mypia_read pia1_read
#define mypia_peek pia1_peek
#define mypia_snapshot_write_module pia1_snapshot_write_module
#define mypia_snapshot_read_module pia1_snapshot_read_module
#define mypia_signal pia1_signal
#define mypia_dump pia1_dump

static piareg mypia;

/* ------------------------------------------------------------------------- */
/* CPU binding */

static void my_set_int(unsigned int pia_int_num, int a)
{
    maincpu_set_irq(pia_int_num, a ? IK_IRQ : IK_NONE);
}

static void my_restore_int(unsigned int pia_int_num, int a)
{
    interrupt_restore_irq(maincpu_int_status, pia_int_num,
                          a ? IK_IRQ : IK_NONE);
}

#define mycpu_rmw_flag   maincpu_rmw_flag
#define myclk            maincpu_clk
#define mycpu_int_status maincpu_int_status

/* ------------------------------------------------------------------------- */
/* PIA resources.  */

/* Flag: is the diagnostic pin enabled?  */
static int diagnostic_pin_enabled;

static int set_diagnostic_pin_enabled(int val, void *param)
{
    diagnostic_pin_enabled = val ? 1 : 0;

    return 0;
}

static const resource_int_t resources_int[] = {
    { "DiagPin", 0, RES_EVENT_SAME, NULL,
      &diagnostic_pin_enabled, set_diagnostic_pin_enabled, NULL },
    { NULL }
};

int pia1_resources_init(void)
{
    return resources_register_int(resources_int);
}


static const cmdline_option_t cmdline_options[] = {
    { "-diagpin", SET_RESOURCE, 0,
      NULL, NULL, "DiagPin", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_USERPORT_DIAG_PIN,
      NULL, NULL },
    { "+diagpin", SET_RESOURCE, 0,
      NULL, NULL, "DiagPin", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_USERPORT_DIAG_PIN,
      NULL, NULL },
    { NULL }
};

int pia1_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int tape1_sense = 0;

static int old_cb2_status = 0xff;

void pia1_set_tape_sense(int v)
{
    tape1_sense = v;
}

/* ------------------------------------------------------------------------- */
/* I/O */

static void pia_set_ca2(int a)
{
    parallel_cpu_set_eoi((BYTE)((a) ? 0 : 1));
    if (petres.pet2k) {
        crtc_screen_enable((a) ? 1 : 0);
    }
}

static void pia_set_cb2(int a)
{
    if (old_cb2_status != a) {
        datasette_set_motor(!a);
        old_cb2_status = a;
    }
}

static void pia_reset(void)
{
}


/*
E810    PORT A  7   Diagnostic sense (pin 5 on the user port)
                6   IEEE EOI in
                5   Cassette sense #2
                4   Cassette sense #1
                3-0 Keyboard row select (through 4->10 decoder)
E811    CA2         output to blank the screen (old PETs only)
                    IEEE EOI out
        CA1         cassette #1 read line
E812    PORT B  7-0 Contents of keyboard row
                    Usually all or all but one bits set.
E813    CB2         output to cassette #1 motor: 0=on, 1=off
        CB1         screen retrace detection in


         Control

 7    CA1 active transition flag. 1= 0->1, 0= 1->0
 6    CA2 active transition flag. 1= 0->1, 0= 1->0
 5    CA2 direction           1 = out        | 0 = in
                    ------------+------------+---------------------
 4    CA2 control   Handshake=0 | Manual=1   | Active: High=1 Low=0
 3    CA2 control   On Read=0   | CA2 High=1 | IRQ on=1, IRQ off=0
                    Pulse  =1   | CA2 Low=0  |

 2    Port A control: DDRA = 0, IORA = 1
 1    CA1 control: Active High = 1, Low = 0
 0    CA1 control: IRQ on=1, off = 0
*/

static void store_pa(BYTE byte)
{
}

static void store_pb(BYTE byte)
{
}

static void undump_pa(BYTE byte)
{
}

static void undump_pb(BYTE byte)
{
}


static BYTE read_pa(void)
{
    BYTE byte;

    drive_cpu_execute_all(maincpu_clk);

    byte = 0xff
           - (tape1_sense ? 16 : 0)
           - (parallel_eoi ? 64 : 0)
           - ((diagnostic_pin_enabled || petmem_superpet_diag()) ? 128 : 0);
    byte = ((byte & ~mypia.ddr_a) | (mypia.port_a & mypia.ddr_a));

    return byte;
}


static BYTE read_pb(void)
{
    int row;
    BYTE j = 0xFF;

    row = mypia.port_a & 15;

    if (row < KBD_ROWS) {
        j = ~keyarr[row];
    }

#if (defined(DEBUG_PIA) || defined(KBDBUG))
    if (j < 255) {
        log_message(mypia_log,
                    "%02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X - row %d  %02x",
                    keyarr[0], keyarr[1], keyarr[2], keyarr[3], keyarr[4],
                    keyarr[5], keyarr[6], keyarr[7], keyarr[8], keyarr[9],
                    row, j);
    }
#endif

    return j;
}

#include "piacore.c"
